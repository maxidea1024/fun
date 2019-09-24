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

void OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(true);
  }
}

void OnMessage( const TcpConnectionPtr& conn,
                Buffer* buf,
                const Timestamp& received_time) {
  conn->Send(buf);
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: server <address> <port> <threads>\n");
  }
  else {
    LOG_INFO << "pid = " << Process::CurrentPid() << ", tid = " << Thread::CurrentTid();

    Logger::SetLogLevel(Logger::WARN);

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress listen_addr(ip, port);
    int thread_count = atoi(argv[3]);

    EventLoop loop;

    TcpServer server(&loop, listen_addr, "PingPong");

    server.SetConnectionCallback(OnConnection);
    server.SetMessageCallback(OnMessage);

    if (thread_count > 1) {
      server.SetThreadCount(thread_count);
    }

    server.Start();
    loop.Loop();
  }
}
