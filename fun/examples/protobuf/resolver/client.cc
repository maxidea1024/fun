#include <examples/protobuf/resolver/resolver.pb.h>

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_connection.h"
#include <red/net/protorpc/RpcChannel.h>

#include <arpa/inet.h>  // inet_ntop

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class RpcClient : Noncopyable
{
 public:
  RpcClient(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop)
    , client_(loop, server_addr, "RpcClient")
    , channel_(new RpcChannel)
    , got_(0)
    , total_(0)
    , stub_(get_pointer(channel_))
  {
    client_.SetConnectionCallback(boost::bind(&RpcClient::OnConnection, this, _1));
    client_.SetMessageCallback(boost::bind(&RpcChannel::OnMessage, get_pointer(channel_), _1, _2, _3));

    // client_.EnableRetry();
  }

  void Connect()
  {
    client_.Connect();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn)
  {
    if (conn->IsConnected())
    {
      //channel_.Reset(new RpcChannel(conn));
      channel_->setConnection(conn);
      total_ = 4;
      Resolve("www.example.com");
      Resolve("www.chenshuo.com");
      Resolve("www.google.com");
      Resolve("acme.chenshuo.org");
    }
    else
    {
      loop_->Quit();
    }
  }

  void Resolve(const String& host)
  {
    resolver::ResolveRequest request;
    request.set_address(host);
    resolver::ResolveResponse* response = new resolver::ResolveResponse;

    stub_.Resolve(NULL, &request, response,
        NewCallback(this, &RpcClient::resolved, response, host));
  }

  void resolved(resolver::ResolveResponse* resp, String host)
  {
    if (resp->resolved())
    {
      char buf[32];
      uint32_t ip = resp->ip(0);
      inet_ntop(AF_INET, &ip, buf, sizeof buf);

      LOG_INFO << "resolved " << host << " : " << buf << "\n"
               << resp->DebugString().c_str();
    }
    else
    {
      LOG_INFO << "resolved " << host << " failed";
    }

    if (++got_ >= total_)
    {
      client_.Disconnect();
    }
  }

  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  int got_;
  int total_;
  resolver::ResolverService::Stub stub_;
};


int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << Process::CurrentPid();
  if (argc > 1)
  {
    EventLoop loop;
    InetAddress server_addr(argv[1], 2053);

    RpcClient rpc_client(&loop, server_addr);
    rpc_client.Connect();
    loop.Loop();
  }
  else
  {
    printf("Usage: %s host_ip\n", argv[0]);
  }
}
