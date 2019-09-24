#include "fun/base/named_mutex_android.h"
#include "fun/base/exception.h"

namespace fun {

NamedMutexImpl::NamedMutexImpl(const String&) {
}

NamedMutexImpl::~NamedMutexImpl() {
}

void NamedMutexImpl::LockImpl() {
  throw NotImplementedException("NamedMutex::Lock() is not supported on Android");
}

bool NamedMutexImpl::TryLockImpl() {
  throw NotImplementedException("NamedMutex::TryLock() is not supported on Android");
}

void NamedMutexImpl::UnlockImpl() {
  throw NotImplementedException("NamedMutex::Unlock() is not supported on Android");
}

} // namespace fun
