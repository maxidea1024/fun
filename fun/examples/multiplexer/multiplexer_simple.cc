#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <queue>
#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

const int kMaxConns = 10; // 65535
const size_t kMaxPacketLen = 255;
const size_t kHeaderLen = 3;

const uint16_t kClientPort = 3333;
const char* backend_ip = "127.0.0.1";
const uint16_t kBackendPort = 9999;

class MultiplexServer : Noncopyable {
 public:
  MultiplexServer(EventLoop* loop,
                  const InetAddress& listen_addr,
                  const InetAddress& backend_addr)
    : server_(loop, listen_addr, "MultiplexServer")
    , backend_(loop, backend_addr, "MultiplexBackend") {
    server_.SetConnectionCallback(boost::bind(&MultiplexServer::OnClientConnection, this, _1));
    server_.SetMessageCallback(boost::bind(&MultiplexServer::OnClientMessage, this, _1, _2, _3));
    backend_.SetConnectionCallback(boost::bind(&MultiplexServer::OnBackendConnection, this, _1));
    backend_.SetMessageCallback(boost::bind(&MultiplexServer::OnBackendMessage, this, _1, _2, _3));

    backend_.EnableRetry();
  }

  void Start() {
    backend_.Connect();
    server_.Start();
  }

 private:
  void OnClientConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << "Client " << conn->GetPeerAddress().ToIpPort() << " -> "
              << conn->GetLocalAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      int id = -1;
      if (!avail_ids_.empty()) {
        id = avail_ids_.front();
        avail_ids_.pop();
        client_conns_[id] = conn;
      }

      if (id <= 0) {
        // no client id available
        conn->Shutdown();
      }
      else {
        conn->SetContext(id);
        char buf[256];
        snprintf(buf, sizeof(buf), "CONN %d FROM %s IS UP\r\n",
                 id, conn->GetPeerAddress().ToIpPort().c_str());
        SendBackendString(0, buf);
      }
    }
    else {
      if (!conn->GetContext().empty()) {
        int id = boost::any_cast<int>(conn->GetContext());
        fun_check(id > 0 && id <= kMaxConns);
        char buf[256];
        snprintf(buf, sizeof(buf), "CONN %d FROM %s IS DOWN\r\n",
                 id, conn->GetPeerAddress().ToIpPort().c_str());
        SendBackendString(0, buf);

        if (backend_conn_) {
          // put client id back for reusing
          avail_ids_.push(id);
          client_conns_.erase(id);
        }
        else {
          fun_check(avail_ids_.empty());
          fun_check(client_conns_.empty());
        }
      }
    }
  }

  void SendBackendString(int id, const String& msg) {
    fun_check(msg.size() <= kMaxPacketLen);
    Buffer buf;
    buf.Append(msg);
    SendBackendPacket(id, &buf);
  }

  //TODO eventloop에서 호출되므로, 지연이 발생하면 네트워크 전반에 악영향을
  //미칠 수 있으므로, blocking 동작을 처리할 경우에는 별도의 스레드풀을 사용
  //해야함.
  void OnClientMessage( const TcpConnectionPtr& conn,
                        Buffer* buf,
                        const Timestamp&) {
    if (!conn->GetContext().empty()) {
      //TODO boost any를 대체할 수단이 필요해보이는데...
      int id = boost::any_cast<int>(conn->GetContext());
      SendBackendBuffer(id, buf);
    }
    else {
      buf->DrainAll();
      // FIXME: error handling
    }
  }

  void SendBackendBuffer(int id, Buffer* buf) {
    while (buf->GetReadableLength() > kMaxPacketLen) {
      Buffer packet;
      packet.Append(buf->GetReadablePtr(), kMaxPacketLen);
      buf->Drain(kMaxPacketLen);
      SendBackendPacket(id, &packet);
    }

    if (buf->GetReadableLength() > 0) {
      SendBackendPacket(id, buf);
    }
  }

  void SendBackendPacket(int id, Buffer* buf) {
    size_t len = buf->GetReadableLength();
    LOG_DEBUG << "SendBackendPacket " << len;
    fun_check(len <= kMaxPacketLen);
    uint8_t header[kHeaderLen] = {
      static_cast<uint8_t>(len),
      static_cast<uint8_t>(id & 0xFF),
      static_cast<uint8_t>((id & 0xFF00) >> 8)
    };
    buf->Prepend(header, kHeaderLen);
    if (backend_conn_) {
      backend_conn_->Send(buf);
    }
  }

  void OnBackendConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << "Backend " << conn->GetLocalAddress().ToIpPort() << " -> "
              << conn->GetPeerAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      backend_conn_ = conn;
      fun_check(avail_ids_.empty());
      for (int i = 1; i <= kMaxConns; ++i) {
        avail_ids_.push(i);
      }
    }
    else {
      backend_conn_.Reset();
      for (std::map<int, TcpConnectionPtr>::iterator it = client_conns_.begin();
          it != client_conns_.end();
          ++it) {
        it->second->Shutdown();
      }
      client_conns_.clear();
      while (!avail_ids_.empty()) {
        avail_ids_.pop();
      }
    }
  }

  void OnBackendMessage(const TcpConnectionPtr& conn,
                        Buffer* buf,
                        const Timestamp&) {
    SendToClient(buf);
  }

  void SendToClient(Buffer* buf) {
    while (buf->GetReadableLength() > kHeaderLen) {
      int len = static_cast<uint8_t>(*buf->GetReadablePtr());
      if (buf->GetReadableLength() < len + kHeaderLen) {
        break;
      }
      else {
        int id = static_cast<uint8_t>(buf->GetReadablePtr()[1]);
        id |= (static_cast<uint8_t>(buf->GetReadablePtr()[2]) << 8);

        if (id != 0) {
          std::map<int, TcpConnectionPtr>::iterator it = client_conns_.find(id);
          if (it != client_conns_.end()) {
            it->second->Send(buf->GetReadablePtr() + kHeaderLen, len);
          }
        }
        else {
          String cmd(buf->GetReadablePtr() + kHeaderLen, len);
          LOG_INFO << "Backend cmd " << cmd;
          DoCommand(cmd);
        }
        buf->Drain(len + kHeaderLen);
      }
    }
  }

  void DoCommand(const String& cmd) {
    static const String kDisconnectCmd = "DISCONNECT ";

    if (cmd.size() > kDisconnectCmd.size()
        && std::equal(kDisconnectCmd.begin(), kDisconnectCmd.end(), cmd.begin())) {
      int conn_id = atoi(&cmd[kDisconnectCmd.size()]);
      std::map<int, TcpConnectionPtr>::iterator it = client_conns_.find(conn_id);
      if (it != client_conns_.end()) {
        it->second->Shutdown();
      }
    }
  }

  TcpServer server_;
  TcpClient backend_;
  // MutexLock mutex_;
  TcpConnectionPtr backend_conn_;
  std::map<int, TcpConnectionPtr> client_conns_;
  std::queue<int> avail_ids_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc > 1) {
    backend_ip = argv[1];
  }

  EventLoop loop;
  InetAddress listen_addr(kClientPort);
  InetAddress backend_addr(backend_ip, kBackendPort);
  MultiplexServer server(&loop, listen_addr, backend_addr);
  server.Start();
  loop.Loop();
}
