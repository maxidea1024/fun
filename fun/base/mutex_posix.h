#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

#include <pthread.h>
#include <errno.h>

namespace fun {

class FUN_BASE_API MutexImpl {
 public:
  enum MutexTypeImpl {
    MUTEX_RECURSIVE_IMPL,
    MUTEX_NONRECURSIVE_IMPL
  };

 protected:
  explicit MutexImpl(MutexTypeImpl type);
  ~MutexImpl();

  void LockImpl();
  bool TryLockImpl();
  bool TryLockImpl(int32 milliseconds);
  void UnlockImpl();

 private:
  pthread_mutex_t mutex_;
};


class FUN_BASE_API FastMutexImpl : public MutexImpl {
 protected:
  FastMutexImpl();
  ~FastMutexImpl();
};


//
// inlines
//

FUN_ALWAYS_INLINE void MutexImpl::LockImpl() {
  if (pthread_mutex_lock(&mutex_)) {
    throw SystemException("cannot Lock mutex");
  }
}

FUN_ALWAYS_INLINE bool MutexImpl::TryLockImpl() {
  int rc = pthread_mutex_trylock(&mutex_);
  if (rc == 0) {
    return true;
  } else if (rc == EBUSY) {
    return false;
  } else {
    throw SystemException("cannot Lock mutex");
  }
}

FUN_ALWAYS_INLINE void MutexImpl::UnlockImpl() {
  if (pthread_mutex_unlock(&mutex_)) {
    throw SystemException("cannot Unlock mutex");
  }
}

} // namespace fun
