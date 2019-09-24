#pragma once

#include "fun/base/base.h"
#include "fun/base/timestamp.h"
#include "fun/base/exception.h"
#include "fun/base/scoped_lock.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/mutex_win32.h"
#else
#include "fun/base/mutex_posix.h"
#endif

namespace fun {

/**
 * A Mutex (mutual exclusion) is a synchronization
 * mechanism used to control access to a shared resource
 * in a concurrent (multithreaded) scenario.
 * Using the ScopedLock class is the preferred way to automatically
 * lock and unlock a mutex.
 */
class FUN_BASE_API Mutex : private MutexImpl
{
 public:
  enum MutexType
  {
    /** A recursive mutex */
    MUTEX_RECURSIVE = MUTEX_RECURSIVE_IMPL,
    /** A non-recursive mutex */
    MUTEX_NONRECURSIVE = MUTEX_NONRECURSIVE_IMPL
  };

  typedef fun::ScopedLock<Mutex> ScopedLock;
  typedef fun::ScopedUnlock<Mutex> ScopedUnlock;

  //TODO Tag로 처리하는게 좋지 아니한가??
  explicit Mutex(MutexType type = MUTEX_RECURSIVE);
  ~Mutex();

  void Lock();
  void Lock(int32 milliseconds);
  bool TryLock();
  bool TryLock(int32 milliseconds);
  void Unlock();

 private:
  Mutex(const Mutex&);
  Mutex& operator = (const Mutex&);
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
class FUN_BASE_API FastMutex : private FastMutexImpl
{
 public:
  typedef fun::ScopedLock<FastMutex> ScopedLock;
  typedef fun::ScopedUnlock<FastMutex> ScopedUnlock;

  FastMutex();
  ~FastMutex();

  void Lock();
  void Lock(int32 milliseconds);
  bool TryLock();
  bool TryLock(int32 milliseconds);
  void Unlock();

 private:
  FastMutex(const FastMutex&);
  FastMutex& operator = (const FastMutex&);
};


/**
 * A NullMutex is an empty mutex implementation
 * which performs no locking at all. Useful in policy driven design
 * where the type of mutex used can be now a template parameter allowing the user to switch
 * between thread-safe and not thread-safe depending on his need
 * Works with the ScopedLock class
 */
class FUN_BASE_API NullMutex
{
 public:
  typedef fun::ScopedLock<NullMutex> ScopedLock;
  typedef fun::ScopedUnlock<NullMutex> ScopedUnlock;

  NullMutex() {}
  ~NullMutex() {}

  void Lock() {}
  void Lock(int32) {}
  bool TryLock() { return true; }
  bool TryLock(int32) { return true; }
  void Unlock() {}
};


//
// inlines
//

//
// Mutex
//

inline void Mutex::Lock()
{
  LockImpl();
}

inline void Mutex::Lock(int32 milliseconds)
{
  if (!TryLockImpl(milliseconds)) {
    throw TimeoutException();
  }
}

inline bool Mutex::TryLock()
{
  return TryLockImpl();
}

inline bool Mutex::TryLock(int32 milliseconds)
{
  return TryLockImpl(milliseconds);
}

inline void Mutex::Unlock()
{
  UnlockImpl();
}


//
// FastMutex
//

inline void FastMutex::Lock()
{
  LockImpl();
}

inline void FastMutex::Lock(int32 milliseconds)
{
  if (!TryLockImpl(milliseconds)) {
    throw TimeoutException();
  }
}

inline bool FastMutex::TryLock()
{
  return TryLockImpl();
}

inline bool FastMutex::TryLock(int32 milliseconds)
{
  return TryLockImpl(milliseconds);
}

inline void FastMutex::Unlock()
{
  UnlockImpl();
}

} // namespace fun
