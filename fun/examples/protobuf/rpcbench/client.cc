#include <examples/protobuf/rpcbench/echo.pb.h>

#include <red/base/CountDownLatch.h>
#include <red/net/EventLoopThreadPool.h>
#include <red/net/protorpc/RpcChannel.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_connection.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

static const int kRequests = 50000;

class RpcClient : Noncopyable {
 public:
  RpcClient(EventLoop* loop, const InetAddress& server_addr,
            CountDownLatch* allConnected,
            CountDownLatch* allFinished)
      :  // loop_(loop),
        client_(loop, server_addr, "RpcClient"),
        channel_(new RpcChannel),
        stub_(get_pointer(channel_)),
        allConnected_(allConnected),
        allFinished_(allFinished),
        count_(0) {
    client_.SetConnectionCallback(
        boost::bind(&RpcClient::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&RpcChannel::OnMessage, get_pointer(channel_), _1, _2, _3));
    // client_.EnableRetry();
  }

  void Connect() { client_.Connect(); }

  void sendRequest() {
    echo::EchoRequest request;
    request.set_payload("001010");
    echo::EchoResponse* response = new echo::EchoResponse;
    stub_.Echo(NULL, &request, response,
               NewCallback(this, &RpcClient::replied, response));
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      // channel_.Reset(new RpcChannel(conn));
      conn->SetTcpNoDelay(true);
      channel_->setConnection(conn);
      allConnected_->CountDown();
    }
  }

  void replied(echo::EchoResponse* resp) {
    // LOG_INFO << "replied:\n" << resp->DebugString().c_str();
    // loop_->Quit();
    ++count_;
    if (count_ < kRequests) {
      sendRequest();
    } else {
      LOG_INFO << "RpcClient " << this << " finished";
      allFinished_->CountDown();
    }
  }

  // EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  echo::EchoService::Stub stub_;
  CountDownLatch* allConnected_;
  CountDownLatch* allFinished_;
  int count_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();
  if (argc > 1) {
    int nClients = 1;

    if (argc > 2) {
      nClients = atoi(argv[2]);
    }

    int nThreads = 1;

    if (argc > 3) {
      nThreads = atoi(argv[3]);
    }

    CountDownLatch allConnected(nClients);
    CountDownLatch allFinished(nClients);

    EventLoop loop;
    EventLoopThreadPool pool(&loop, "rpcbench-client");
    pool.SetThreadCount(nThreads);
    pool.start();
    InetAddress server_addr(argv[1], 8888);

    boost::ptr_vector<RpcClient> clients;
    for (int i = 0; i < nClients; ++i) {
      clients.push_back(new RpcClient(pool.GetNextLoop(), server_addr,
                                      &allConnected, &allFinished));
      clients.back().Connect();
    }
    allConnected.Wait();
    Timestamp start(Timestamp::Now());
    LOG_INFO << "all connected";
    for (int i = 0; i < nClients; ++i) {
      clients[i].sendRequest();
    }
    allFinished.Wait();
    Timestamp end(Timestamp::Now());
    LOG_INFO << "all finished";
    double seconds = TimeDifference(end, start);
    printf("%f seconds\n", seconds);
    printf("%.1f calls per second\n", nClients * kRequests / seconds);

    exit(0);
  } else {
    printf("Usage: %s host_ip numClients [thread_count]\n", argv[0]);
  }
}
