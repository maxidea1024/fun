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

/**
 */
class EchoClient : Noncopyable {
 public:
  EchoClient(EventLoop* loop, const InetAddress& listen_addr, int size)
      : loop_(loop),
        client_(loop, listen_addr, "EchoClient"),
        message_(size, 'H') {
    client_.SetConnectionCallback(
        boost::bind(&EchoClient::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&EchoClient::OnMessage, this, _1, _2, _3));

    // client_.EnableRetry();
  }

  void Connect() { client_.Connect(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->GetLocalAddress().ToIpPort() << " -> "
              << conn->GetPeerAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      conn->SetTcpNoDelay(true);
      conn->Send(message_);
    } else {
      loop_->Quit();
    }
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 const Timestamp& time) {
    conn->Send(buf);
  }

  EventLoop* loop_;
  TcpClient client_;
  String message_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid()
           << ", tid = " << Thread::CurrentTid();

  if (argc > 1) {
    EventLoop loop;
    InetAddress server_addr(argv[1], 2007);

    int size = 256;
    if (argc > 2) {
      size = atoi(argv[2]);
    }

    EchoClient client(&loop, server_addr, size);
    client.Connect();
    // TODO client�� ������ ���ƾ߸� �ϴ°���...
    loop.Loop();
  } else {
    printf("Usage: %s host_ip [msg_size]\n", argv[0]);
  }
}
