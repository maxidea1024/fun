#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

using namespace fun;
using namespace fun::net;

void OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    conn->Shutdown();
  }
}

int main() {
  EventLoop loop;
  TcpServer server(&loop, InetAddress(1079), "Finger");
  server.SetConnectionCallback(OnConnection);
  server.Start();
  loop.Loop();
}
