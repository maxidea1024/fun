#include "fun/mutex_win32.h"
#include "fun/base/thread.h"
#include "fun/base/timestamp.h"

namespace fun {

//
// MutexImpl
//

MutexImpl::MutexImpl(MutexTypeImpl type)
  : lock_count_(0)
  , recursive_(type == MUTEX_RECURSIVE_IMPL)
{
  // the fct has a boolean return value under WInnNt/2000/XP but not on Win98
  // the return only checks if the input address of &cs_ was valid, so it is safe to omit it
  InitializeCriticalSectionAndSpinCount(&cs_, 4000);
}

MutexImpl::~MutexImpl()
{
  DeleteCriticalSection(&cs_);
}

void MutexImpl::LockImpl()
{
  try {
    EnterCriticalSection(&cs_);
    ++lock_count_;

    if (!recursive_ && lock_count_ > 1) {
      // We're trying to go recursive so self-deadlock
      Thread::Current()->Join();
    }
  }
  catch (...) {
    throw SystemException("cannot lock mutex");
  }
}

bool MutexImpl::TryLockImpl()
{
  try
  {
    if (TryEnterCriticalSection(&cs_) == 0)
    {
      return false;
    }

    if (!recursive_ && lock_count_ > 0)
    {
      LeaveCriticalSection(&cs_);
      return false;
    }

    ++lock_count_;
    return true;
  }
  catch (...)
  {
  }
  throw SystemException("cannot lock mutex");
}

bool MutexImpl::TryLockImpl(long milliseconds)
{
  const int kSleepMillis = 5;
  Timestamp now;
  Timestamp::TimeDiff diff(Timestamp::TimeDiff(milliseconds)*1000);

  do
  {
    try
    {
      if (TryLockImpl())
      {
        return true;
      }
    }
    catch (...)
    {
      throw SystemException("cannot lock mutex");
    }
    Sleep(kSleepMillis);
  } while (!now.IsElapsed(diff));
  return false;
}


//
// FastMutexImpl
//

FastMutexImpl::FastMutexImpl()
{
  // the fct has a boolean return value under WInnNt/2000/XP but not on Win98
  // the return only checks if the input address of &cs_ was valid, so it is safe to omit it
  InitializeCriticalSectionAndSpinCount(&cs_, 4000);
}

FastMutexImpl::~FastMutexImpl()
{
  DeleteCriticalSection(&cs_);
}

bool FastMutexImpl::TryLockImpl(int32 milliseconds)
{
  const int kSleepMillis = 5;
  Timestamp now;
  Timestamp::TimeDiff diff(Timestamp::TimeDiff(milliseconds)*1000);
  do {
    try {
      if (TryEnterCriticalSection(&cs_) == TRUE) {
        return true;
      }
    }
    catch (...) {
      throw SystemException("cannot lock mutex");
    }
    Sleep(kSleepMillis);
  }
  while (!now.IsElapsed(diff));
  return false;
}

} // namespace fun
