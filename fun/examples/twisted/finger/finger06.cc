#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <map>

using namespace fun;
using namespace fun::net;

typedef std::map<String, String> UserMap;
UserMap users;

String GetUser(const String& user) {
  String result = "No such user";
  UserMap::iterator it = users.find(user);
  if (it != users.end()) {
    result = it->second;
  }
  return result;
}

void OnMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               const Timestamp& received_time) {
  if (const char* crlf = buf->FindCRLF()) {
    String user(buf->GetReadablePtr(), crlf);
    conn->Send(GetUser(user) + "\r\n");
    buf->DrainUntil(crlf + 2);
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
