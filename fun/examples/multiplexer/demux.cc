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

typedef fun::SharedPtr<TcpClient> TcpClientPtr;

// const int kMaxConns = 1;
const size_t kMaxPacketLen = 255;
const size_t kHeaderLen = 3;

const uint16_t kListenPort = 9999;
const char* kSocksIp = "127.0.0.1";
const uint16_t kSocksPort = 7777;

struct Entry {
  int conn_id;
  TcpClientPtr client;
  TcpConnectionPtr connection;
  Buffer pending;
};

class DemuxServer : Noncopyable {
 public:
  DemuxServer(EventLoop* loop, const InetAddress& listen_addr,
              const InetAddress& socks_addr)
      : loop_(loop),
        server_(loop, listen_addr, "DemuxServer"),
        socks_addr_(socks_addr) {
    server_.SetConnectionCallback(
        boost::bind(&DemuxServer::OnServerConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&DemuxServer::OnServerMessage, this, _1, _2, _3));
  }

  void Start() { server_.Start(); }

  void OnServerConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      if (server_conn_) {
        conn->Shutdown();
      } else {
        server_conn_ = conn;
        LOG_INFO << "OnServerConnection set server_conn_";
      }
    } else {
      if (server_conn_ == conn) {
        server_conn_.Reset();
        socks_conns_.Clear();

        LOG_INFO << "OnServerConnection Reset server_conn_";
      }
    }
  }

  void OnServerMessage(const TcpConnectionPtr& conn, Buffer* buf,
                       const Timestamp&) {
    while (buf->GetReadableLength() > kHeaderLen) {
      int len = static_cast<uint8_t>(*buf->GetReadablePtr());
      if (buf->GetReadableLength() < len + kHeaderLen) {
        break;
      } else {
        int conn_id = static_cast<uint8_t>(buf->GetReadablePtr()[1]);
        conn_id |= (static_cast<uint8_t>(buf->GetReadablePtr()[2]) << 8);

        if (conn_id != 0) {
          fun_check(socks_conns_.find(conn_id) != socks_conns_.end());
          TcpConnectionPtr& socks_conn = socks_conns_[conn_id].connection;
          if (socks_conn) {
            fun_check(socks_conns_[conn_id].pending.GetReadableLength() == 0);
            socks_conn->Send(buf->GetReadablePtr() + kHeaderLen, len);
          } else {
            socks_conns_[conn_id].pending.Append(
                buf->GetReadablePtr() + kHeaderLen, len);
          }
        } else {
          String cmd(buf->GetReadablePtr() + kHeaderLen, len);
          DoCommand(cmd);
        }
        buf->Drain(len + kHeaderLen);
      }
    }
  }

  void DoCommand(const String& cmd) {
    static const String kConn = "CONN ";

    int conn_id = atoi(&cmd[kConn.size()]);
    bool isUp = cmd.find(" IS UP") != String::npos;
    LOG_INFO << "DoCommand " << conn_id << " " << isUp;
    if (isUp) {
      fun_check(socks_conns_.find(conn_id) == socks_conns_.end());
      char conn_name[256];
      snprintf(conn_name, sizeof conn_name, "SocksClient %d", conn_id);
      Entry entry;
      entry.conn_id = conn_id;
      entry.client.Reset(new TcpClient(loop_, socks_addr_, conn_name));
      entry.client->SetConnectionCallback(
          boost::bind(&DemuxServer::OnSocksConnection, this, conn_id, _1));
      entry.client->SetMessageCallback(
          boost::bind(&DemuxServer::OnSocksMessage, this, conn_id, _1, _2, _3));
      // FIXME: SetWriteCompleteCallback
      socks_conns_[conn_id] = entry;
      entry.client->Connect();
    } else {
      fun_check(socks_conns_.find(conn_id) != socks_conns_.end());
      TcpConnectionPtr& socks_conn = socks_conns_[conn_id].connection;
      if (socks_conn) {
        socks_conn->Shutdown();
      } else {
        socks_conns_.erase(conn_id);
      }
    }
  }

  void OnSocksConnection(int conn_id, const TcpConnectionPtr& conn) {
    fun_check(socks_conns_.find(conn_id) != socks_conns_.end());
    if (conn->IsConnected()) {
      socks_conns_[conn_id].connection = conn;
      Buffer& pending_data = socks_conns_[conn_id].pending;
      if (pending_data.GetReadableLength() > 0) {
        conn->Send(&pending_data);
      }
    } else {
      if (server_conn_) {
        char buf[256];
        int len = snprintf(buf, sizeof(buf), "DISCONNECT %d\r\n", conn_id);
        Buffer buffer;
        buffer.Append(buf, len);
        SendServerPacket(0, &buffer);
      } else {
        socks_conns_.erase(conn_id);
      }
    }
  }

  void OnSocksMessage(int conn_id, const TcpConnectionPtr& conn, Buffer* buf,
                      const Timestamp&) {
    fun_check(socks_conns_.find(conn_id) != socks_conns_.end());
    while (buf->GetReadableLength() > kMaxPacketLen) {
      Buffer packet;
      packet.Append(buf->GetReadablePtr(), kMaxPacketLen);
      buf->Drain(kMaxPacketLen);
      SendServerPacket(conn_id, &packet);
    }

    if (buf->GetReadableLength() > 0) {
      SendServerPacket(conn_id, buf);
    }
  }

  void SendServerPacket(int conn_id, Buffer* buf) {
    size_t len = buf->GetReadableLength();
    LOG_DEBUG << len;
    fun_check(len <= kMaxPacketLen);
    uint8_t header[kHeaderLen] = {
        static_cast<uint8_t>(len), static_cast<uint8_t>(conn_id & 0xFF),
        static_cast<uint8_t>((conn_id & 0xFF00) >> 8)};
    buf->Prepend(header, kHeaderLen);
    if (server_conn_) {
      server_conn_->Send(buf);
    }
  }

  EventLoop* loop_;
  TcpServer server_;
  TcpConnectionPtr server_conn_;
  const InetAddress socks_addr_;
  std::map<int, Entry> socks_conns_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  InetAddress listen_addr(kListenPort);
  if (argc > 1) {
    kSocksIp = argv[1];
  }

  InetAddress socks_addr(kSocksIp, kSocksPort);

  EventLoop loop;
  DemuxServer server(&loop, listen_addr, socks_addr);
  server.Start();
  loop.Loop();
}
