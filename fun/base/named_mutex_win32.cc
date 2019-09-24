#include "fun/base/named_mutex_win32.h"
#include "fun/base/exception.h"

namespace fun {

NamedMutexImpl::NamedMutexImpl(const String& name)
  : name_(name) {
  uname_ = UString::FromUtf8(name);
  mutex_ = CreateMutexW(NULL, FALSE, uname_.c_str());
  if (!mutex_) {
    throw SystemException("cannot create named mutex", name_);
  }
}

NamedMutexImpl::~NamedMutexImpl() {
  CloseHandle(mutex_);
}

void NamedMutexImpl::LockImpl() {
  switch (WaitForSingleObject(mutex_, INFINITE)) {
    case WAIT_OBJECT_0:
      return;
    case WAIT_ABANDONED:
      throw SystemException("cannot lock named mutex (abadoned)", name_);
    default:
      throw SystemException("cannot lock named mutex", name_);
  }
}

bool NamedMutexImpl::TryLockImpl() {
  switch (WaitForSingleObject(mutex_, 0)) {
    case WAIT_OBJECT_0:
      return true;
    case WAIT_TIMEOUT:
      return false;
    case WAIT_ABANDONED:
      throw SystemException("cannot lock named mutex (abadoned)", name_);
    default:
      throw SystemException("cannot lock named mutex", name_);
  }
}

void NamedMutexImpl::UnlockImpl() {
  ReleaseMutex(mutex_);
}

} // namespace fun
