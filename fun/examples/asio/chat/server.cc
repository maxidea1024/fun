#include "codec.h"

#include "fun/base/logging.h"
#include "fun/base/mutex.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>
#include <set>

using namespace fun;
using namespace fun::net;

/**
TODO
*/
class ChatServer : Noncopyable {
 public:
  ChatServer(EventLoop* loop, const InetAddress& listen_addr)
      : tcp_server_(loop, listen_addr, "ChatServer"),
        codec_(boost::bind(&ChatServer::OnStringMessage, this, _1, _2, _3)) {
    tcp_server_.SetConnectionCallback(
        boost::bind(&ChatServer::OnConnection, this, _1));
    tcp_server_.SetMessageCallback(
        boost::bind(&LengthHeaderCodec::OnMessage, &codec_, _1, _2, _3));
  }

  void Start() { tcp_server_.Start(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      connections_.insert(conn);
    } else {
      connections_.erase(conn);
    }
  }

  void OnStringMessage(const TcpConnectionPtr&, const String& message,
                       const Timestamp&) {
    for (ConnectionList::iterator it = connections_.begin();
         it != connections_.end(); ++it) {
      codec_.Send(get_pointer(*it), message);
    }
  }

  typedef std::set<TcpConnectionPtr> ConnectionList;
  TcpServer tcp_server_;
  LengthHeaderCodec codec_;
  ConnectionList connections_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc > 1) {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress server_addr(port);
    ChatServer server(&loop, server_addr);
    server.Start();
    loop.Loop();
  } else {
    printf("Usage: %s port\n", argv[0]);
  }
}
