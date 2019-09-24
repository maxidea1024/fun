#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * A class that simplifies thread synchronization with a mutex.
 * The constructor accepts a Mutex (and optionally a timeout value
 * in milliseconds) and locks it.
 *
 * The destructor unlocks the mutex.
 */
template <typename _MutexType>
class ScopedLock {
 public:
  typedef _MutexType MutexType;

  explicit ScopedLock(const MutexType& mutex)
    : mutex_(const_cast<MutexType*>(&mutex)) {
    mutex_->Lock();
  }

  ScopedLock(const MutexType& mutex, int32 milliseconds)
    : mutex_(const_cast<MutexType*>(&mutex)) {
    mutex_->Lock(milliseconds);
  }

  ~ScopedLock() {
    try {
      mutex_->Unlock();
    } catch (...) {
      fun_unexpected();
    }
  }

  // Disable default constructor and copy
  ScopedLock() = delete;
  ScopedLock(const ScopedLock&) = delete;
  ScopedLock& operator = (const ScopedLock&) = delete;

  //void Lock() {
  //  mutex_.Lock();
  //}
  //
  //void Unlock() {
  //  mutex_.Unlock();
  //}

 private:
  MutexType* mutex_;
  //TODO
  bool locked_;
};


template <typename _MutexType>
class ScopedLockWithUnlock {
 public:
  typedef _MutexType MutexType;

  explicit ScopedLockWithUnlock(const MutexType& mutex)
    : mutex_(const_cast<MutexType*>(&mutex)) {
    mutex_->Lock();
  }

  ScopedLockWithUnlock(const MutexType& mutex, int32 milliseconds)
    : mutex_(const_cast<MutexType*>(&mutex)) {
    mutex_->Lock(milliseconds);
  }

  ~ScopedLockWithUnlock() {
    try {
      mutex_->Unlock();
    } catch (...) {
      fun_unexpected();
    }
  }

  //void Lock() {
  //  mutex_->Lock();
  //}
  //

  void Unlock() {
    mutex_->Unlock();
  }

  ScopedLockWithUnlock() = delete;
  ScopedLockWithUnlock(const ScopedLockWithUnlock&) = delete;
  ScopedLockWithUnlock& operator = (const ScopedLockWithUnlock&) = delete;

 private:
  MutexType* mutex_;
};

} // namespace fun
