#pragma once

#include "fun/base/base.h"
#include "fun/base/event.h"
#include "fun/base/mutex.h"
#include "fun/base/environment.h"

//TODO thread_std만 사용하고 있으므로, 구태여 impl형태로 구현할 필요가 없다.
#include "fun/base/thread_std.h"

namespace fun {

class Runnable;
class ThreadLocalStorage;

/**
 * This class implements a platform-independent
 * wrapper to an operating system thread.
 *
 * Every Thread object gets a unique (within
 * its process) numeric thread ID.
 * Furthermore, a thread can be assigned a name.
 * The name of a thread can be changed at any time.
 */
class FUN_BASE_API Thread : private ThreadImpl
{
 public:
  typedef ThreadImpl::TIDImpl TID;

  using ThreadImpl::Callable;

  /**
   * Thread priorities.
   */
  enum Priority {
    /** The lowest thread priority. */
    PRIO_LOWEST = PRIO_LOWEST_IMPL,
    /** A lower than normal thread priority. */
    PRIO_LOW = PRIO_LOW_IMPL,
    /** The normal thread priority. */
    PRIO_NORMAL = PRIO_NORMAL_IMPL,
    /** A higher than normal thread priority. */
    PRIO_HIGH = PRIO_HIGH_IMPL,
    /** The highest thread priority. */
    PRIO_HIGHEST = PRIO_HIGHEST_IMPL
  };

  enum Policy { POLICY_DEFAULT = POLICY_DEFAULT_IMPL };

  /**
   * Creates a thread. Call Start() to start it.
   */
  Thread();

  /**
   * Creates a named thread. Call Start() to start it.
   */
  Thread(const String& name);

  /**
   * Destroys the thread.
   */
  ~Thread();

  /**
   * Returns the unique thread ID of the thread.
   */
  int32 GetId() const;

  /**
   * Returns the native thread ID of the thread.
   */
  TID GetTid() const;

  /**
   * Returns the name of the thread.
   */
  String GetName() const;

  /**
   * Sets the name of the thread.
   */
  void SetName(const String& name);

  /**
   * Sets the thread's priority.
   *
   * Some platform only allow changing a thread's priority
   * if the process has certain privileges.
   */
  void SetPriority(Priority prio);

  /**
   * Returns the thread's priority.
   */
  Priority GetPriority() const;

  /**
   * Sets the thread's priority, using an operating system specific
   * priority value. Use GetMinOsPriority() and GetMaxOsPriority() to
   * obtain minimum and maximum priority values. Additionally,
   * a scheduling policy can be specified. The policy is currently
   * only used on POSIX platforms where the values SCHED_OTHER (default),
   * SCHED_FIFO and SCHED_RR are supported.
   */
  void SetOsPriority(int32 prio, int32 policy = POLICY_DEFAULT);

  /**
   * Returns the thread's priority, expressed as an operating system
   * specific priority value.
   *
   * May return 0 if the priority has not been explicitly set.
   */
  int32 GetOsPriority() const;

  /**
   * Returns the minimum operating system-specific priority value,
   * which can be passed to SetOsPriority() for the given policy.
   */
  static int32 GetMinOsPriority(int32 policy = POLICY_DEFAULT);

  /**
   * Returns the maximum operating system-specific priority value,
   * which can be passed to SetOsPriority() for the given policy.
   */
  static int32 GetMaxOsPriority(int32 policy = POLICY_DEFAULT);

  /**
   * Returns the thread's stack size in bytes.
   * If the default stack size is used, 0 is returned.
   */
  int32 GetStackSize() const;

  /**
   * Sets the thread's stack size in bytes.
   * Setting the stack size to 0 will use the default stack size.
   * Typically, the real stack size is rounded up to the nearest
   * page size multiple.
   */
  void SetStackSize(int32 size);

  /**
   * Binds the thread to run only on the CPU core with the
   * given index.
   *
   * Does nothing if the system does not support CPU affinity for
   * threads.
   */
  void SetAffinity(int32 cpu);

  /**
   * Returns the index of the CPU core this thread has been bound to,
   * or -1 if the thread has not been bound to a CPU.
   */
  int32 GetAffinity() const;

  /**
   * Starts the thread with the given target.
   *
   * Note that the given Runnable object must remain
   * valid during the entire lifetime of the thread, as
   * only a reference to it is stored internally.
   */
  void Start(Runnable& target);

  /**
   * Starts the thread with the given target and parameter.
   */
  void Start(Callable target, void* data = nullptr);

  /**
   * Starts the thread with the given functor object or lambda.
   */
  template <typename Functor>
  void StartFunc(Functor fn) {
    StartImpl(new FunctorRunnable<Functor>(fn));
  }

  /**
   * Waits until the thread completes execution.
   * If multiple threads try to join the same
   * thread, the result is undefined.
   */
  void Join();

  /**
   * Waits for at most the given interval for the thread
   * to complete. Throws a TimeoutException if the thread
   * does not complete within the specified time interval.
   */
  void Join(int32 milliseconds);

