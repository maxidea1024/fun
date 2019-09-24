#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"
#include "fun/base/windows_less.h"

namespace fun {

class FUN_BASE_API MutexImpl
{
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
  CRITICAL_SECTION cs_;
  int32 lock_count_;
  const bool recursive_;

 private:
  MutexImpl(const MutexImpl&);
  MutexImpl& operator = (const MutexImpl&);
};


class FUN_BASE_API FastMutexImpl
{
 protected:
  FastMutexImpl();
  ~FastMutexImpl();

  void LockImpl();
  bool TryLockImpl();
  bool TryLockImpl(int32 milliseconds);
  void UnlockImpl();

 private:
  CRITICAL_SECTION cs_;
};


//
// inlines
//

inline void MutexImpl::UnlockImpl()
{
  --_lockCount;
  LeaveCriticalSection(&cs_);
}

inline void FastMutexImpl::LockImpl()
{
  try {
    EnterCriticalSection(&cs_);
  }
  catch (...) {
    throw SystemException("cannot lock mutex");
  }
}

inline void FastMutexImpl::UnlockImpl()
{
  LeaveCriticalSection(&cs_);
}

inline bool FastMutexImpl::TryLockImpl()
{
  try {
    return TryEnterCriticalSection(&cs_) != 0;
  }
  catch (...) {
  }
  throw SystemException("cannot lock mutex");
}

} // namespace fun
