#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

using namespace fun;
using namespace fun::net;

int main() {
  EventLoop loop;
  TcpServer server(&loop, InetAddress(1079), "Finger");
  server.Start();
  loop.Loop();
}