  /**
   * Waits for at most the given interval for the thread
   * to complete. Returns true if the thread has finished,
   * false otherwise.
   */
  bool TryJoin(int32 milliseconds);

  /**
   * Returns true if the thread is running.
   */
  bool IsRunning() const;

  /**
   * Wakes up the thread which is in the state of interruptible
   * sleep. For threads that are not suspended, calling this
   * function has the effect of preventing the subsequent
   * TrySleep() call to put thread in a suspended state.
   */
  void WakeUp();

  /**
   * Starts an interruptible sleep. When TrySleep() is called,
   * the thread will remain suspended until:
   *   - the timeout expires or
   *   - WakeUp() is called
   *
   * Function returns true if sleep attempt was completed, false
   * if sleep was interrupted by a WakeUp() call.
   * A frequent scenario where TrySleep()/WakeUp() pair of functions
   * is useful is with threads spending most of the time idle,
   * with periodic activity between the idle times; trying to sleep
   * (as opposed to sleeping) allows immediate ending of idle thread
   * from the outside.
   *
   * The TrySleep() and WakeUp() calls should be used with
   * understanding that the suspended state is not a true sleep,
   * but rather a state of waiting for an event, with timeout
   * expiration. This makes order of calls significant; calling
   * WakeUp() before calling TrySleep() will prevent the next
   * TrySleep() call to actually suspend the thread (which, in
   * some scenarios, may be desirable behavior).
   */
  static bool TrySleep(int32 milliseconds);

  /**
   * Suspends the current thread for the specified amount of time.
   */
  static void Sleep(int32 milliseconds);

  /**
   * Yields cpu to other threads.
   */
  static void Yield();

  /**
   * Returns the Thread object for the currently active thread.
   * If the current thread is the main thread, nullptr is returned.
   */
  static Thread* Current();

  /**
   * Returns the native thread ID for the current thread.
   */
  static TID CurrentTid();

 protected:
  /**
   * Returns a reference to the thread's local storage.
   */
  ThreadLocalStorage& Tls();

  /**
   * Clears the thread's local storage.
   */
  void ClearTls();

  /**
   * Creates a unique name for a thread.
   */
  String MakeName();

  /**
   * Creates and returns a unique id for a thread.
   */
  static int32 UniqueId();

  template <typename Functor>
  class FunctorRunnable : public Runnable {
   public:
    FunctorRunnable(const Functor& functor) : functor_(functor) {}
    ~FunctorRunnable() {}

    void Run() override {
      functor_();
    }

   private:
    Functor functor_;
  };

 private:
  Thread(const Thread&) = delete;
  Thread& operator = (const Thread&) = delete;

  int32 id_;
  String name_;
  ThreadLocalStorage* tls_;
  Event event_;
  FastMutex mutex_;

  friend class ThreadLocalStorage;
  friend class PooledThread;
};



//
// inlines
//

FUN_ALWAYS_INLINE int32 Thread::GetId() const {
  return id_;
}

FUN_ALWAYS_INLINE Thread::TID Thread::GetTid() const {
  return GetTidImpl();
}

FUN_ALWAYS_INLINE String Thread::GetName() const {
  ScopedLock<FastMutex> guard(mutex_);
  return name_;
}

FUN_ALWAYS_INLINE bool Thread::IsRunning() const {
  return IsRunningImpl();
}

FUN_ALWAYS_INLINE void Thread::Sleep(int32 milliseconds) {
  SleepImpl(milliseconds);
}

FUN_ALWAYS_INLINE void Thread::Yield() {
  YieldImpl();
}

FUN_ALWAYS_INLINE Thread* Thread::Current() {
  return static_cast<Thread*>(CurrentImpl());
}

FUN_ALWAYS_INLINE void Thread::SetOsPriority(int32 prio, int32 policy) {
  SetOsPriorityImpl(prio, policy);
}

FUN_ALWAYS_INLINE int32 Thread::GetOsPriority() const {
  return GetOsPriorityImpl();
}

FUN_ALWAYS_INLINE int32 Thread::GetMinOsPriority(int32 policy) {
  return ThreadImpl::GetMinOsPriorityImpl(policy);
}

FUN_ALWAYS_INLINE int32 Thread::GetMaxOsPriority(int32 policy) {
  return ThreadImpl::GetMaxOsPriorityImpl(policy);
}

FUN_ALWAYS_INLINE void Thread::SetStackSize(int32 size) {
  SetStackSizeImpl(size);
}

FUN_ALWAYS_INLINE void Thread::SetAffinity(int32 cpu) {
  SetAffinityImpl(cpu);
}

FUN_ALWAYS_INLINE int32 Thread::GetAffinity() const {
  return GetAffinityImpl();
}

FUN_ALWAYS_INLINE int32 Thread::GetStackSize() const {
  return GetStackSizeImpl();
}

FUN_ALWAYS_INLINE Thread::TID Thread::CurrentTid() {
  return CurrentTidImpl();
}

} // namespace fun
