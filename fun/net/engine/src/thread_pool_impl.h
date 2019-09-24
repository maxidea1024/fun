//@deprecated
#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class ThreadPoolImpl;

/**
 * stop될때에 referrer이 있는지 체크 하기 위한 용도.
 * 물론 refcount를 만들수도 있지만, 나중을 생각하면 이것이 활용도가 높다.
 */
class IThreadReferer {
 public:
  virtual ~IThreadReferer() {}
};


class ThreadPoolImpl
  : public ThreadPool2
  , public ICompletionPortCallbacks
  , public Runnable {
 public:
  ThreadPoolImpl();
  virtual ~ThreadPoolImpl();

  // Runnable interface
 private:
  void Run() override;

  // ThreadPool2 interface
 public:
  void SetCallbacks(IThreadPoolCallbacks* callbacks) override;
  void Start(int32 thread_count) override;
  void Stop() override;

 protected:
  void OnCompletionPortWarning(CompletionPort* completion_port, const char* msg) override;

 public:
  void AssertIsLockedByCurrentThread() {
    fun_check(mutex_.IsLockedByCurrentThread() == true);
  }

  void AssertIsNotLockedByCurrentThread() {
    fun_check(mutex_.IsLockedByCurrentThread() == false);
  }

  bool RegistReferer(IThreadReferer* referer);
  void UnregisterReferer(IThreadReferer* referer);
  bool IsCurrentThread();
  void GetThreadInfos(Array<ThreadInfo>& out_info_list);
  bool IsActive();
  void AssociateSocket(InternalSocket* socket);
  void PostCompletionStatus(ICompletionKey* key, UINT_PTR custom_value);

 private:
  CCriticalSection2 mutex_;

  SharedPtr<CompletionPort> completion_port_;
  Array<UniquePtr<RunnableThread>> thread_pool_worker_;
  FUN_ALIGNED_VOLATILE bool stop_all_threads_;
  FUN_ALIGNED_VOLATILE bool start_flag_;
  IThreadPoolCallbacks* callbacks_;
  List<IThreadReferer*> referers_;
};

} // namespace net
} // namespace fun
