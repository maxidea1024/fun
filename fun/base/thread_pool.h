#pragma once

#include "fun/base/base.h"
#include "fun/base/thread.h"
#include "fun/base/mutex.h"
#include "fun/base/atomic_counter.h"
#include "fun/base/container/array.h"
#include "fun/base/string/string.h"

namespace fun {

class Runnable;
class PooledThread;

/**
 * A thread pool always keeps a number of threads running, ready
 * to accept work.
 *
 * Creating and starting a threads can impose a significant runtime
 * overhead to an application. A thread pool helps to improve
 * the performance of an application by reducing the number
 * of threads that have to be created (and destroyed again).
 * Threads in a thread pool are re-used once they become
 * available again.
 *
 * The thread pool always keeps a minimum number of threads
 * running. If the demand for threads increases, additional
 * threads are created. Once the demand for threads sinks
 * again, no-longer used threads are stopped and removed
 * from the pool.
 */
class FUN_BASE_API ThreadPool
{
 public:
  enum ThreadAffinityPolicy {
    TAP_DEFAULT = 0,
    TAP_UNIFORM_DISTRIBUTION,
    TAP_CUSTOM
  };

  /**
   * Creates a thread pool with minCapacity threads.
   * If required, up to maxCapacity threads are created
   * a NoThreadAvailableException exception is thrown.
   * If a thread is running idle for more than idleTime seconds,
   * and more than minCapacity threads are running, the thread
   * is killed. Threads are created with given stack size.
   * Threads are created with given affinity policy.
   */
  ThreadPool( int32 min_capacity = 2,
              int32 max_capacity = 16,
              int32 idle_time = 60,
              int32 stack_size = FUN_THREAD_STACK_SIZE,
              ThreadAffinityPolicy affinity_policy = TAP_DEFAULT);

  /**
   * Creates a thread pool with the given name and minCapacity threads.
   * If required, up to maxCapacity threads are created
   * a NoThreadAvailableException exception is thrown.
   * If a thread is running idle for more than idleTime seconds,
   * and more than minCapacity threads are running, the thread
   * is killed. Threads are created with given stack size.
   * Threads are created with given affinity policy.
   */
  ThreadPool( const String& name,
              int32 min_capacity = 2,
              int32 max_capacity = 16,
              int32 idle_time = 60,
              int32 stack_size = FUN_THREAD_STACK_SIZE,
              ThreadAffinityPolicy affinity_policy = TAP_DEFAULT);

  /**
   * Currently running threads will remain active
   * until they complete.
   */
  ~ThreadPool();

  /**
   * Increases (or decreases, if n is negative)
   * the maximum number of threads.
   */
  void AddCapacity(int32 n);

  /**
   * Returns the maximum capacity of threads.
   */
  int32 GetCapacity() const;

  /**
   * Sets the stack size for threads.
   * New stack size applies only for newly created threads.
   */
  void SetStackSize(int32 stack_size);

  /**
   * Returns the stack size used to create new threads.
   */
  int32 GetStackSize() const;

  /**
   * Sets the thread affinity policy for newly created threads.
   */
  void SetAffinityPolicy(ThreadAffinityPolicy affinity_policy);

  /**
   * Returns the thread affinity policy used to create new threads.
   */
  ThreadAffinityPolicy GetAffinityPolicy();

  /**
   * Returns the number of currently used threads.
   */
  int32 GetUsedCount() const;

  /**
   * Returns the number of currently allocated threads.
   */
  int32 GetAllocatedCount() const;

  /**
   * Returns the number available threads.
   */
  int32 GetAvailableCount() const;

  /**
   * Obtains a thread and starts the target on specified cpu.
   *
   * Throws a NoThreadAvailableException if no more
   * threads are available.
   */
  void Start(Runnable& target, int32 cpu = -1);

  /**
   * Obtains a thread and starts the target on specified cpu.
   * Assigns the given name to the thread.
   *
   * Throws a NoThreadAvailableException if no more
   * threads are available.
   */
  void Start(Runnable& target, const String& name, int32 cpu = -1);

  /**
   * Obtains a thread, adjusts the thread's priority, and starts the target on specified cpu.
   *
   * Throws a NoThreadAvailableException if no more
   * threads are available.
   */
  void StartWithPriority( Thread::Priority priority,
                          Runnable& target,
                          int32 cpu = -1);

  /**
   * Obtains a thread, adjusts the thread's priority, and starts the target on specified cpu.
   * Assigns the given name to the thread.
   *
   * Throws a NoThreadAvailableException if no more
   * threads are available.
   */
  void StartWithPriority( Thread::Priority priority,
                          Runnable& target,
                          const String& name,
                          int32 cpu = -1);

  /**
   * Stops all running threads and waits for their completion.
   *
   * Will also delete all thread objects.
   * If used, this method should be the last action before
   * the thread pool is deleted.
   *
   * Note: If a thread fails to stop within 10 seconds
   * (due to a programming error, for example), the
   * underlying thread object will not be deleted and
   * this method will return anyway. This allows for a
   * more or less graceful shutdown in case of a misbehaving
   * thread.
   */
  void StopAll();

  /**
   * Waits for all threads to complete.
   *
   * Note that this will not actually Join() the underlying
   * thread, but rather wait for the thread's runnables
   * to finish.
   */
  void JoinAll();

  /**
   * Stops and removes no longer used threads from the
   * thread pool. Can be called at various times in an
   * application's life time to help the thread pool
   * manage its threads. Calling this method is optional,
   * as the thread pool is also implicitly managed in
   * calls to Start(), AddCapacity() and JoinAll().
   */
  void Collect();

  /**
   * Returns the name of the thread pool,
   * or an empty string if no name has been
   * specified in the constructor.
   */
  const String& GetName() const;

  // definition in core.cc
  static ThreadPool& DefaultPool(ThreadAffinityPolicy affinity_policy = TAP_DEFAULT);

 protected:
  PooledThread* GetThread();
  PooledThread* CreateThread();

  void Housekeep();
  int32 GetAffinity(int32 cpu);

 private:
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator = (const ThreadPool&) = delete;

  typedef Array<PooledThread*> ThreadList;

  String name_;
  int32 min_capacity_;
  int32 max_capacity_;
  int32 idle_time_;
  int32 serial_;
  int32 age_;
  int32 stack_size_;
  ThreadList threads_;
  mutable FastMutex mutex_;
  ThreadAffinityPolicy affinity_policy_;
  AtomicCounter32 last_cpu_;
};


//
// inlines
//

FUN_ALWAYS_INLINE void ThreadPool::SetStackSize(int32 stack_size) {
  stack_size_ = stack_size;
}

FUN_ALWAYS_INLINE int32 ThreadPool::GetStackSize() const {
  return stack_size_;
}

FUN_ALWAYS_INLINE void ThreadPool::SetAffinityPolicy(ThreadPool::ThreadAffinityPolicy affinity_policy) {
  affinity_policy_ = affinity_policy;
}

FUN_ALWAYS_INLINE ThreadPool::ThreadAffinityPolicy ThreadPool::GetAffinityPolicy() {
  return affinity_policy_;
}

FUN_ALWAYS_INLINE const String& ThreadPool::GetName() const {
  return name_;
}

} // namespace fun
