#include "fun/base/rw_lock_std.h"
//#include "fun/base/exception.h"

namespace fun {

RWLockImpl::RWLockImpl() : mutex_() {}

void RWLockImpl::ReadLockImpl() {
  mutex_.lock_shared();
}

bool RWLockImpl::TryReadLockImpl() {
  return mutex_.try_lock_shared();
}

void RWLockImpl::WriteLockImpl() {
  mutex_.lock();
}

bool RWLockImpl::TryWriteLockImpl() {
  return mutex_.try_lock();
}

void RWLockImpl::UnlockImpl() {
  // TODO: unlock_shared()?
  mutex_.unlock();
}

} // namespace fun
