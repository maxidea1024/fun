//TODO 이름을 CServerSocketPool에서 CTcpServerSocketPool로 바꾸자.
//TODO 어드레스 패밀리를 외부에서 설정할 수 있게 해야할까??

#include "fun/net/net.h"
#include "server_socket_pool.h"

namespace fun {
namespace net {

#if FUN_DISABLE_IPV6
static const int32 SOCKET_ADDR_FAMILY = AF_INET;
#else
static const int32 SOCKET_ADDR_FAMILY = AF_INET6;
#endif

//TODO 요구하는 곳에서 address-family를 지정할 수 있도록 하자.
InternalSocket* ServerSocketPool::NewTcpSocket( IInternalSocketDelegate* delegate,
                                                ISocketIoCompletionDelegate* io_completion_delegate) {
  // Note: This method should only be called when ServerSocketPool is not destroyed!
  if (should_stop_thread_) {
    throw Exception("ServerSocketPool is already destructed!");
  }

  // Wait until the socket is ready.
  // it'll be done anyway soon.

  const double begun_time = Clock::Seconds();
  while ((Clock::Seconds() - begun_time) < 10.0) { //10초???
    ScopedLock<FastMutex> guard(mutex_);

    if (!newbie_sockets_.IsEmpty()) {
      const int32 index = newbie_sockets_.Count() - 1;
      InternalSocket* new_socket = new InternalSocket(newbie_sockets_[index], delegate, io_completion_delegate);
      newbie_sockets_.RemoveAt(index);
      return new_socket;
    } else {
      guard.Unlock();

      // Give other threads a chance.
      CPlatformProcess::Sleep(0.001f);
    }
  }

  // it is rare, but I am not able to generate it by the deadline. At this time, let's just force it.
  // This API takes a long time to run in unlocked state.
  const SOCKET fd = socket(SOCKET_ADDR_FAMILY, SOCK_STREAM, IPPROTO_TCP);
  if (fd != INVALID_SOCKET) {
    InternalSocket::SetIPv6Only(fd, false);
    return new InternalSocket(fd, delegate, io_completion_delegate);
  } else {
    // socket creation is failed.
    return nullptr;
  }
}

ServerSocketPool::ServerSocketPool()
    : should_stop_thread_(false) {
  thread_.Reset(RunnableThread::Create(this, "ServerSocketPool"));
}

ServerSocketPool::~ServerSocketPool() {
  should_stop_thread_ = true;
  thread_->Join();
  thread_.Reset();

  ScopedLock<FastMutex> guard(mutex_);
  for (auto socket : newbie_sockets_) {
    NetUtil::AssertCloseSocketWillReturnImmediately(socket);
    closesocket(socket);
  }
  newbie_sockets_.Clear();
}

void ServerSocketPool::Run() {
  while (!should_stop_thread_) {
    // Keep 100 sockets at all times.
    ScopedLock<FastMutex> guard(mutex_);

    if (newbie_sockets_.Count() >= 100) {
      guard.Unlock();
      CPlatformProcess::Sleep(0.01f);
    } else {
      // Fill as many as we want.
      guard.Unlock();

      // This API takes a long time to run in unlocked state.
      const SOCKET fd = socket(SOCKET_ADDR_FAMILY, SOCK_STREAM, IPPROTO_TCP);
      if (fd != INVALID_SOCKET) {
        guard.guard();
        InternalSocket::SetIPv6Only(fd, false);
        newbie_sockets_.Add(fd);
      }
    }
  }
}

} // namespace net
} // namespace fun
