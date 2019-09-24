#include "fun/net/tcp_server.h"
#include "fun/net/rpc/rpc_service.h"

namespace fun {
namespace net {
namespace rpc {

class Service;

class FUN_RPC_API RpcServer {
 public:
  RpcServer(EventLoop* loop, const InetAddress& listen_addr);
  
  void SetThreadCount(int thread_count) {
    tcp_server_.SetThreadCount(thread_count);
  }
  
  void RegisterService(Service*);
  void Start();
  
 private:
  void OnConnection(const TcpConnectionPtr& conn);
  
  EventLoop* loop_;
  TcpServer tcp_server_;
  Map<String,Service*> services_;
  RpcServiceImpl meta_service_;
};

} // namespace rpc
} // namespace net
} // namespace fun
