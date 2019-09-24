#include "../chargen/chargen.h"
#include "../daytime/daytime.h"
#include "../discard/discard.h"
#include "../echo/echo.h"
#include "../time/time.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <unistd.h>

using namespace fun;
using namespace fun::net;

int main() {
  LOG_INFO << "pid = " << Process::CurrentPid();

  EventLoop loop;  // one loop shared by multiple servers

  ChargenServer chargen_server(&loop, InetAddress(2019));
  chargen_server.Start();

  DaytimeServer daytime_server(&loop, InetAddress(2013));
  daytime_server.Start();

  DiscardServer discard_server(&loop, InetAddress(2009));
  discard_server.Start();

  EchoServer echo_server(&loop, InetAddress(2007));
  echo_server.Start();

  TimeServer time_server(&loop, InetAddress(2037));
  time_server.Start();

  loop.Loop();
}
