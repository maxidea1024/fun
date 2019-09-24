#pragma once

#include "fun/base/base.h"
#include "fun/base/timestamp.h"
#include "fun/base/exception.h"
#include "fun/base/scoped_lock.h"
#include "fun/base/scoped_unlock.h"
#include "fun/base/lockable.h"

#include <atomic>

#if FUN_PLATFORM == FUN_PLATFORM_CYGWIN || FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/mutex_posix.h"
#else
#include "fun/base/mutex_std.h"
#endif

namespace fun {

/**
 * A Mutex (mutual exclusion) is a synchronization
 * mechanism used to control access to a shared resource
 * in a concurrent (multithreaded) scenario.
 * Using the ScopedLock class is the preferred way to automatically
 * lock and unlock a mutex.
 */
class FUN_BASE_API Mutex : private MutexImpl {
 public:
  enum MutexType {
    MUTEX_RECURSIVE = MUTEX_RECURSIVE_IMPL,      /// A recursive mutex
    MUTEX_NONRECURSIVE = MUTEX_NONRECURSIVE_IMPL /// A non-recursive mutex
  };

  typedef fun::ScopedLock<Mutex> ScopedLock;
  typedef fun::ScopedUnlock<Mutex> ScopedUnlock;

  //TODO Tag로 처리하는게 좋지 아니한가??
  /**
   * Creates the Mutex.
   */
  explicit Mutex(MutexType type = MUTEX_RECURSIVE);

  /**
   * Destroys the Mutex.
   */
  ~Mutex();

  /**
   * Locks the mutex. Blocks if the mutex is held by another thread.
   */
  void Lock();

  /**
   * Locks the mutex. Blocks up to the given number of milliseconds
   * if the mutex is held by another thread. Throws a TimeoutException
   * if the mutex can not be locked within the given timeout.
   *
   * Performance Note: On most platforms (including Windows), this member function is
   * implemented using a loop calling (the equivalent of) TryLock() and Thread::Sleep().
   * On POSIX platforms that support pthread_mutex_timedlock(), this is used.
   */
  void Lock(int32 milliseconds);

  /**
   * Tries to lock the mutex. Returns false immediately
   * if the mutex is already held by another thread.
   * Returns true if the mutex was successfully locked.
   */
  bool TryLock();

  /**
   * Locks the mutex. Blocks up to the given number of milliseconds
   * if the mutex is held by another thread.
   * Returns true if the mutex was successfully locked.
   *
   * Performance Note: On most platforms (including Windows), this member function is
   * implemented using a loop calling (the equivalent of) TryLock() and Thread::Sleep().
   * On POSIX platforms that support pthread_mutex_timedlock(), this is used.
   */
  bool TryLock(int32 milliseconds);

  /**
   * Unlocks the mutex so that it can be acquired by other threads.
   */
  void Unlock();

  /*
  //TODO 아래 두 함수를 추가해야함.
  //TID등의 타입을 노출하기 위해서는, thread_base.h같은 파일이 있어야할듯...
  //현재는 mutex.h, thread.h 두 헤더 파일이 순환참조를 하고 있음.
  TID GetOwnerTid() const {
    return owner_tid_;
  }

  bool IsLockedByThisThread() const {
    return owner_tid_ == Thread::CurrentTid();
  }
  */

 private:
  //TID owner_tid_;

  Mutex(const Mutex&) = delete;
  Mutex& operator = (const Mutex&) = delete;
};


/**
 * A FastMutex (mutual exclusion) is similar to a Mutex.
 * Locking a FastMutex is guaranteed to be at least as
 * fast as locking a Mutex.  However, a FastMutex is not
 * guaranteed to be either recursive or non-recursive.
 * It is best suited to thread safe components like pools,
 * caches and queues where locking is internal to the component.
 * Using the ScopedLock class is the preferred way to automatically
 * lock and unlock a mutex.
 */
class FUN_BASE_API FastMutex : private FastMutexImpl {
 public:
  typedef fun::ScopedLock<FastMutex> ScopedLock;
  typedef fun::ScopedUnlock<FastMutex> ScopedUnlock;

  /**
   * Creates the Mutex.
   */
  FastMutex();

  /**
   * Destroys the Mutex.
   */
  ~FastMutex();

  /**
   * Locks the mutex. Blocks if the mutex is held by another thread.
   */
  void Lock();

