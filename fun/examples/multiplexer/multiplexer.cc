#include <red/base/atomic.h>
#include <red/base/logging.h>
#include <red/base/mutex.h>
#include <red/base/thread.h>
#include <red/net/event_loop.h>
#include <red/net/inet_address.h>
#include <red/net/tcp_client.h>
#include <red/net/tcp_server.h>

#include <boost/bind.hpp>

#include <queue>
#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

const int kMaxConns = 10;  // 65535
const size_t kMaxPacketLen = 255;
const size_t kHeaderLen = 3;

const uint16_t kClientPort = 3333;
const char* backend_ip = "127.0.0.1";
const uint16_t kBackendPort = 9999;

class MultiplexServer {
 public:
  MultiplexServer(EventLoop* loop, const InetAddress& listen_addr,
                  const InetAddress& backend_addr, int thread_count)
      : server_(loop, listen_addr, "MultiplexServer"),
        backend_(loop, backend_addr, "MultiplexBackend"),
        thread_count_(thread_count),
        old_counter_(0),
        start_time_(Timestamp::Now()) {
    server_.SetConnectionCallback(
        boost::bind(&MultiplexServer::OnClientConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&MultiplexServer::OnClientMessage, this, _1, _2, _3));
    server_.SetThreadCount(thread_count);

    backend_.SetConnectionCallback(
        boost::bind(&MultiplexServer::OnBackendConnection, this, _1));
    backend_.SetMessageCallback(
        boost::bind(&MultiplexServer::OnBackendMessage, this, _1, _2, _3));

    backend_.EnableRetry();

    // loop->ScheduleEvery(10.0, boost::bind(&MultiplexServer::PrintStatistics,
    // this));
  }

  void Start() {
    LOG_INFO << "starting " << thread_count_ << " threads.";
    backend_.Connect();
    server_.Start();
  }

 private:
  void SendBackendPacket(int id, Buffer* buf) {
    size_t len = buf->GetReadableLength();
    fun_check(len <= kMaxPacketLen);
    uint8_t header[kHeaderLen] = {static_cast<uint8_t>(len),
                                  static_cast<uint8_t>(id & 0xFF),
                                  static_cast<uint8_t>((id & 0xFF00) >> 8)};
    buf->Prepend(header, kHeaderLen);
    TcpConnectionPtr backend_conn;
    {
      ScopedLock guard(mutex_);
      backend_conn = backend_conn_;
    }

    if (backend_conn) {
      backend_conn->Send(buf);
    }
  }

