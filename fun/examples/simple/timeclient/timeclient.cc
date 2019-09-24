#include "fun/base/logging.h"
#include <red/net/Endian.h>
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class TimeClient : Noncopyable {
 public:
  TimeClient(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop)
    , client_(loop, server_addr, "TimeClient") {
    client_.SetConnectionCallback(boost::bind(&TimeClient::OnConnection, this, _1));
    client_.SetMessageCallback(boost::bind(&TimeClient::OnMessage, this, _1, _2, _3));

    // client_.EnableRetry();
  }

  void Connect() {
    client_.Connect();
  }

 private:
  EventLoop* loop_;
  TcpClient client_;

  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");

    if (!conn->IsConnected()) {
      loop_->Quit();
    }
  }

  void OnMessage( const TcpConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp& received_time) {
    if (buf->GetReadableLength() >= sizeof(int32_t)) {
      const void* data = buf->GetReadablePtr();
      int32_t be32 = *static_cast<const int32_t*>(data);
      buf->Drain(sizeof(int32_t));
      time_t time = sockets::networkToHost32(be32);
      Timestamp ts(ImplicitCast<uint64_t>(time) * Timestamp::kMicroSecondsPerSecond);
      LOG_INFO << "Server time = " << time << ", " << ts.ToFormattedString();
    }
    else {
      LOG_INFO << conn->GetName() << " no enough data " << buf->GetReadableLength()
               << " at " << received_time.ToFormattedString();
    }
  }
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();
  if (argc > 1) {
    EventLoop loop;
    InetAddress server_addr(argv[1], 2037);

    TimeClient time_client(&loop, server_addr);
    timeClient.Connect();
    loop.Loop();
  }
  else {
    printf("Usage: %s host_ip\n", argv[0]);
  }
}
