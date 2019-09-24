#include "fun/net/tcp_client.h"
#include "fun/base/logging.h"
#include "fun/base/thread.h"
#include "fun/net/event_loop.h"
#include "fun/net/event_loop_thread_pool.h"
#include "fun/net/inet_address.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class Client;

class Session : Noncopyable {
 public:
  Session(EventLoop* loop,
          const InetAddress& server_addr,
          const String& name,
          Client* owner)
    : client_(loop, server_addr, name)
    , owner_(owner)
    , bytes_readed_(0)
    , bytes_written_(0)
    , messages_readed_(0) {
    client_.SetConnectionCallback(boost::bind(&Session::OnConnection, this, _1));
    client_.SetMessageCallback(boost::bind(&Session::OnMessage, this, _1, _2, _3));
  }

  void Start() {
    client_.Connect();
  }

  void Stop() {
    client_.Disconnect();
  }

  int64_t GetBytesReaded() const {
     return bytes_readed_;
  }

  int64_t GetMessagesReaded() const {
     return messages_readed_;
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn);

  void OnMessage( const TcpConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp& received_time) {
    ++messages_readed_;
    bytes_readed_ += buf->GetReadableLength();
    bytes_written_ += buf->GetReadableLength();
    conn->Send(buf);
  }

  TcpClient client_;
  Client* owner_;
  int64_t bytes_readed_;
  int64_t bytes_written_;
  int64_t messages_readed_;
};

class Client : Noncopyable {
 public:
  Client(EventLoop* loop,
         const InetAddress& server_addr,
         int block_size,
         int session_count,
         int timeout,
         int thread_count)
    : loop_(loop)
    , thread_pool_(loop, "pingpong-client")
    , session_count_(session_count)
    , timeout_(timeout) {
    loop->ScheduleAfter(timeout, boost::bind(&Client::HandleTimeout, this));

    if (thread_count > 1) {
      thread_pool_.SetThreadCount(thread_count);
    }
    thread_pool_.Start();

    for (int i = 0; i < block_size; ++i) {
      message_.push_back(static_cast<char>(i % 128));
    }

    for (int i = 0; i < session_count; ++i) {
      char buf[32];
      snprintf(buf, sizeof buf, "C%05d", i);

      Session* session = new Session(thread_pool_.GetNextLoop(), server_addr, buf, this);
      session->Start();
      sessions_.push_back(session);
    }
  }

  const String& GetMessage() const {
    return message_;
  }

  void OnConnect() {
    if (connected_count_.IncrementAndGet() == session_count_) {
      LOG_WARN << "all connected";
    }
  }

  void OnDisconnect(const TcpConnectionPtr& conn) {
    if (connected_count_.decrementAndGet() == 0) {
      LOG_WARN << "all disconnected";

      int64_t total_bytes_readed = 0;
      int64_t total_messages_readed = 0;
      for (boost::ptr_vector<Session>::iterator it = sessions_.begin();
          it != sessions_.end(); ++it) {
        total_bytes_readed += it->GetBytesReaded();
        total_messages_readed += it->GetMessagesReaded();
      }
      LOG_WARN << total_bytes_readed << " total bytes read";
      LOG_WARN << total_messages_readed << " total messages read";
      LOG_WARN << static_cast<double>(total_bytes_readed) / static_cast<double>(total_messages_readed)
               << " average message size";
      LOG_WARN << static_cast<double>(total_bytes_readed) / (timeout_ * 1024 * 1024)
               << " MiB/s throughput";
      conn->GetLoop()->QueueInLoop(boost::bind(&Client::Quit, this));
    }
  }

 private:
  void Quit() {
    loop_->QueueInLoop(boost::bind(&EventLoop::Quit, loop_));
  }

  void HandleTimeout() {
    LOG_WARN << "Stop";
    std::for_each(sessions_.begin(), sessions_.end(),
                  boost::mem_fn(&Session::Stop));
  }

  EventLoop* loop_;
  EventLoopThreadPool thread_pool_;
  int session_count_;
  int timeout_;
  boost::ptr_vector<Session> sessions_;
  String message_;
  AtomicInt32 connected_count_;
};

void Session::OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(true);
    conn->Send(owner_->GetMessage());
    owner_->OnConnect();
  }
  else {
    owner_->OnDisconnect(conn);
  }
}

int main(int argc, char* argv[]) {
  if (argc != 7) {
    fprintf(stderr, "Usage: client <host_ip> <port> <threads> <blocksize> ");
    fprintf(stderr, "<sessions> <time>\n");
  }
  else {
    LOG_INFO << "pid = " << Process::CurrentPid() << ", tid = " << Thread::CurrentTid();
    Logger::SetLogLevel(Logger::WARN);

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int thread_count = atoi(argv[3]);
    int block_size = atoi(argv[4]);
    int session_count = atoi(argv[5]);
    int timeout = atoi(argv[6]);

    EventLoop loop;
    InetAddress server_addr(ip, port);
    Client client(&loop, server_addr, block_size, session_count, timeout, thread_count);
    loop.Loop();
  }
}
