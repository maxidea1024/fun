#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <list>
#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

// RFC 862
class EchoServer {
 public:
  EchoServer(EventLoop* loop,
             const InetAddress& listen_addr,
             int idle_seconds);

  void Start() {
    server_.Start();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn);

  void OnMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 const Timestamp& time);

  void OnTimer();

  void DumpConnectionList() const;

  typedef WeakPtr<TcpConnection> WeakTcpConnectionPtr;
  typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;

  struct Node : public fun::copyable {
    Timestamp last_received_time;
    WeakConnectionList::iterator position;
  };

  TcpServer server_;
  int idle_seconds_;
  WeakConnectionList connection_list_;
};

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listen_addr,
                       int idle_seconds)
  : server_(loop, listen_addr, "EchoServer")
  , idle_seconds_(idle_seconds) {
  server_.SetConnectionCallback(
      boost::bind(&EchoServer::OnConnection, this, _1));
  server_.SetMessageCallback(
      boost::bind(&EchoServer::OnMessage, this, _1, _2, _3));
  loop->ScheduleEvery(1.0, boost::bind(&EchoServer::OnTimer, this));
  DumpConnectionList();
}

void EchoServer::OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "EchoServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    Node node;
    node.last_received_time = Timestamp::Now();
    connection_list_.Add(conn);
    node.position = --connection_list_.end();
    conn->SetContext(node);
  }
  else {
    fun_check(!conn->GetContext().empty());
    const Node& node = boost::any_cast<const Node&>(conn->GetContext());
    connection_list_.erase(node.position);
  }

  DumpConnectionList();
}

void EchoServer::OnMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           const Timestamp& time) {
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " echo " << msg.size()
           << " bytes at " << time.ToString();
  conn->Send(msg);

  fun_check(!conn->GetContext().empty());
  Node* node = boost::any_cast<Node>(conn->getMutableContext());
  node->last_received_time = time;
  connection_list_.splice(connection_list_.end(), connection_list_, node->position);
  fun_check(node->position == --connection_list_.end());

  DumpConnectionList();
}

void EchoServer::OnTimer() {
  DumpConnectionList();
  Timestamp now = Timestamp::Now();
  for (WeakConnectionList::iterator it = connection_list_.begin();
      it != connection_list_.end();) {
    TcpConnectionPtr conn = it->lock();
    if (conn) {
      Node* n = boost::any_cast<Node>(conn->getMutableContext());
      double age = TimeDifference(now, n->last_received_time);
      if (age > idle_seconds_) {
        if (conn->IsConnected()) {
          conn->Shutdown();
          LOG_INFO << "shutting down " << conn->GetName();
          conn->ForceCloseWithDelay(3.5);  // > round trip of the whole Internet.
        }
      }
      else if (age < 0) {
        LOG_WARN << "Time jump";
        n->last_received_time = now;
      }
      else {
        break;
      }
      ++it;
    }
    else {
      LOG_WARN << "Expired";
      it = connection_list_.erase(it);
    }
  }
}

void EchoServer::DumpConnectionList() const {
  LOG_INFO << "size = " << connection_list_.size();

  for (WeakConnectionList::const_iterator it = connection_list_.begin();
      it != connection_list_.end(); ++it) {
    TcpConnectionPtr conn = it->lock();
    if (conn) {
      printf("conn %p\n", get_pointer(conn));
      const Node& n = boost::any_cast<const Node&>(conn->GetContext());
      printf("    time %s\n", n.last_received_time.ToString().c_str());
    }
    else {
      printf("expired\n");
    }
  }
}

int main(int argc, char* argv[]) {
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