  /**
   * Locks the mutex. Blocks up to the given number of milliseconds
   * if the mutex is held by another thread. Throws a TimeoutException
   * if the mutex can not be locked within the given timeout.
   *
   * Performance Note: On most platforms (including Windows), this member function is
   * implemented using a loop calling (the equivalent of) TryLock() and Thread::Sleep().
   * On POSIX platforms that support pthread_mutex_timedlock(), this is used.
   */
  void Lock(int32 milliseconds);

  /**
   * Tries to lock the mutex. Returns false immediately
   * if the mutex is already held by another thread.
   * Returns true if the mutex was successfully locked.
   */
  bool TryLock();

  /**
   * Locks the mutex. Blocks up to the given number of milliseconds
   * if the mutex is held by another thread.
   * Returns true if the mutex was successfully locked.
   *
   * Performance Note: On most platforms (including Windows), this member function is
   * implemented using a loop calling (the equivalent of) TryLock() and Thread::Sleep().
   * On POSIX platforms that support pthread_mutex_timedlock(), this is used.
   */
  bool TryLock(int32 milliseconds);

  /**
   * Unlocks the mutex so that it can be acquired by other threads.
   */
  void Unlock();

  /*
  //TODO 아래 두 함수를 추가해야함.
  //TID등의 타입을 노출하기 위해서는, thread_base.h같은 파일이 있어야할듯...
  //현재는 mutex.h, thread.h 두 헤더 파일이 순환참조를 하고 있음.
  TID GetOwnerTid() const {
    return owner_tid_;
  }

  bool IsLockedByThisThread() const {
    return owner_tid_ == Thread::CurrentTid();
  }
  */

 private:
  //TID owner_tid_;

  FastMutex(const FastMutex&) = delete;
  FastMutex& operator = (const FastMutex&) = delete;
};


/**
 * A SpinlockMutex, implemented in terms of std::atomic_flag, as
 * busy-wait mutual exclusion.
 *
 * While in some cases (eg. locking small blocks of code)
 * busy-waiting may be an optimal solution, in many scenarios
 * spinlock may not be the right choice - it is up to the user to
 * choose the proper mutex type for their particular case.
 *
 * Works with the ScopedLock class.
 */
class FUN_BASE_API SpinlockMutex {
 public:
  typedef fun::ScopedLock<SpinlockMutex> ScopedLock;
  typedef fun::ScopedUnlock<SpinlockMutex> ScopedUnlock;

  /**
   * Creates the SpinlockMutex.
   */
  SpinlockMutex();

  /**
   * Destroys the SpinlockMutex.
   */
  ~SpinlockMutex();

  /**
   * Locks the mutex. Blocks if the mutex is held by another thread.
   */
  void Lock();

  /**
   * Locks the mutex. Blocks up to the given number of milliseconds
   * if the mutex is held by another thread. Throws a TimeoutException
   * if the mutex can not be locked within the given timeout.
   */
  void Lock(int32 milliseconds);

  /**
   * Tries to lock the mutex. Returns immediately, false
   * if the mutex is already held by another thread, true
   * if the mutex was successfully locked.
   */
  bool TryLock();

  /**
   * Locks the mutex. Blocks up to the given number of milliseconds
   * if the mutex is held by another thread.
   * Returns true if the mutex was successfully locked.
   */
  bool TryLock(int32 milliseconds);

  /**
   * Unlocks the mutex so that it can be acquired by other threads.
   */
  void Unlock();

  /*
  //TODO 아래 두 함수를 추가해야함.
  //TID등의 타입을 노출하기 위해서는, thread_base.h같은 파일이 있어야할듯...
  //현재는 mutex.h, thread.h 두 헤더 파일이 순환참조를 하고 있음.
  TID GetOwnerTid() const {
    return owner_tid_;
  }

  bool IsLockedByThisThread() const {
    return owner_tid_ == Thread::CurrentTid();
  }
  */

 private:
  //TID owner_tid_;

  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};


/**
 * A NullMutex is an empty mutex implementation
 * which performs no locking at all. Useful in policy driven design
 * where the type of mutex used can be now a template parameter allowing the user to switch
 * between thread-safe and not thread-safe depending on his need
 * Works with the ScopedLock class
 */
class FUN_BASE_API NullMutex {
 public:
  typedef fun::ScopedLock<NullMutex> ScopedLock;
  typedef fun::ScopedUnlock<NullMutex> ScopedUnlock;

