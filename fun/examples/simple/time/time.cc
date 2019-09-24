#include "time.h"

#include "fun/base/logging.h"
#include <red/net/Endian.h>

#include <boost/bind.hpp>

using namespace fun;
using namespace fun::net;

TimeServer::TimeServer( fun::net::EventLoop* loop,
                        const fun::net::InetAddress& listen_addr)
  : server_(loop, listen_addr, "TimeServer") {
  server_.SetConnectionCallback(boost::bind(&TimeServer::OnConnection, this, _1));
  server_.SetMessageCallback(boost::bind(&TimeServer::OnMessage, this, _1, _2, _3));
}

void TimeServer::Start() {
  server_.Start();
}

void TimeServer::OnConnection(const fun::net::TcpConnectionPtr& conn) {
  LOG_INFO << "TimeServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");
  if (conn->IsConnected()) {
    time_t now = ::time(NULL);
    int32_t be32 = sockets::hostToNetwork32(static_cast<int32_t>(now));
    conn->Send(&be32, sizeof be32);
    conn->Shutdown();
  }
}

void TimeServer::OnMessage(const fun::net::TcpConnectionPtr& conn,
                 fun::net::Buffer* buf,
                 const fun::Timestamp& time) {
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " discards " << msg.size()
           << " bytes received at " << time.ToString();
}
