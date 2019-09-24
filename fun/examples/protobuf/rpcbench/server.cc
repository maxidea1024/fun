#include <examples/protobuf/rpcbench/echo.pb.h>

#include <red/net/protorpc/RpcServer.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <unistd.h>

using namespace fun;
using namespace fun::net;

namespace echo {

class EchoServiceImpl : public EchoService {
 public:
  virtual void Echo(::google::protobuf::RpcController* controller,
                    const ::echo::EchoRequest* request,
                    ::echo::EchoResponse* response,
                    ::google::protobuf::Closure* done) {
    // LOG_INFO << "EchoServiceImpl::Solve";
    response->set_payload(request->payload());
    done->Run();
  }
};

}  // namespace echo

int main(int argc, char* argv[]) {
  int nThreads = argc > 1 ? atoi(argv[1]) : 1;
  LOG_INFO << "pid = " << Process::CurrentPid() << " threads = " << nThreads;
  EventLoop loop;
  int port = argc > 2 ? atoi(argv[2]) : 8888;
  InetAddress listen_addr(static_cast<uint16_t>(port));
  echo::EchoServiceImpl impl;
  RpcServer server(&loop, listen_addr);
  server.SetThreadCount(nThreads);
  server.RegisterService(&impl);
  server.Start();
  loop.Loop();
}
