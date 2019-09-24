#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"
#include "fun/base/scoped_unlock.h"
#include "fun/base/event.h"
#include "fun/base/exception.h"
#include "fun/base/container/list.h"

namespace fun {

/**
 * \~english
 * A Condition is a synchronization object used to block a thread
 * until a particular condition is met.
 * A Condition object is always used in conjunction with
 * a Mutex (or FastMutex) object.
 * 
 * Condition objects are similar to POSIX condition variables, which the
 * difference that Condition is not subject to spurious wakeups.
 * 
 * Threads waiting on a Condition are resumed in FIFO order.
 * 
 * \~korean
 * Condition는 특정 조건이 만족할때까지 스레드를 차단하는데 사용되는 객체입니다.
 * Condition 객체는 Mutex(또는 FastMutex) 객체와 함께 사용됩니다.
 * 
 * Condition 객체는 POSIX 시스템의 조건 변수(conditiona variable)와 유사합니다.
 * 이 객체는 가짜 깨우기(wakeup)의 대상은 아닙니다.
 * 
 * Condition에서 대기중인 스레드는 FIFO 순서로 재개됩니다.
 * 즉, 먼저 기다린 스레드를 먼저 재개하게 됩니다.
 */
class FUN_BASE_API Condition {
 public:
  /**
   * Creates the Condition.
   */
  Condition();

  /**
   * Destroys the Condition.
   */
  ~Condition();

  /**
   * Unlocks the mutex (which must be locked upon calling
   * Wait()) and waits until the Condition is signalled.
   * 
   * The given mutex will be locked again upon
   * leaving the function, even in case of an exception.
   */
  template <typename MutexType>
  void Wait(MutexType& mutex) {
    ScopedUnlock<MutexType> unlock(mutex, false);

    Event event; //TODO event pooling
    {
      ScopedLock<FastMutex> lock(wait_queue_mutex_);
      mutex.Unlock();
      Enqueue(event);
    }

    event.Wait();
  }

  /**
   * Unlocks the mutex (which must be locked upon calling
   * Wait()) and waits for the given time until the Condition is signalled.
   * 
   * The given mutex will be locked again upon successfully leaving the
   * function, even in case of an exception.
   * 
   * Throws a TimeoutException if the Condition is not signalled
   * within the given time interval.
   */
  template <typename MutexType>
  void Wait(MutexType& mutex, int32 milliseconds) {
    if (!TryWait(mutex, milliseconds)) {
      throw TimeoutException();
    }
  }

  /**
   * Unlocks the mutex (which must be locked upon calling
   * TryWait()) and waits for the given time until the Condition is signalled.
   * 
   * The given mutex will be locked again upon leaving the
   * function, even in case of an exception.
   * 
   * Returns true if the Condition has been signalled
   * within the given time interval, otherwise false.
   */
  template <typename MutexType>
  bool TryWait(MutexType& mutex, int32 milliseconds) {
    ScopedUnlock<MutexType> unlock(mutex, false);
    Event event; //TODO event pooling
    {
      ScopedLock<FastMutex> lock(wait_queue_mutex_);
      mutex.Unlock();
      Enqueue(event);
    }

    if (!event.TryWait(milliseconds)) {
      ScopedLock<FastMutex> lock(wait_queue_mutex_);
      Dequeue(event);
      return false;
    }

    return true;
  }

  /**
   * Signals the Condition and allows one waiting thread
   * to continue execution.
   */
  void NotifyOne();

  /**
   * Signals the Condition and allows all waiting
   * threads to continue their execution.
   */
  void NotifyAll();

  // Disable copy and assignment.
  Condition(const Condition&) = delete;
  Condition& operator = (const Condition&) = delete;

 protected:
  void Enqueue(Event& event);
  void Dequeue();
  void Dequeue(Event& event);

 private:
  using WaitQueue = List<Event*>;

  FastMutex wait_queue_mutex_;
  WaitQueue wait_queue_;
};

} // namespace fun
