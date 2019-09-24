#include "codec.h"

#include "fun/base/logging.h"
#include "fun/base/mutex.h"
#include "fun/base/thread_local_singleton.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <stdio.h>
#include <unistd.h>
#include <set>

using namespace fun;
using namespace fun::net;

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

  void Start() {
    server_.SetThreadInitCallback(
        boost::bind(&ChatServer::ThreadInit, this, _1));
    server_.Start();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      LocalConnections::instance().insert(conn);
    } else {
      LocalConnections::instance().erase(conn);
    }
  }

  void OnStringMessage(const TcpConnectionPtr&, const String& message,
                       const Timestamp&) {
    EventLoop::Functor f =
        boost::bind(&ChatServer::DistributeMessage, this, message);
    LOG_DEBUG;

    ScopedLock guard(mutex_);
    for (std::set<EventLoop*>::iterator it = loops_.begin(); it != loops_.end();
         ++it) {
      (*it)->QueueInLoop(f);
    }
    LOG_DEBUG;
  }

  typedef std::set<TcpConnectionPtr> ConnectionList;

  void DistributeMessage(const String& message) {
    LOG_DEBUG << "begin";
    for (ConnectionList::iterator it = LocalConnections::instance().begin();
         it != LocalConnections::instance().end(); ++it) {
      codec_.Send(get_pointer(*it), message);
    }
    LOG_DEBUG << "end";
  }

  void ThreadInit(EventLoop* loop) {
    fun_check(LocalConnections::pointer() == NULL);
    LocalConnections::Get();  // singleton??
    fun_check(LocalConnections::pointer() != NULL);
    ScopedLock guard(mutex_);
    loops_.insert(loop);
  }

  TcpServer server_;
  LengthHeaderCodec codec_;
  typedef ThreadLocalSingleton<ConnectionList> LocalConnections;

  MutexLock mutex_;
  std::set<EventLoop*> loops_;
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
