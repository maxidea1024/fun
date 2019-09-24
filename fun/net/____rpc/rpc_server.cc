#include "fun/net/rpc/rpc_server.h"

namespace fun {
namespace net {
namespace rpc {

RpcServer::RpcServer(EventLoop* loop, const InetAddress& listen_addr)
  : loop_(loop)
  , tcp_server_(loop, listen_addr, "RpcServer")
  , services_()
  , meta_service_(&services_) {
  tcp_server_.SetConnectionCallback(std::bind(&RpcServer::OnConnection,this,_1));

  RegisterService(&meta_service_);
}

void RpcServer::RegisterService(Service* service) {
  const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
  services_.Add(desc->full_name(), service);
}

void RpcServer::Start() {
  tcp_server_.Start();
}

void RpcServer::OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "RpcServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
    << conn->GetLocalAddress().toIpPort() << " is "
    << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    RpcChannelPtr channel(new RpcChannel(conn));
    channel->SetServices(&services_);
    conn->SetMessageCallback(std::bind(&RpcChannel::OnMessage, get_pointer(channel), _1,_2,_3));
    conn->SetContext(channel);
  }
  else {
    conn->SetContext(RpcChannelPtr());
  }
}

} // namespace rpc
} // namespace net
} // namespace fun
