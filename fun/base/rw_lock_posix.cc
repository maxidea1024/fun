#include "fun/base/rw_lock_posix.h"

namespace fun {

RWLockImpl::RWLockImpl() {
  if (pthread_rwlock_init(&rwl_, NULL)) {
    throw SystemException("cannot create reader/writer lock");
  }
}

RWLockImpl::~RWLockImpl() {
  pthread_rwlock_destroy(&rwl_);
}

} // namespace fun
