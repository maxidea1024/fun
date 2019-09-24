#include "codec.h"

#include "fun/base/logging.h"
#include "fun/base/mutex.h"
#include "fun/net/event_loop_thread.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace fun;
using namespace fun::net;

/**
TODO
*/
class ChatClient : Noncopyable {
 public:
  ChatClient(EventLoop* loop, const InetAddress& server_addr)
      : tcp_client_(loop, server_addr, "ChatClient"),
        codec_(boost::bind(&ChatClient::OnStringMessage, this, _1, _2, _3)) {
    tcp_client_.SetConnectionCallback(
        boost::bind(&ChatClient::OnConnection, this, _1));
    tcp_client_.SetMessageCallback(
        boost::bind(&LengthHeaderCodec::OnMessage, &codec_, _1, _2, _3));

    tcp_client_.EnableRetry();
  }

  void Connect() { tcp_client_.Connect(); }

  void Disconnect() { tcp_client_.Disconnect(); }

  void Write(const StringPiece& message) {
    ScopedLock guard(mutex_);
    if (connection_) {
      codec_.Send(get_pointer(connection_), message);
    }
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");

    ScopedLock guard(mutex_);
    if (conn->IsConnected()) {
      connection_ = conn;
    } else {
      connection_.Reset();
    }
  }

  void OnStringMessage(const TcpConnectionPtr&, const String& message,
                       const Timestamp&) {
    printf("<<< %s\n", message.c_str());
  }

  TcpClient tcp_client_;
  LengthHeaderCodec codec_;
  MutexLock mutex_;
  TcpConnectionPtr connection_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc > 2) {
    EventLoopThread loop_thread;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(argv[1], port);

    ChatClient client(loop_thread.StartLoop(), server_addr);
    client.Connect();
    String line;
    while (std::getline(std::cin, line)) {
      client.Write(line);
    }
    client.Disconnect();
    CurrentThread::sleepUsec(
        1000 * 1000);  // wait for Disconnect, see ace/logging/client.cc
  } else {
    printf("Usage: %s host_ip port\n", argv[0]);
  }
}
