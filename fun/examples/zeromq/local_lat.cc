#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include "examples/asio/chat/codec.h"

#include <boost/bind.hpp>
#include <stdio.h>
#include <unistd.h>

bool g_tcp_no_delay = false;

void OnConnection(const fun::net::TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(g_tcp_no_delay);
  }
}

void OnStringMessage(LengthHeaderCodec* codec,
                     const fun::net::TcpConnectionPtr& conn,
                     const String& message,
                     const fun::Timestamp&) {
  codec->Send(get_pointer(conn), message);
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    g_tcp_no_delay = argc > 2 ? atoi(argv[2]) : 0;
    int thread_count = argc > 3 ? atoi(argv[3]) : 0;

    LOG_INFO << "pid = " << Process::CurrentPid() << ", listen port = " << port;

    // fun::Logger::SetLogLevel(fun::Logger::WARN);
    fun::net::EventLoop loop;
    fun::net::InetAddress listen_addr(port);
    fun::net::TcpServer server(&loop, listen_addr, "PingPong");
    LengthHeaderCodec codec(boost::bind(OnStringMessage, &codec, _1, _2, _3));

    server.SetConnectionCallback(OnConnection);
    server.SetMessageCallback(boost::bind(&LengthHeaderCodec::OnMessage, &codec, _1, _2, _3));

    if (thread_count > 1) {
      server.SetThreadCount(thread_count);
    }

    server.Start();
    loop.Loop();
  }
  else {
    fprintf(stderr, "Usage: %s listen_port [tcp_no_delay [threads]]\n", argv[0]);
  }
}
