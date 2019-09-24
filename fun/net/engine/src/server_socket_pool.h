#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

// TODO 소켓 주소 family를 별도로 지정할 수 있으면 좋을듯...

/**
 * Since the socket API takes a long time to create a new socket, it is
 * necessary to advance it in a separate thread before returning it.
 */
class ServerSocketPool : public Singleton<ServerSocketPool>, public Runnable {
 public:
  ServerSocketPool();
  ~ServerSocketPool();

  InternalSocket* NewTcpSocket(
      IInternalSocketDelegate* Delegate,
      ISocketIoCompletionDelegate* IoCompletionDelegate);

 private:
  // Runnable interface
  void Run() override;

  FUN_ALIGNED_VOLATILE bool should_stop_thread_;
  UniquePtr<Thread> thread_;

  FastMutex mutex_;
  Array<SOCKET> newbie_sockets_;  // List로 하는건 어떨런지?? 삽입삭제가
                                  // 빈번하지 않으려나??
};

}  // namespace net
}  // namespace fun
