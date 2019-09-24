#pragma once

#include "fun/base/base.h"
#include "fun/base/scoped_lock.h"


#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/named_mutex_win32.h"
#elif FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/named_mutex_android.h"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/named_mutex_unix.h"
#endif

namespace fun {

/**
 * A NamedMutex (mutual exclusion) is a global synchronization
 * mechanism used to control access to a shared resource
 * in a concurrent (multi process) scenario.
 * Using the ScopedLock class is the preferred way to automatically
 * lock and unlock a mutex.
 * 
 * Unlike a Mutex or a FastMutex, which itself is the unit of synchronization,
 * a NamedMutex refers to a named operating system resource being the
 * unit of synchronization.
 * In other words, there can be multiple instances of NamedMutex referring
 * to the same actual synchronization object.
 * 
 * 
 * There should not be more than one instance of NamedMutex for
 * a given name in a process. Otherwise, the instances may
 * interfere with each other.
 */
class FUN_BASE_API NamedMutex : private NamedMutexImpl {
 public:
  typedef fun::ScopedLock<NamedMutex> ScopedLock;

  /**
   * Creates the Mutex.
   */
  NamedMutex(const String& name);

  /**
   * Destroys the Mutex.
   */
  ~NamedMutex();

  /**
   * Locks the mutex. Blocks if the mutex
   * is held by another process or thread.
   */
  void Lock();

  /**
   * Tries to lock the mutex. Returns false immediately
   * if the mutex is already held by another process or thread.
   * Returns true if the mutex was successfully locked.
   */
  bool TryLock();

  /**
   * Unlocks the mutex so that it can be acquired by
   * other threads.
   */
  void Unlock();

 private:
  NamedMutex();
  NamedMutex(const NamedMutex&);
  NamedMutex& operator = (const NamedMutex&);
};



//
// inlines
//

FUN_ALWAYS_INLINE void NamedMutex::Lock() {
  LockImpl();
}

FUN_ALWAYS_INLINE bool NamedMutex::TryLock() {
  return TryLockImpl();
}

FUN_ALWAYS_INLINE void NamedMutex::Unlock() {
  UnlockImpl();
}

} // namespace fun
