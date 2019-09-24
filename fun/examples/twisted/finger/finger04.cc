#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

using namespace fun;
using namespace fun::net;

void OnMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               const Timestamp& received_time) {
  if (buf->FindCRLF()) {
    conn->Shutdown();
  }
}

int main() {
  EventLoop loop;
  TcpServer server(&loop, InetAddress(1079), "Finger");
  server.SetMessageCallback(OnMessage);
  server.Start();
  loop.Loop();
}
