#include "echo.h"

#include <stdio.h>

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

using namespace fun;
using namespace fun::net;

void TestHash() {
  boost::hash<fun::SharedPtr<int> > h;
  fun::SharedPtr<int> x1(new int(10));
  fun::SharedPtr<int> x2(new int(10));
  h(x1);
  fun_check(h(x1) != h(x2));
  x1 = x2;
  fun_check(h(x1) == h(x2));
  x1.Reset();
  fun_check(h(x1) != h(x2));
  x2.Reset();
  fun_check(h(x1) == h(x2));
}

int main(int argc, char* argv[]) {
  TestHash();

  EventLoop loop;
  InetAddress listen_addr(2007);
  int idle_seconds = 10;
  if (argc > 1) {
    idle_seconds = atoi(argv[1]);
  }
  LOG_INFO << "pid = " << Process::CurrentPid() << ", idle seconds = " << idle_seconds;
  EchoServer server(&loop, listen_addr, idle_seconds);
  server.Start();
  loop.Loop();
}
