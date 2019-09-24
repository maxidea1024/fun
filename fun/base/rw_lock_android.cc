#include "fun/base/rw_lock_android.h"

namespace fun {

RWLockImpl::RWLockImpl() {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  if (pthread_mutex_init(&mutex_, &attr)) {
    pthread_mutexattr_destroy(&attr);
    throw SystemException("cannot create mutex");
  }
  pthread_mutexattr_destroy(&attr);
}

RWLockImpl::~RWLockImpl() { pthread_mutex_destroy(&mutex_); }

}  // namespace fun
