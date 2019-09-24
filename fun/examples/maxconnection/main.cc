#include "echo.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <unistd.h>

using namespace fun;
using namespace fun::net;

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();
  EventLoop loop;
  InetAddress listen_addr(2007);
  int max_connections = 5;
  if (argc > 1) {
    max_connections = atoi(argv[1]);
  }
  LOG_INFO << "max_connections = " << max_connections;
  EchoServer server(&loop, listen_addr, max_connections);
  server.Start();
  loop.Loop();
}
