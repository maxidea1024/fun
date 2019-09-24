#include "fun/base/thread_pool.h"
#include "fun/base/runnable.h"
#include "fun/base/thread.h"
#include "fun/base/event.h"
#include "fun/base/thread_local.h"
#include "fun/base/error_handler.h"

#include <sstream>
#include <ctime>

#if defined(_WIN32_WCE) && _WIN32_WCE < 0x800
#include "wce_time.h"
#endif

namespace fun {

class PooledThread : public Runnable {
 public:
  PooledThread(const String& name, int32 stack_size = FUN_THREAD_STACK_SIZE);
  ~PooledThread();

  void Start(int32 cpu = -1);
  void Start(Thread::Priority priority, Runnable& target, int32 cpu = -1);
  void Start(Thread::Priority priority, Runnable& target, const String& name, int32 cpu = -1);
  bool IsIdle();
  int32 GetIdleTime();
  void Join();
  void Activate();
  void Release();
  void Run();

 private:
  bool idle_;
  std::time_t idle_time_;
  Runnable* target_;
  String name_;
  Thread thread_;
  Event target_ready_;
  Event target_completed_;
  Event started_;
  FastMutex mutex_;
};

PooledThread::PooledThread(const String& name, int32 stack_size)
  : idle_(true),
    idle_time_(0),
    target_(0),
    name_(name),
    thread_(name),
    target_ready_(),
    target_completed_(EventResetType::Manual),
    started_(),
    mutex_() {
  fun_check_dbg(stack_size >= 0);
  thread_.SetStackSize(stack_size);
#if defined(_WIN32_WCE) && _WIN32_WCE < 0x800
  idle_time_ = wceex_time(NULL);
#else
  idle_time_ = std::time(NULL);
#endif
}

PooledThread::~PooledThread() {
}

void PooledThread::Start(int32 cpu) {
  thread_.Start(*this);
  started_.Wait();
  if (cpu >= 0) {
    thread_.SetAffinity(static_cast<uint32>(cpu));
  }
}

void PooledThread::Start(Thread::Priority priority, Runnable& target, int32 cpu) {
  ScopedLock<FastMutex> guard(mutex_);

  fun_check(target_ == 0);

  target_ = &target;
  thread_.SetPriority(priority);
  target_ready_.Set();
  if (cpu >= 0) {
    thread_.SetAffinity(static_cast<uint32>(cpu));
  }
}

void PooledThread::Start( Thread::Priority priority,
                          Runnable& target,
                          const String& name,
                          int32 cpu) {
  ScopedLock<FastMutex> guard(mutex_);

  String full_name(name);
  if (name.IsEmpty()) {
    full_name = name_;
  } else {
    full_name.Append(" (");
    full_name.Append(name_);
    full_name.Append(")");
  }
  thread_.SetName(full_name);
  thread_.SetPriority(priority);

  fun_check(target_ == 0);

  target_ = &target;
  target_ready_.Set();
  if (cpu >= 0) {
    thread_.SetAffinity(static_cast<uint32>(cpu));
  }
}

bool PooledThread::IsIdle() {
  ScopedLock<FastMutex> guard(mutex_);
  return idle_;
}

int32 PooledThread::GetIdleTime() {
  ScopedLock<FastMutex> guard(mutex_);

#if defined(_WIN32_WCE) && _WIN32_WCE < 0x800
  return (int32)(wceex_time(NULL) - idle_time_);
#else
  return (int32)(time(NULL) - idle_time_);
#endif
}

void PooledThread::Join() {
  mutex_.Lock();
  Runnable* target = target_;
  mutex_.Unlock();
  if (target) {
    target_completed_.Wait();
  }
}

void PooledThread::Activate() {
  ScopedLock<FastMutex> guard(mutex_);

  fun_check(idle_);
  idle_ = false;
  target_completed_.Reset();
}

void PooledThread::Release() {
  const int32 JOIN_TIMEOUT = 10000;

  mutex_.Lock();
  target_ = 0;
  mutex_.Unlock();

  // In case of a statically Allocated thread pool (such
  // as the default thread pool), Windows may have already
  // terminated the thread before we got here.
  if (thread_.IsRunning()) {
    target_ready_.Set();
  }

  if (thread_.TryJoin(JOIN_TIMEOUT)) {
    delete this;
  }
}

void PooledThread::Run() {
  started_.Set();

  for (;;) {
    target_ready_.Wait();
    mutex_.Lock();
    if (target_) { // a NULL target means kill yourself
      Runnable* target = target_;
      mutex_.Unlock();
      try {
        target->Run();
      } catch (Exception& e) {
        ErrorHandler::Handle(e);
      } catch (std::exception& e) {
        ErrorHandler::Handle(e);
      } catch (...) {
        ErrorHandler::Handle();
      }

      ScopedLock<FastMutex> guard(mutex_);
      target_ = nullptr;
#if defined(_WIN32_WCE) && _WIN32_WCE < 0x800
      idle_time_ = wceex_time(NULL);
#else
      idle_time_ = time(NULL);
#endif
      idle_ = true;
      target_completed_.Set();
      ThreadLocalStorage::Clear();
      thread_.SetName(name_);
      thread_.SetPriority(Thread::PRIO_NORMAL);
    } else {
      mutex_.Unlock();
      break;
    }
  }
}


//
// ThreadPool
//

ThreadPool::ThreadPool(
        int32 min_capacity,
        int32 max_capacity,
        int32 idle_time,
        int32 stack_size,
        ThreadAffinityPolicy affinity_policy)
  : min_capacity_(min_capacity),
    max_capacity_(max_capacity),
    idle_time_(idle_time),
    serial_(0),
    age_(0),
    stack_size_(stack_size),
    affinity_policy_(affinity_policy),
    last_cpu_(0) {
  fun_check(min_capacity >= 1 && max_capacity >= min_capacity && idle_time > 0);

  int32 cpu = -1;
  int32 cpu_count = Environment::GetProcessorCount();

  for (int32 i = 0; i < min_capacity_; i++) {
    if (affinity_policy_ == TAP_UNIFORM_DISTRIBUTION) {
      cpu = last_cpu_.Value() % cpu_count;
      last_cpu_++;
    }
    PooledThread* thread = CreateThread();
    threads_.Add(thread);
    thread->Start(cpu);
  }
}

ThreadPool::ThreadPool(
        const String& name,
        int32 min_capacity,
        int32 max_capacity,
        int32 idle_time,
        int32 stack_size,
        ThreadAffinityPolicy affinity_policy)
  : name_(name),
    min_capacity_(min_capacity),
    max_capacity_(max_capacity),
    idle_time_(idle_time),
    serial_(0),
    age_(0),
    stack_size_(stack_size),
    affinity_policy_(affinity_policy),
    last_cpu_(0) {
  fun_check(min_capacity >= 1 && max_capacity >= min_capacity && idle_time > 0);

  int32 cpu = -1;
  int32 cpu_count = Environment::GetProcessorCount();
  for (int32 i = 0; i < min_capacity_; i++) {
    if (affinity_policy_ == TAP_UNIFORM_DISTRIBUTION) {
      cpu = last_cpu_.Value() % cpu_count;
      last_cpu_++;
    }
    PooledThread* thread = CreateThread();
    threads_.Add(thread);
    thread->Start(cpu);
  }
}

ThreadPool::~ThreadPool() {
  try {
    StopAll();
  } catch (...) {
    fun_unexpected();
  }
}

void ThreadPool::AddCapacity(int32 n) {
  ScopedLock<FastMutex> guard(mutex_);

  fun_check(max_capacity_ + n >= min_capacity_);
  max_capacity_ += n;
  Housekeep();
}

int32 ThreadPool::GetCapacity() const {
  ScopedLock<FastMutex> guard(mutex_);
  return max_capacity_;
}

int32 ThreadPool::GetAvailableCount() const {
  ScopedLock<FastMutex> guard(mutex_);

  int32 count = 0;
  for (const auto& thread : threads_) {
    if (thread->IsIdle()) {
      ++count;
    }
  }
  return (int32)(count + max_capacity_ - threads_.Count());
}

int32 ThreadPool::GetUsedCount() const {
  ScopedLock<FastMutex> guard(mutex_);

  int32 count = 0;
  for (const auto& thread : threads_) {
    if (!thread->IsIdle()) {
      ++count;
    }
  }
  return count;
}

int32 ThreadPool::GetAllocatedCount() const {
  ScopedLock<FastMutex> guard(mutex_);

  return threads_.Count();
}

int32 ThreadPool::GetAffinity(int32 cpu) {
  switch (static_cast<int32>(affinity_policy_)) {
    case TAP_UNIFORM_DISTRIBUTION: {
      cpu = last_cpu_.Value() % Environment::GetProcessorCount();
      last_cpu_++;
    }
    break;

    case TAP_DEFAULT: {
      cpu = -1;
    }
    break;

    case TAP_CUSTOM: {
      if ((cpu < -1) || (cpu >= Environment::GetProcessorCount())) {
        throw InvalidArgumentException("cpu argument is invalid");
      }
    }
    break;
  }
  return cpu;
}

void ThreadPool::Start(Runnable& target, int32 cpu) {
  GetThread()->Start(Thread::PRIO_NORMAL, target, GetAffinity(cpu));
}

void ThreadPool::Start(Runnable& target, const String& name, int32 cpu) {
  GetThread()->Start(Thread::PRIO_NORMAL, target, name, GetAffinity(cpu));
}

void ThreadPool::StartWithPriority( Thread::Priority priority,
                                    Runnable& target,
                                    int32 cpu) {
  GetThread()->Start(priority, target, GetAffinity(cpu));
}

void ThreadPool::StartWithPriority( Thread::Priority priority,
                                    Runnable& target,
                                    const String& name,
                                    int32 cpu) {
  GetThread()->Start(priority, target, name, GetAffinity(cpu));
}

void ThreadPool::StopAll() {
  ScopedLock<FastMutex> guard(mutex_);

  for (auto& thread : threads_) {
    thread->Release();
  }
  threads_.Clear();
}

void ThreadPool::JoinAll() {
  ScopedLock<FastMutex> guard(mutex_);

  for (auto& thread : threads_) {
    thread->Join();
  }
  Housekeep();
}

void ThreadPool::Collect() {
  ScopedLock<FastMutex> guard(mutex_);
  Housekeep();
}

void ThreadPool::Housekeep() {
  age_ = 0;

  if (threads_.Count() <= min_capacity_) {
    return;
  }

  ThreadList idle_threads;
  ThreadList expired_threads;
  ThreadList active_threads;
  idle_threads.Reserve(threads_.Count());
  active_threads.Reserve(threads_.Count());

  for (auto& thread : threads_) {
    if (thread->IsIdle()) {
      if (thread->GetIdleTime() < idle_time_) {
        idle_threads.Add(thread);
      } else {
        expired_threads.Add(thread);
      }
    } else {
      active_threads.Add(thread);
    }
  }

  int32 n = active_threads.Count();
  int32 limit = idle_threads.Count() + n;
  if (limit < min_capacity_) {
    limit = min_capacity_;
  }
  idle_threads.Append(expired_threads);
  threads_.Clear();
  for (auto& thread : idle_threads) {
    if (n < limit) {
      threads_.Add(thread);
      ++n;
    } else {
      thread->Release();
    }
  }
  threads_.Append(active_threads);
}

PooledThread* ThreadPool::GetThread() {
  ScopedLock<FastMutex> guard(mutex_);

  if (++age_ == 32) {
    Housekeep();
  }

  PooledThread* thread = nullptr;
  for (auto& test : threads_) {
    if (test->IsIdle()) {
      thread = test;
      break;
    }
  }
  if (!thread) {
    if (threads_.Count() < max_capacity_) {
      thread = CreateThread();
      try {
        thread->Start();
        threads_.Add(thread);
      } catch (...) {
        delete thread;
        throw;
      }
    } else {
      throw NoThreadAvailableException();
    }
  }
  thread->Activate();
  return thread;
}

PooledThread* ThreadPool::CreateThread() {
  String thread_name;
  thread_name << name_ << "[#" << ++serial_ << "]";
  return new PooledThread(thread_name, stack_size_);
}

} // namespace fun
