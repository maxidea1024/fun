#include "echo.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <unistd.h>

// using namespace fun;
// using namespace fun::net;

int main() {
  LOG_INFO << "pid = " << Process::CurrentPid();
  fun::net::EventLoop loop;
  fun::net::InetAddress listen_addr(2007);
  EchoServer server(&loop, listen_addr);
  server.Start();
  loop.Loop();
}
