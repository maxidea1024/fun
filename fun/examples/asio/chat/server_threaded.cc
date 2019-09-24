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
      : server_(loop, listen_addr, "ChatServer"),
        codec_(boost::bind(&ChatServer::OnStringMessage, this, _1, _2, _3)) {
    server_.SetConnectionCallback(
        boost::bind(&ChatServer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&LengthHeaderCodec::OnMessage, &codec_, _1, _2, _3));
  }

  void SetThreadCount(int thread_count) {
    server_.SetThreadCount(thread_count);
  }

  void Start() { server_.Start(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");

    ScopedLock guard(mutex_);
    if (conn->IsConnected()) {
      connections_.insert(conn);
    } else {
      connections_.erase(conn);
    }
  }

  void OnStringMessage(const TcpConnectionPtr&, const String& message,
                       const Timestamp&) {
    ScopedLock guard(mutex_);
    for (ConnectionList::iterator it = connections_.begin();
         it != connections_.end(); ++it) {
      codec_.Send(get_pointer(*it), message);
    }
  }

  typedef std::set<TcpConnectionPtr> ConnectionList;
  TcpServer server_;
  LengthHeaderCodec codec_;
  MutexLock mutex_;
  ConnectionList connections_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();
  if (argc > 1) {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress server_addr(port);
    ChatServer server(&loop, server_addr);
    if (argc > 2) {
      server.SetThreadCount(atoi(argv[2]));
    }
    server.Start();
    loop.Loop();
  } else {
    printf("Usage: %s port [thread_num]\n", argv[0]);
  }
}
