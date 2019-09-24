#pragma once

#include "fun/base/base.h"
#include "fun/base/runnable.h"

#if FUN_PLATFORM_UNIX_FAMILY && FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/signal_handler.h"
#endif

#include "fun/base/event.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/ftl/unique_ptr.h"
#include "fun/base/ref_counted.h"

#include <thread>

#ifdef __APPLE__
#define thread_local __thread
#elif defined(_MSC_VER)
#include "fun/base/windows_less.h"
#endif

namespace fun {

class FUN_BASE_API ThreadImpl {
 public:
  typedef std::thread::native_handle_type TIDImpl;

  typedef void (*Callable)(void*);

  enum Priority {
    PRIO_LOWEST_IMPL,
    PRIO_LOW_IMPL,
    PRIO_NORMAL_IMPL,
    PRIO_HIGH_IMPL,
    PRIO_HIGHEST_IMPL
  };

  enum Policy { POLICY_DEFAULT_IMPL = 0 };

  ThreadImpl();
  ~ThreadImpl();

  TIDImpl GetTidImpl() const;
  void SetPriorityImpl(int prio);
  int GetPriorityImpl() const;
  void SetOsPriorityImpl(int prio, int policy = 0);
  int GetOsPriorityImpl() const;
  static int GetMinOsPriorityImpl(int policy);
  static int GetMaxOsPriorityImpl(int policy);
  void SetStackSizeImpl(int size);
  int GetStackSizeImpl() const;
  void SetAffinityImpl(int cpu);
  int GetAffinityImpl() const;
  void StartImpl(SharedPtr<Runnable> target);
  void JoinImpl();
  bool JoinImpl(int32 milliseconds);
  bool IsRunningImpl() const;
  static void SleepImpl(int32 milliseconds);
  static void YieldImpl();

  static ThreadImpl* CurrentImpl();
  static TIDImpl CurrentTidImpl();

 protected:
  static void* RunnableEntry(void* thread);

 private:
  class CurrentThreadHolder {
   public:
    CurrentThreadHolder() {}
    ~CurrentThreadHolder() {}

    ThreadImpl* Get() const { return thread_; }
    void Set(ThreadImpl* thread) { thread_ = thread; }

   private:
    static thread_local ThreadImpl* thread_;
  };

  struct ThreadData : public RefCountedObject {
    ThreadData()
        : thread(),
          prio(PRIO_NORMAL_IMPL),
          policy(0),
          task(0),
          done(EventResetType::Manual),
          stack_size(FUN_THREAD_STACK_SIZE),
          cpu(-1),
          started(false),
          joined(false) {}

    SharedPtr<Runnable> runnable_target;
    UniquePtr<std::thread> thread;
    TIDImpl tid;
    int prio;
    int os_prio;
    int policy;
    int task;
    Event done;
    size_t stack_size;
    int cpu;
    bool started;
    bool joined;
  };

  RefCountedPtr<ThreadData> data_;

  static CurrentThreadHolder current_thread_holder_;

#if FUN_PLATFORM_UNIX_FAMILY && FUN_PLATFORM != FUN_PLATFORM_VXWORKS
  SignalHandler::JumpBufferList jump_buffer_list_;
  friend class SignalHandler;
#endif
};

//
// inlines
//

FUN_ALWAYS_INLINE int ThreadImpl::GetPriorityImpl() const {
  return data_->prio;
}

FUN_ALWAYS_INLINE int ThreadImpl::GetOsPriorityImpl() const {
  return data_->os_prio;
}

FUN_ALWAYS_INLINE bool ThreadImpl::IsRunningImpl() const {
  return data_->runnable_target.IsValid();
}

FUN_ALWAYS_INLINE void ThreadImpl::YieldImpl() { std::this_thread::yield(); }

FUN_ALWAYS_INLINE int ThreadImpl::GetStackSizeImpl() const {
  return static_cast<int>(data_->stack_size);
}

FUN_ALWAYS_INLINE ThreadImpl::TIDImpl ThreadImpl::GetTidImpl() const {
  return data_->tid;
}

}  // namespace fun
