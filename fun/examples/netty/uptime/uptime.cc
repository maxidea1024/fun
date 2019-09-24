#include "fun/net/tcp_client.h"

#include "fun/base/logging.h"
#include "fun/base/thread.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class UptimeClient : Noncopyable {
 public:
  UptimeClient(EventLoop* loop, const InetAddress& listen_addr)
    : client_(loop, listen_addr, "UptimeClient") {
    client_.SetConnectionCallback(boost::bind(&UptimeClient::OnConnection, this, _1));
    client_.SetMessageCallback(boost::bind(&UptimeClient::OnMessage, this, _1, _2, _3));

    //client_.EnableRetry();
  }

  void Connect() {
    client_.Connect();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->GetLocalAddress().ToIpPort() << " -> "
              << conn->GetPeerAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");
  }

  void OnMessage( const TcpConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp& time) {
  }

  TcpClient client_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid() << ", tid = " << Thread::CurrentTid();

  if (argc > 2) {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(argv[1], port);

    UptimeClient client(&loop, server_addr);
    client.Connect();
    loop.Loop();
  }
  else {
    printf("Usage: %s host_ip port\n", argv[0]);
  }
}
