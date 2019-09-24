#include <examples/protobuf/resolver/resolver.pb.h>

#include <examples/cdns/Resolver.h>
#include <red/net/protorpc/RpcServer.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <boost/bind.hpp>

#include <unistd.h>

using namespace fun;
using namespace fun::net;

namespace resolver {

class ResolverServiceImpl : public ResolverService {
 public:
  ResolverServiceImpl(EventLoop* loop)
      : resolver_(loop, cdns::Resolver::kDnsOnly) {}

  virtual void Resolve(::google::protobuf::RpcController* controller,
                       const ::resolver::ResolveRequest* request,
                       ::resolver::ResolveResponse* response,
                       ::google::protobuf::Closure* done) {
    LOG_INFO << "ResolverServiceImpl::Resolve " << request->GetAddress();

    bool succeed = resolver_.Resolve(
        request->GetAddress(),
        boost::bind(&ResolverServiceImpl::DoneCallback, this,
                    request->GetAddress(), _1, response, done));
    if (!succeed) {
      response->set_resolved(false);
      done->Run();
    }
  }

 private:
  void DoneCallback(const String& host, const fun::net::InetAddress& address,
                    ::resolver::ResolveResponse* response,
                    ::google::protobuf::Closure* done)

  {
    LOG_INFO << "ResolverServiceImpl::DoneCallback " << host;
    int32_t ip = address.ipNetEndian();
    if (ip) {
      response->set_resolved(true);
      response->add_ip(ip);
      response->add_port(address.portNetEndian());
    } else {
      response->set_resolved(false);
    }
    done->Run();
  }

  cdns::Resolver resolver_;
};

}  // namespace resolver

int main() {
  LOG_INFO << "pid = " << Process::CurrentPid();

  EventLoop loop;
  InetAddress listen_addr(2053);
  resolver::ResolverServiceImpl impl(&loop);
  RpcServer server(&loop, listen_addr);
  server.RegisterService(&impl);
  server.Start();
  loop.Loop();
}
