#include "fun/base/base.h"
#include "fun/base/memory_pool.h"
#include "fun/base/ref_counted_object.h"
#include "fun/base/logging/logger.h"
#include "fun/base/thread_pool.h"
#include "fun/base/string/string.h"

// Important note:
//
// This file contains definitions of static and/or global variables
// which depend on each other and require deterministic order of
// creation and destruction. Therefore, the definition order in
// this file should not be changed, nor should these definitions be
// moved elsewhere without a good reason *and* proper understanding
// of the consequences.


namespace fun {

//
// FastMemoryPool
//

//namespace {
//  FastMemoryPool<WeakRefCounter> wrcFMP(FUN_FAST_MEMORY_POOL_PREALLOC);
//}
//
//template <typename T>
//FastMemoryPool<T>& GetFastMemoryPool() {
//  fun_check(false);
//}
//
//template <>
//FastMemoryPool<WeakRefCounter>& GetFastMemoryPool() {
//  return wrcFMP;
//}

#if FUN_ENABLE_REFCOUNT_DC

//
// RefCountDiagnosticContext
//

// static RefCountDiagnosticContext members
RCDC::TraceMap RCDC::trace_map_;
SpinlockMutex RCDC::mutex_;
bool RCDC::full_ = false;

namespace {
RefCountDiagnosticContext rcdc;
}

RefCountDiagnosticContext& RefCountDiagnosticContext::Get() {
  return rcdc;
}

#endif // FUN_ENABLE_REFCOUNT_DC


//
// Logger
//

// static Logger members
ScopedPtr<Logger::Logger::LoggerMap> Logger::logger_map_;
Mutex Logger::logger_map_mutex_;
const String Logger::ROOT;

/**
 * Ensures proper Logger termination.
 */
class AutoLoggerShutdown {
 public:
  AutoLoggerShutdown() {}

  ~AutoLoggerShutdown() {
    try {
      Logger::Shutdown();
    } catch (...) {
      fun_unexpected();
    }
  }
};

namespace {
static AutoLoggerShutdown als;
}


//
// ThreadPool
//

class ThreadPoolSingletonHolder {
 public:
  ThreadPoolSingletonHolder() {
    pool_ = nullptr;
  }

  ~ThreadPoolSingletonHolder() {
    delete pool_;
  }

  ThreadPool* GetPool(ThreadPool::ThreadAffinityPolicy affinity_policy = ThreadPool::TAP_DEFAULT) {
    FastMutex::ScopedLock guard(mutex_);

    if (!pool_) {
      pool_ = new ThreadPool("default");
      pool_->SetAffinityPolicy(affinity_policy);
#if FUN_THREAD_STACK_SIZE > 0
      pool_->SetStackSize(FUN_THREAD_STACK_SIZE);
#endif
    }
    return pool_;
  }

 private:
  ThreadPool* pool_;
  FastMutex mutex_;
};

namespace {
static ThreadPoolSingletonHolder sh;
}

ThreadPool& ThreadPool::DefaultPool(ThreadAffinityPolicy affinity_policy) {
  return *sh.GetPool(affinity_policy);
}

} // namespace fun
