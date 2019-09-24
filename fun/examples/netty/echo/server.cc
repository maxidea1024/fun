#include "fun/net/tcp_server.h"

#include "fun/base/atomic.h"
#include "fun/base/logging.h"
#include "fun/base/thread.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

int g_thread_count = 0;

class EchoServer {
 public:
  EchoServer(EventLoop* loop, const InetAddress& listen_addr)
    : server_(loop, listen_addr, "EchoServer")
    , old_counter_(0)
    , start_time_(Timestamp::Now()) {
    server_.SetConnectionCallback(boost::bind(&EchoServer::OnConnection, this, _1));
    server_.SetMessageCallback(boost::bind(&EchoServer::OnMessage, this, _1, _2, _3));
    server_.SetThreadCount(g_thread_count);

    //매 3초마다 throughput을 측정하고 출력함.
    loop->ScheduleEvery(3.0, boost::bind(&EchoServer::PrintThroughput, this));
  }

  void Start() {
    LOG_INFO << "starting " << g_thread_count << " threads.";

    server_.Start();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->GetPeerAddress().ToIpPort() << " -> "
        << conn->GetLocalAddress().ToIpPort() << " is "
        << (conn->IsConnected() ? "UP" : "DOWN");

    conn->SetTcpNoDelay(true);
  }

  void OnMessage( const TcpConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp&) {
    size_t len = buf->GetReadableLength();
    transferred_.addAndGet(len);
    received_messages_.IncrementAndGet();
    conn->Send(buf);
  }

  void PrintThroughput() {
    Timestamp end_time = Timestamp::Now();
    int64_t new_counter = transferred_.get();
    int64_t bytes = new_counter - old_counter_;
    int64_t msgs = received_messages_.getAndSet(0);
    double time = TimeDifference(end_time, start_time_);
    printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
        static_cast<double>(bytes)/time/1024/1024,
        static_cast<double>(msgs)/time/1024,
        static_cast<double>(bytes)/static_cast<double>(msgs));

    old_counter_ = new_counter;
    start_time_ = end_time;
  }

  TcpServer server_;
  AtomicInt64 transferred_;
  AtomicInt64 received_messages_;
  int64_t old_counter_;
  Timestamp start_time_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid() << ", tid = " << Thread::CurrentTid();
  if (argc > 1) {
    g_thread_count = atoi(argv[1]);
  }

  EventLoop loop;
  InetAddress listen_addr(2007);
  EchoServer server(&loop, listen_addr);
  server.Start();
  loop.Loop();
}
