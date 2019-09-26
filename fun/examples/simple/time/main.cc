#include "time.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <unistd.h>

using namespace fun;
using namespace fun::net;

int main() {
  LOG_INFO << "pid = " << Process::CurrentPid();
  EventLoop loop;
  InetAddress listen_addr(2037);
  TimeServer server(&loop, listen_addr);
  server.Start();
  loop.Loop();
}
