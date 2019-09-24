#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

#include <mutex>
#include <chrono>
#include <memory>

namespace fun {

class FUN_BASE_API MutexImpl_BaseMutex {
 public:
  virtual ~MutexImpl_BaseMutex() {}

  virtual void Lock() = 0;
  virtual bool TryLock() = 0;
  virtual bool TryLock(int32 milliseconds) = 0;
  virtual void Unlock() = 0;
};


template <typename T>
class MutexImpl_MutexI : public MutexImpl_BaseMutex {
 public:
  MutexImpl_MutexI() : mutex_() {}

  void Lock() {
    mutex_.lock();
  }

  bool TryLock() {
    return mutex_.try_lock();
  }

  bool TryLock(int32 milliseconds) {
    return mutex_.try_lock_for(std::chrono::milliseconds(milliseconds));
  }

  void Unlock() {
    mutex_.unlock();
  }

 private:
  T mutex_;
};


class FUN_BASE_API MutexImpl {
 public:
  enum MutexTypeImpl {
    MUTEX_RECURSIVE_IMPL,
    MUTEX_NONRECURSIVE_IMPL,
  };

  MutexImpl(const MutexImpl&) = delete;
  MutexImpl& operator = (const MutexImpl&) = delete;

 protected:
  explicit MutexImpl(MutexTypeImpl type);

  void LockImpl();
  bool TryLockImpl();
  bool TryLockImpl(int32 milliseconds);
  void UnlockImpl();

 private:
  std::unique_ptr<MutexImpl_BaseMutex> mutex_;
};


class FUN_BASE_API FastMutexImpl {
 protected:
  FastMutexImpl();

  void LockImpl();
  bool TryLockImpl();
  bool TryLockImpl(int32 milliseconds);
  void UnlockImpl();

 private:
  std::timed_mutex mutex_;
};


//
// inlines
//

FUN_ALWAYS_INLINE void MutexImpl::LockImpl() {
  mutex_->Lock();
}

FUN_ALWAYS_INLINE bool MutexImpl::TryLockImpl() {
  return mutex_->TryLock();
}

FUN_ALWAYS_INLINE void MutexImpl::UnlockImpl() {
  mutex_->Unlock();
}

FUN_ALWAYS_INLINE void FastMutexImpl::LockImpl() {
  mutex_.lock();
}

FUN_ALWAYS_INLINE bool FastMutexImpl::TryLockImpl() {
  return mutex_.try_lock();
}

FUN_ALWAYS_INLINE void FastMutexImpl::UnlockImpl() {
  mutex_.unlock();
}

} // namespace fun
