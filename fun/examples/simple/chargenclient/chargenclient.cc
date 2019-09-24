#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class ChargenClient : Noncopyable {
 public:
  ChargenClient(EventLoop* loop, const InetAddress& listen_addr)
    : loop_(loop)
    , client_(loop, listen_addr, "ChargenClient") {
    client_.SetConnectionCallback(boost::bind(&ChargenClient::OnConnection, this, _1));
    client_.SetMessageCallback(boost::bind(&ChargenClient::OnMessage, this, _1, _2, _3));

    // client_.EnableRetry();
  }

  void Connect() {
    client_.Connect();
  }

 private:
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
    buf->DrainAll();
  }

  EventLoop* loop_;
  TcpClient client_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc > 1) {
    EventLoop loop;
    InetAddress server_addr(argv[1], 2019);

    ChargenClient chargen_client(&loop, server_addr);
    chargen_client.Connect();
    loop.Loop();
  }
  else {
    printf("Usage: %s host_ip\n", argv[0]);
  }
}
