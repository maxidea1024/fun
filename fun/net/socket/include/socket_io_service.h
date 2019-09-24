// TODO 없애는게 좋을듯함...

#pragma once

#include "Net/Core/StdThreadPool.h"
#include "socket.h"

#include <condition_variable>  //차후에 대체품을 작성해야함.
#include <mutex>               //차후에 대체품을 작성해야함.

namespace fun {

class SelfPipe;

class SocketIoService : public Runnable {
 public:
  SocketIoService();
  ~SocketIoService();

  // disable copy constructor and assignment operator.
  SocketIoService(const SocketIoService&) = delete;
  SocketIoService& operator=(const SocketIoService&) = delete;

  static SharedPtr<SocketIoService>& GetDefaultIoService();
  static void SetDefaultIoService(const SharedPtr<SocketIoService>& io_service);

 private:
  // TODO singleton으로 숨기는게 좋을듯...
  static SharedPtr<SocketIoService> default_io_service_;

 public:
  typedef Function<void(SOCKET)> Callback;

  void Associate(const Socket& socket, const Callback& read_cb = nullptr,
                 const Callback& write_cb = nullptr);
  void SetReadCallback(const Socket& socket, const Callback& read_cb);
  void SetWriteCallback(const Socket& socket, const Callback& write_cb);
  void Unassociate(const Socket& socket, bool wait_for_removal);

  void WaitForRemoval(const Socket& socket);

 private:
  struct AssociatedSocket {
    AssociatedSocket()
        : read_cb(nullptr),
          write_cb(nullptr),
          is_executing_read_cb(false),
          is_executing_write_cb(false) {}

    /** Read event 감지시 호출되는 콜백함수. */
    Callback read_cb;
    /** Read callback 함수가 스레드 풀에서 실행중인지 여부. */
    ThreadSafeBool is_executing_read_cb;

    /** Write event 감지시 호출되는 콜백함수. */
    Callback write_cb;
    /** Write callback 함수가 스레드 풀에서 실행중인지 여부. */
    ThreadSafeBool is_executing_write_cb;

    /**
     * Read/Write callback이 현재 스레드풀에서 실행중일 경우
     * 바로 목록에서 제거할 수 없으므로, 표시만 해두고 차후에
     * 안전한때에 목록에서 제거되도록 표시만 하는 역활.
     */
    ThreadSafeBool marked_for_unassociate;
  };

 private:
  // Runnable interface
  void Run() override { Poll(); }

 private:
  void Poll();

  int32 InitPollFDsInfo();

  void ProcessEvents();
  void ProcessReadEvent(SOCKET fd, AssociatedSocket& socket);
  void ProcessWriteEvent(SOCKET fd, AssociatedSocket& socket);

  void WakeupPoller();

 private:
  Map<SOCKET, AssociatedSocket> associated_sockets_;
  AtomicCounter32 should_stop_;
  SharedPtr<Thread> poll_thread_;

  StdThreadPool thread_pool_;

  std::mutex associated_sockets_mutex_;
  Array<SOCKET> polled_fds_;
  fd_set read_set_;
  fd_set write_set_;
  std::condition_variable wait_for_removal_cv_;
  SelfPipe* notifier_;
};

}  // namespace fun
