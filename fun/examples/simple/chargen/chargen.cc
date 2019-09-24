#include "chargen.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <stdio.h>
#include <boost/bind.hpp>

using namespace fun;
using namespace fun::net;

ChargenServer::ChargenServer(EventLoop* loop, const InetAddress& listen_addr,
                             bool print)
    : server_(loop, listen_addr, "ChargenServer"),
      transferred_(0),
      start_time_(Timestamp::Now()) {
  server_.SetConnectionCallback(
      boost::bind(&ChargenServer::OnConnection, this, _1));
  server_.SetMessageCallback(
      boost::bind(&ChargenServer::OnMessage, this, _1, _2, _3));
  server_.SetWriteCompleteCallback(
      boost::bind(&ChargenServer::OnWriteComplete, this, _1));

  if (print) {
    loop->ScheduleEvery(3.0,
                        boost::bind(&ChargenServer::PrintThroughput, this));
  }

  String line;
  for (int i = 33; i < 127; ++i) {
    line.push_back(char(i));
  }
  line += line;

  for (size_t i = 0; i < 127 - 33; ++i) {
    message_ += line.substr(i, 72) + '\n';
  }
}

void ChargenServer::Start() { server_.Start(); }

void ChargenServer::OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "ChargenServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");
  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(true);
    conn->Send(message_);
  }
}

void ChargenServer::OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                              const Timestamp& time) {
  // TODO ReadAllStringRef()
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " discards " << msg.size()
           << " bytes received at " << time.ToString();
}

void ChargenServer::OnWriteComplete(const TcpConnectionPtr& conn) {
  transferred_ += message_.size();
  conn->Send(message_);
}

void ChargenServer::PrintThroughput() {
  Timestamp end_time = Timestamp::Now();
  double time = TimeDifference(end_time, start_time_);
  printf("%4.3f MiB/s\n",
         static_cast<double>(transferred_) / time / 1024 / 1024);
  transferred_ = 0;
  start_time_ = end_time;
}