  /**
   * Creates the NullMutex.
   */
  NullMutex() {}

  /**
   * Destroys the NullMutex.
   */
  ~NullMutex() {}

  /**
   * Does nothing.
   */
  void Lock() {}

  /**
   * Does nothing.
   */
  void Lock(int32) {}

  /**
   * Does nothing and always returns true.
   */
  bool TryLock() { return true; }

  /**
   * Does nothing and always returns true.
   */
  bool TryLock(int32) { return true; }

  /**
   * Does nothing.
   */
  void Unlock() {}

  /*
  //TODO 아래 두 함수를 추가해야함.
  //TID등의 타입을 노출하기 위해서는, thread_base.h같은 파일이 있어야할듯...
  //현재는 mutex.h, thread.h 두 헤더 파일이 순환참조를 하고 있음.
  TID GetOwnerTid() const {
    return owner_tid_;
  }

  bool IsLockedByThisThread() const {
    return owner_tid_ == Thread::CurrentTid();
  }
  */
};



//
// inlines
//

//
// Mutex
//

FUN_ALWAYS_INLINE void Mutex::Lock() {
  LockImpl();
  //SetOwnerThread();
}

FUN_ALWAYS_INLINE void Mutex::Lock(int32 milliseconds) {
  if (!TryLockImpl(milliseconds)) {
    throw TimeoutException();
  }
  //SetOwnerThread();
}

FUN_ALWAYS_INLINE bool Mutex::TryLock() {
  bool locked = TryLockImpl();
  if (locked) {
    //SetOwnerThread();
  }
  return locked;
}

FUN_ALWAYS_INLINE bool Mutex::TryLock(int32 milliseconds) {
  bool locked = TryLockImpl(milliseconds);
  if (!locked) {
    return false;
  }

  //SetOwnerThread();
  return true;
}

FUN_ALWAYS_INLINE void Mutex::Unlock() {
  //ClearOwnerThread();
  UnlockImpl();
}


//
// FastMutex
//

FUN_ALWAYS_INLINE void FastMutex::Lock() {
  LockImpl();
  //SetOwnerThread();
}

FUN_ALWAYS_INLINE void FastMutex::Lock(int32 milliseconds) {
  if (!TryLockImpl(milliseconds)) {
    throw TimeoutException();
  }
  //SetOwnerThread();
}

FUN_ALWAYS_INLINE bool FastMutex::TryLock() {
  bool locked = TryLockImpl();
  if (locked) {
    //SetOwnerThread();
  }
  return locked;
}

FUN_ALWAYS_INLINE bool FastMutex::TryLock(int32 milliseconds) {
  bool locked = TryLockImpl(milliseconds);
  if (!locked) {
    return false;
  }

  //SetOwnerThread();
  return true;
}

FUN_ALWAYS_INLINE void FastMutex::Unlock() {
  UnlockImpl();
}


//
// SpinlockMutex
//

FUN_ALWAYS_INLINE void SpinlockMutex::Lock() {
  while (flag_.test_and_set(std::memory_order_acquire)) ;
}

FUN_ALWAYS_INLINE void SpinlockMutex::Lock(int32 milliseconds) {
  Timestamp now(Timestamp::Now());
  Timestamp::TimeDiff diff(Timestamp::TimeDiff(milliseconds)*1000);
  while (flag_.test_and_set(std::memory_order_acquire)) {
    if (now.IsElapsed(diff)) {
      throw TimeoutException();
    }
  }
}

FUN_ALWAYS_INLINE bool SpinlockMutex::TryLock() {
  return !flag_.test_and_set(std::memory_order_acquire);
}

FUN_ALWAYS_INLINE bool SpinlockMutex::TryLock(int32 milliseconds) {
  Timestamp now(Timestamp::Now());
  Timestamp::TimeDiff diff(Timestamp::TimeDiff(milliseconds)*1000);
  while (flag_.test_and_set(std::memory_order_acquire)) {
    if (now.IsElapsed(diff)) {
      return false;
    }
  }
  return true;
}

FUN_ALWAYS_INLINE void SpinlockMutex::Unlock() {
  flag_.clear(std::memory_order_release);
}

} // namespace fun
