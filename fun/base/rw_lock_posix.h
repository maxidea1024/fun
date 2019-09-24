#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

#include <errno.h>
#include <pthread.h>

namespace fun {

class FUN_BASE_API RWLockImpl {
 protected:
  RWLockImpl();
  ~RWLockImpl();

  void ReadLockImpl();
  bool TryReadLockImpl();
  void WriteLockImpl();
  bool TryWriteLockImpl();
  void UnlockImpl();

 private:
  pthread_rwlock_t rwl_;
};

//
// inlines
//

inline void RWLockImpl::ReadLockImpl() {
  if (pthread_rwlock_rdlock(&rwl_)) {
    throw SystemException("cannot lock reader/writer lock");
  }
}

inline bool RWLockImpl::TryReadLockImpl() {
  int rc = pthread_rwlock_tryrdlock(&rwl_);
  if (rc == 0) {
    return true;
  } else if (rc == EBUSY) {
    return false;
  } else {
    throw SystemException("cannot lock reader/writer lock");
  }
}

inline void RWLockImpl::WriteLockImpl() {
  if (pthread_rwlock_wrlock(&rwl_)) {
    throw SystemException("cannot lock reader/writer lock");
  }
}

inline bool RWLockImpl::TryWriteLockImpl() {
  int rc = pthread_rwlock_trywrlock(&rwl_);
  if (rc == 0) {
    return true;
  } else if (rc == EBUSY) {
    return false;
  } else {
    throw SystemException("cannot lock reader/writer lock");
  }
}

inline void RWLockImpl::UnlockImpl() {
  if (pthread_rwlock_unlock(&rwl_)) {
    throw SystemException("cannot unlock mutex");
  }
}

}  // namespace fun
