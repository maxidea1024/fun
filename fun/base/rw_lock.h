#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

// TODO: std::shared_timed_mutex has separate read and write Unlock

//#define FUN_CXX11_RWLOCK_FINISHED

#if defined(FUN_CXX11_RWLOCK_FINISHED) && defined(FUN_ENABLE_CPP14)
#include "fun/base/rw_lock_std.h"
#elif FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/rw_lock_wince.h"
#else
#include "fun/base/rw_lock_win32.h"
#endif
#elif FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/rw_lock_android.h"
#elif FUN_PLATFORM == FUN_PLATFORM_VXWORKS
#include "fun/base/rw_lock_vx.h"
#else
#include "fun/base/rw_lock_posix.h"
#endif

namespace fun {

class ScopedRWLock;
class ScopedReadRWLock;
class ScopedWriteRWLock;

/**
 * A reader writer lock allows multiple concurrent
 * readers or one exclusive writer.
 */
class FUN_BASE_API RWLock : private RWLockImpl {
 public:
  typedef ScopedRWLock ScopedLock;
  typedef ScopedReadRWLock ScopedReadLock;
  typedef ScopedWriteRWLock ScopedWriteLock;

  /**
   * Creates the Reader/Writer lock.
   */
  RWLock();

  /**
   * Destroys the Reader/Writer lock.
   */
  ~RWLock();

  /**
   * Acquires a read lock. If another thread currently holds a write lock,
   * waits until the write lock is released.
   */
  void ReadLock();

  /**
   * Tries to acquire a read lock. Immediately returns true if successful, or
   * false if another thread currently holds a write lock.
   */
  bool TryReadLock();

  /**
   * Acquires a write lock. If one or more other threads currently hold
   * locks, waits until all locks are released. The results are undefined
   * if the same thread already holds a read or write lock
   */
  void WriteLock();

  /**
   * Tries to acquire a write lock. Immediately returns true if successful,
   * or false if one or more other threads currently hold
   * locks. The result is undefined if the same thread already
   * holds a read or write lock.
   */
  bool TryWriteLock();

  /**
   * Releases the read or write lock.
   */
  void Unlock();

 private:
  RWLock(const RWLock&) = delete;
  RWLock& operator = (const RWLock&) = delete;
};


/**
 * A variant of ScopedLock for reader/writer locks.
 */
class FUN_BASE_API ScopedRWLock {
 public:
  ScopedRWLock(RWLock& rwl, bool write = false);
  ~ScopedRWLock();

  ScopedRWLock() = delete;
  ScopedRWLock(const ScopedRWLock&) = delete;
  ScopedRWLock& operator = (const ScopedRWLock&) = delete;

 private:
  RWLock& rwl_;
};


/**
 * A variant of ScopedLock for reader locks.
 */
class FUN_BASE_API ScopedReadRWLock : public ScopedRWLock {
 public:
  ScopedReadRWLock(RWLock& rwl);
  ~ScopedReadRWLock();
};


/**
 * A variant of ScopedLock for writer locks.
 */
class FUN_BASE_API ScopedWriteRWLock : public ScopedRWLock {
 public:
  ScopedWriteRWLock(RWLock& rwl);
  ~ScopedWriteRWLock();
};


//
// inlines
//

FUN_ALWAYS_INLINE void RWLock::ReadLock() {
  ReadLockImpl();
}

FUN_ALWAYS_INLINE bool RWLock::TryReadLock() {
  return TryReadLockImpl();
}

FUN_ALWAYS_INLINE void RWLock::WriteLock() {
  WriteLockImpl();
}

FUN_ALWAYS_INLINE bool RWLock::TryWriteLock() {
  return TryWriteLockImpl();
}

FUN_ALWAYS_INLINE void RWLock::Unlock() {
  UnlockImpl();
}

FUN_ALWAYS_INLINE ScopedRWLock::ScopedRWLock(RWLock& rwl, bool write)
  : rwl_(rwl) {
  if (write) {
    rwl_.WriteLock();
  } else {
    rwl_.ReadLock();
  }
}

FUN_ALWAYS_INLINE ScopedRWLock::~ScopedRWLock() {
  try {
    rwl_.Unlock();
  } catch (...) {
    fun_unexpected();
  }
}


FUN_ALWAYS_INLINE ScopedReadRWLock::ScopedReadRWLock(RWLock& rwl)
  : ScopedRWLock(rwl, false) {}

FUN_ALWAYS_INLINE ScopedReadRWLock::~ScopedReadRWLock() {}

FUN_ALWAYS_INLINE ScopedWriteRWLock::ScopedWriteRWLock(RWLock& rwl)
  : ScopedRWLock(rwl, true) {}

FUN_ALWAYS_INLINE ScopedWriteRWLock::~ScopedWriteRWLock() {}

} // namespace fun
