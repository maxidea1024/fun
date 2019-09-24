#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"

#include "examples/asio/chat/codec.h"

#include <stdio.h>
#include <boost/bind.hpp>

bool g_tcp_no_delay = false;
int g_msg_size = 0;
int g_total_msgs = 0;
int g_msg_count = 0;
String g_message;
fun::Timestamp g_start;

void OnConnection(LengthHeaderCodec* codec,
                  const fun::net::TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    LOG_INFO << "connected";
    g_start = fun::Timestamp::Now();
    conn->SetTcpNoDelay(g_tcp_no_delay);
    codec->Send(get_pointer(conn), g_message);
  } else {
    LOG_INFO << "disconnected";
    fun::net::EventLoop::GetEventLoopOfCurrentThread()->Quit();
  }
}

void OnStringMessage(LengthHeaderCodec* codec,
                     const fun::net::TcpConnectionPtr& conn,
                     const String& message, const fun::Timestamp&) {
  if (message.size() != static_cast<size_t>(g_msg_size)) {
    abort();
  }

  ++g_msg_count;

  if (g_msg_count < g_total_msgs) {
    codec->Send(get_pointer(conn), message);
  } else {
    fun::Timestamp end = fun::Timestamp::Now();
    LOG_INFO << "done";
    double elapsed = TimeDifference(end, g_start);
    LOG_INFO << g_msg_size << " message bytes";
    LOG_INFO << g_msg_count << " round-trips";
    LOG_INFO << elapsed << " seconds";
    LOG_INFO << fun::Fmt("%.3f", g_msg_count / elapsed)
             << " round-trips per second";
    LOG_INFO << fun::Fmt("%.3f", (1000000 * elapsed / g_msg_count / 2))
             << " latency [us]";
    LOG_INFO << fun::Fmt("%.3f",
                         (g_msg_size * g_msg_count / elapsed / 1024 / 1024))
             << " band width [MiB/s]";
    conn->Shutdown();
  }
}

int main(int argc, char* argv[]) {
  if (argc > 3) {
    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    g_msg_size = atoi(argv[3]);
    g_message.Assign(g_msg_size, 'H');
    g_total_msgs = argc > 4 ? atoi(argv[4]) : 10000;
    g_tcp_no_delay = argc > 5 ? atoi(argv[5]) : 0;

    fun::net::EventLoop loop;
    fun::net::InetAddress server_addr(ip, port);
    fun::net::TcpClient client(&loop, server_addr, "Client");
    LengthHeaderCodec codec(boost::bind(OnStringMessage, &codec, _1, _2, _3));
    client.SetConnectionCallback(boost::bind(OnConnection, &codec, _1));
    client.SetMessageCallback(
        boost::bind(&LengthHeaderCodec::OnMessage, &codec, _1, _2, _3));
    client.Connect();
    loop.Loop();
  } else {
    fprintf(stderr, "Usage: %s server_ip server_port msg_size", argv[0]);
    fprintf(stderr, " [msg_count [tcp_no_delay]]\n");
  }
}
