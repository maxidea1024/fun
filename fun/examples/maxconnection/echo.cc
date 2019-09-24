#include "echo.h"

#include "fun/base/logging.h"

#include <boost/bind.hpp>

using namespace fun;
using namespace fun::net;

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listen_addr,
                       int max_connections)
  : server_(loop, listen_addr, "EchoServer")
  , connected_count_(0)
  , MAX_CONNECTIONS(max_connections) {
  server_.SetConnectionCallback(boost::bind(&EchoServer::OnConnection, this, _1));
  server_.SetMessageCallback(boost::bind(&EchoServer::OnMessage, this, _1, _2, _3));
}

void EchoServer::Start() {
  server_.Start();
}

void EchoServer::OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "EchoServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    ++connected_count_;
    if (connected_count_ > MAX_CONNECTIONS) {
      conn->Shutdown();
      conn->ForceCloseWithDelay(3.0);  // > round trip of the whole Internet.
    }
  }
  else {
    --connected_count_;
  }
  LOG_INFO << "numConnected = " << connected_count_;
}

void EchoServer::OnMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           const Timestamp& time) {
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " echo " << msg.size() << " bytes at " << time.ToString();
  conn->Send(msg);
}
