#include "codec.h"

#include <red/net/EventLoopThreadPool.h>
#include "fun/base/atomic.h"
#include "fun/base/logging.h"
#include "fun/base/mutex.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

int g_connections = 0;
AtomicInt32 g_alive_connections;
AtomicInt32 g_message_received;
Timestamp g_start_time;
std::vector<Timestamp> g_received_time;
EventLoop* g_loop;
Function<void()> g_statistic;

/**
TODO
*/
class ChatClient : Noncopyable {
 public:
  ChatClient(EventLoop* loop, const InetAddress& server_addr)
      : loop_(loop),
        client_(loop, server_addr, "LoadTestClient"),
        codec_(boost::bind(&ChatClient::OnStringMessage, this, _1, _2, _3)) {
    client_.SetConnectionCallback(
        boost::bind(&ChatClient::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&LengthHeaderCodec::OnMessage, &codec_, _1, _2, _3));

    // client_.EnableRetry();
  }

  void Connect() { client_.Connect(); }

  void Disconnect() {
    // client_.Disconnect();
  }

  Timestamp GetReceivedTime() const { return received_time_; }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      connection_ = conn;
      if (g_alive_connections.IncrementAndGet() == g_connections) {
        LOG_INFO << "all connected";
        loop_->ScheduleAfter(10.0, boost::bind(&ChatClient::Send, this));
      }
    } else {
      connection_.Reset();
    }
  }

  void OnStringMessage(const TcpConnectionPtr&, const String& message,
                       const Timestamp&) {
    // printf("<<< %s\n", message.c_str());
    received_time_ = loop_->GetPollReturnTime();
    int received = g_message_received.IncrementAndGet();
    if (received == g_connections) {
      Timestamp end_time = Timestamp::Now();
      LOG_INFO << "all received " << g_connections << " in "
               << TimeDifference(end_time, g_start_time);
      g_loop->QueueInLoop(g_statistic);
    } else if (received % 1000 == 0) {
      LOG_DEBUG << received;
    }
  }

  void Send() {
    g_start_time = Timestamp::Now();
    codec_.Send(get_pointer(connection_), "hello");
    LOG_DEBUG << "sent";
  }

  EventLoop* loop_;
  TcpClient client_;
  LengthHeaderCodec codec_;
  TcpConnectionPtr connection_;
  Timestamp received_time_;
};

void statistic(const boost::ptr_vector<ChatClient>& clients) {
  LOG_INFO << "statistic " << clients.size();

  std::vector<double> seconds(clients.size());
  for (size_t i = 0; i < clients.size(); ++i) {
    seconds[i] = TimeDifference(clients[i].GetReceivedTime(), g_start_time);
  }

  std::sort(seconds.begin(), seconds.end());
  for (size_t i = 0; i < clients.size();
       i += std::max(static_cast<size_t>(1), clients.size() / 20)) {
    printf("%6zd%% %.6f\n", i * 100 / clients.size(), seconds[i]);
  }
  if (clients.size() >= 100) {
    printf("%6d%% %.6f\n", 99, seconds[clients.size() - clients.size() / 100]);
  }
  printf("%6d%% %.6f\n", 100, seconds.back());
}

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc > 3) {
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(argv[1], port);
    g_connections = atoi(argv[3]);
    int thread_count = 0;
    if (argc > 4) {
      thread_count = atoi(argv[4]);
    }

    EventLoop loop;
    g_loop = &loop;
    EventLoopThreadPool loop_pool(&loop, "chat-loadtest");
    loop_pool.SetThreadCount(thread_count);
    loop_pool.Start();

    g_received_time.reserve(g_connections);
    boost::ptr_vector<ChatClient> clients(g_connections);
    g_statistic = boost::bind(statistic, boost::ref(clients));

    for (int i = 0; i < g_connections; ++i) {
      clients.push_back(new ChatClient(loop_pool.GetNextLoop(), server_addr));
      clients[i].Connect();
      usleep(200);
    }

    loop.Loop();
    // client.Disconnect();
  } else {
    printf("Usage: %s host_ip port connections [thread_count]\n", argv[0]);
  }
}