  void SendBackendString(int id, const String& msg) {
    fun_check(msg.size() <= kMaxPacketLen);
    Buffer buf;
    buf.Append(msg);
    SendBackendPacket(id, &buf);
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

  void SendToClient(Buffer* buf) {
    while (buf->GetReadableLength() > kHeaderLen) {
      int len = static_cast<uint8_t>(*buf->GetReadablePtr());
      if (buf->GetReadableLength() < len + kHeaderLen) {
        break;
      } else {
        int id = static_cast<uint8_t>(buf->GetReadablePtr()[1]);
        id |= (static_cast<uint8_t>(buf->GetReadablePtr()[2]) << 8);

        TcpConnectionPtr client_conn;
        {
          ScopedLock guard(mutex_);
          std::map<int, TcpConnectionPtr>::iterator it = client_conns_.find(id);
          if (it != client_conns_.end()) {
            client_conn = it->second;
          }
        }

        if (client_conn) {
          client_conn->Send(buf->GetReadablePtr() + kHeaderLen, len);
        }

        buf->Drain(len + kHeaderLen);
      }
    }
  }

  void OnClientConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << "Client " << conn->GetPeerAddress().ToIpPort() << " -> "
              << conn->GetLocalAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      int id = -1;
      {
        ScopedLock guard(mutex_);
        if (!avail_ids_.empty()) {
          id = avail_ids_.front();
          avail_ids_.pop();
          client_conns_[id] = conn;
        }
      }

      if (id <= 0) {
        conn->Shutdown();
      } else {
        conn->SetContext(id);
        char buf[256];
        snprintf(buf, sizeof(buf), "CONN %d FROM %s IS UP\r\n", id,
                 conn->GetPeerAddress().ToIpPort().c_str());
        SendBackendString(0, buf);
      }
    } else {
      if (!conn->GetContext().empty()) {
        int id = boost::any_cast<int>(conn->GetContext());
        fun_check(id > 0 && id <= kMaxConns);
        char buf[256];
        snprintf(buf, sizeof(buf), "CONN %d FROM %s IS DOWN\r\n", id,
                 conn->GetPeerAddress().ToIpPort().c_str());
        SendBackendString(0, buf);

        ScopedLock guard(mutex_);
        if (backend_conn_) {
          avail_ids_.push(id);
          client_conns_.erase(id);
        } else {
          fun_check(avail_ids_.empty());
          fun_check(client_conns_.empty());
        }
      }
    }
  }

  void OnClientMessage(const TcpConnectionPtr& conn, Buffer* buf,
                       const Timestamp&) {
    size_t len = buf->GetReadableLength();
    transferred_.addAndGet(len);
    received_messages_.IncrementAndGet();
    if (!conn->GetContext().empty()) {
      int id = boost::any_cast<int>(conn->GetContext());
      SendBackendBuffer(id, buf);
      // fun_check(buf->GetReadableLength() == 0);
    } else {
      buf->DrainAll();
      // FIXME: error handling
    }
  }

  void OnBackendConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << "Backend " << conn->GetLocalAddress().ToIpPort() << " -> "
              << conn->GetPeerAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");

    std::vector<TcpConnectionPtr> conns_to_destroy;
    if (conn->IsConnected()) {
      ScopedLock guard(mutex_);
      backend_conn_ = conn;
      fun_check(avail_ids_.empty());
      for (int i = 1; i <= kMaxConns; ++i) {
        avail_ids_.push(i);
      }
    } else {
      ScopedLock guard(mutex_);
      backend_conn_.Reset();
      conns_to_destroy.reserve(client_conns_.size());
      for (std::map<int, TcpConnectionPtr>::iterator it = client_conns_.begin();
           it != client_conns_.end(); ++it) {
        conns_to_destroy.push_back(it->second);
      }
      client_conns_.clear();
      while (!avail_ids_.empty()) {
        avail_ids_.pop();
      }
    }

    for (std::vector<TcpConnectionPtr>::iterator it = conns_to_destroy.begin();
         it != conns_to_destroy.end(); ++it) {
      (*it)->Shutdown();
    }
  }

  void OnBackendMessage(const TcpConnectionPtr& conn, Buffer* buf,
                        const Timestamp&) {
    size_t len = buf->GetReadableLength();
    transferred_.addAndGet(len);
    received_messages_.IncrementAndGet();
    SendToClient(buf);
  }

  void PrintStatistics() {
    Timestamp end_time = Timestamp::Now();
    int64_t new_counter = transferred_.get();
    int64_t bytes = new_counter - old_counter_;
    int64_t msgs = received_messages_.getAndSet(0);
    double time = TimeDifference(end_time, start_time_);
    printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
           static_cast<double>(bytes) / time / 1024 / 1024,
           static_cast<double>(msgs) / time / 1024,
           static_cast<double>(bytes) / static_cast<double>(msgs));

    old_counter_ = new_counter;
    start_time_ = end_time;
  }

  TcpServer server_;
  TcpClient backend_;
  int thread_count_;
  AtomicInt64 transferred_;
  AtomicInt64 received_messages_;
  int64_t old_counter_;
  Timestamp start_time_;
  MutexLock mutex_;
  TcpConnectionPtr backend_conn_;
  std::map<int, TcpConnectionPtr> client_conns_;
  std::queue<int> avail_ids_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid()
           << ", tid = " << Thread::CurrentTid();

  int thread_count = 4;
  if (argc > 1) {
    backend_ip = argv[1];
  }
  if (argc > 2) {
    thread_count = atoi(argv[2]);
  }

  EventLoop loop;
  InetAddress listen_addr(kClientPort);
  InetAddress backend_addr(backend_ip, kBackendPort);
  MultiplexServer server(&loop, listen_addr, backend_addr, thread_count);
  server.Start();
  loop.Loop();
}
