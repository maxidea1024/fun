#include "fun/net/tcp_server.h"

#include "fun/base/atomic.h"
#include "fun/base/file_util.h"
#include "fun/base/logging.h"
#include "fun/base/process_info.h"
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
      : server_(loop, listen_addr, "EchoServer"),
        start_time_(Timestamp::Now()) {
    server_.SetConnectionCallback(
        boost::bind(&EchoServer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&EchoServer::OnMessage, this, _1, _2, _3));
    server_.SetThreadCount(g_thread_count);
    loop->ScheduleEvery(5.0, boost::bind(&EchoServer::PrintThroughput, this));
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

    if (conn->IsConnected()) {
      connections_.increment();
    } else {
      connections_.decrement();
    }
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, const Timestamp&) {
    size_t len = buf->GetReadableLength();
    transferred_bytes_.addAndGet(len);
    received_messages_.IncrementAndGet();
    conn->Send(buf);
  }

  void PrintThroughput() {
    Timestamp end_time = Timestamp::Now();
    double bytes = static_cast<double>(transferred_bytes_.getAndSet(0));
    int msgs = received_messages_.getAndSet(0);
    double bytesPerMsg = msgs > 0 ? bytes / msgs : 0;
    double time = TimeDifference(end_time, start_time_);

    printf("%.3f MiB/s %.2f Kilo Msgs/s %.2f bytes per msg, ",
           bytes / time / 1024 / 1024, static_cast<double>(msgs) / time / 1000,
           bytesPerMsg);

    printConnection();
    fflush(stdout);
    start_time_ = end_time;
  }

  void printConnection() {
    String procStatus = ProcessInfo::procStatus();
    printf("%d conn, files %d , VmSize %ld KiB, RSS %ld KiB, ",
           connections_.get(), ProcessInfo::openedFiles(),
           getLong(procStatus, "VmSize:"), getLong(procStatus, "VmRSS:"));

    String meminfo;
    FileUtil::readFile("/proc/meminfo", 65536, &meminfo);
    long total_kb = getLong(meminfo, "MemTotal:");
    long free_kb = getLong(meminfo, "MemFree:");
    long buffers_kb = getLong(meminfo, "Buffers:");
    long cached_kb = getLong(meminfo, "Cached:");
    printf("system memory used %ld KiB\n",
           total_kb - free_kb - buffers_kb - cached_kb);
  }

  long getLong(const String& procStatus, const char* key) {
    long result = 0;
    size_t pos = procStatus.find(key);
    if (pos != String::npos) {
      result = ::atol(procStatus.c_str() + pos + strlen(key));
    }
    return result;
  }

  TcpServer server_;
  AtomicInt32 connections_;
  AtomicInt32 received_messages_;
  AtomicInt64 transferred_bytes_;
  Timestamp start_time_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid()
           << ", tid = " << Thread::CurrentTid()
           << ", max files = " << ProcessInfo::maxOpenFiles();
  Logger::SetLogLevel(Logger::WARN);
  if (argc > 1) {
    g_thread_count = atoi(argv[1]);
  }

  EventLoop loop;
  InetAddress listen_addr(2007);
  EchoServer server(&loop, listen_addr);

  server.Start();

  loop.Loop();
}
