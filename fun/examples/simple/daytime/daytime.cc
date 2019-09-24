#include "daytime.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <boost/bind.hpp>

using namespace fun;
using namespace fun::net;

DaytimeServer::DaytimeServer(EventLoop* loop,
                             const InetAddress& listen_addr)
  : server_(loop, listen_addr, "DaytimeServer") {
  server_.SetConnectionCallback(boost::bind(&DaytimeServer::OnConnection, this, _1));
  server_.SetMessageCallback(boost::bind(&DaytimeServer::OnMessage, this, _1, _2, _3));
}

void DaytimeServer::Start() {
  server_.Start();
}

void DaytimeServer::OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "DaytimeServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");
  if (conn->IsConnected()) {
    conn->Send(Timestamp::Now().ToFormattedString() + "\n");
    conn->Shutdown();
  }
}

void DaytimeServer::OnMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              const Timestamp& time) {
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " discards " << msg.size()
           << " bytes received at " << time.ToString();
}
