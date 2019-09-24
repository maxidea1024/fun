#include "fun/net/tcp_server.h"

#include <red/base/Atomic.h>
#include <red/base/Thread.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

int thread_count = 0;

class DiscardServer {
 public:
  DiscardServer(EventLoop* loop, const InetAddress& listen_addr)
      : server_(loop, listen_addr, "DiscardServer"),
        old_counter_(0),
        start_time_(Timestamp::Now()) {
    server_.SetConnectionCallback(
        boost::bind(&DiscardServer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&DiscardServer::OnMessage, this, _1, _2, _3));
    server_.SetThreadCount(thread_count);
    loop->ScheduleEvery(3.0,
                        boost::bind(&DiscardServer::PrintThroughput, this));
  }

  void Start() {
    LOG_INFO << "starting " << thread_count << " threads.";
    server_.Start();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->GetPeerAddress().ToIpPort() << " -> "
              << conn->GetLocalAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, const Timestamp&) {
    size_t len = buf->GetReadableLength();
    transferred_.add(len);
    received_messages_.IncrementAndGet();
    buf->DrainAll();
  }

  void PrintThroughput() {
    Timestamp end_time = Timestamp::Now();

    int64_t new_counter = transferred_.get();
    int64_t transfered_bytes = new_counter - old_counter_;
    int64_t received_msg_count = received_messages_.getAndSet(0);
    double time = TimeDifference(end_time, start_time_);

    printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
           static_cast<double>(transfered_bytes) / time / 1024 / 1024,
           static_cast<double>(received_msg_count) / time / 1024,
           static_cast<double>(transfered_bytes) /
               static_cast<double>(received_msg_count));

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
  LOG_INFO << "pid = " << Process::CurrentPid()
           << ", tid = " << Thread::CurrentTid();

  if (argc > 1) {
    thread_count = atoi(argv[1]);
  }

  EventLoop loop;
  InetAddress listen_addr(2009);
  DiscardServer server(&loop, listen_addr);
  server.Start();
  loop.Loop();
}
