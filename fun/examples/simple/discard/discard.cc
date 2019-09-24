#include "discard.h"

#include "fun/base/logging.h"

#include <boost/bind.hpp>

using namespace fun;
using namespace fun::net;

DiscardServer::DiscardServer(EventLoop* loop,
                             const InetAddress& listen_addr)
  : server_(loop, listen_addr, "DiscardServer") {
  server_.SetConnectionCallback(boost::bind(&DiscardServer::OnConnection, this, _1));
  server_.SetMessageCallback(boost::bind(&DiscardServer::OnMessage, this, _1, _2, _3));
}

void DiscardServer::Start() {
  server_.Start();
}

void DiscardServer::OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "DiscardServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");
}

void DiscardServer::OnMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              const Timestamp& time) {
  //TODO ReadAllAsStringRef()
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " discards " << msg.size()
           << " bytes received at " << time.ToString();
}
