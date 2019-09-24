#include "echo.h"

#include "fun/base/logging.h"

#include <boost/bind.hpp>

// using namespace fun;
// using namespace fun::net;

EchoServer::EchoServer(fun::net::EventLoop* loop,
                       const fun::net::InetAddress& listen_addr)
  : server_(loop, listen_addr, "EchoServer") {
  server_.SetConnectionCallback(boost::bind(&EchoServer::OnConnection, this, _1));
  server_.SetMessageCallback(boost::bind(&EchoServer::OnMessage, this, _1, _2, _3));
}

void EchoServer::Start() {
  server_.Start();
}

void EchoServer::OnConnection(const fun::net::TcpConnectionPtr& conn) {
  LOG_INFO << "EchoServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");
}

void EchoServer::OnMessage(const fun::net::TcpConnectionPtr& conn,
                           fun::net::Buffer* buf,
                           const fun::Timestamp& time) {
  //TODO ReadAllAsStringRef()도 추가해주면 좋을듯... 복사가 불필요해지니..
  //단, lifecycle을 잘 관리해야함.
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " echo " << msg.size() << " bytes, "
           << "data received at " << time.ToString();
  conn->Send(msg);
}
