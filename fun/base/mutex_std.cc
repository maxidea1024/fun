#include "fun/base/mutex_std.h"
#include "fun/base/timestamp.h"

#include <thread>

namespace fun {

MutexImpl::MutexImpl(MutexTypeImpl type)
  : mutex_(type == MUTEX_RECURSIVE_IMPL ?
    std::unique_ptr<MutexImpl_BaseMutex>(new MutexImpl_MutexI<std::recursive_timed_mutex>()) :
    std::unique_ptr<MutexImpl_BaseMutex>(new MutexImpl_MutexI<std::timed_mutex>())) {}

bool MutexImpl::TryLockImpl(int32 milliseconds) {
  const int kSleepMillis = 5;
  Timestamp now;
  Timestamp::TimeDiff diff(Timestamp::TimeDiff(milliseconds)*1000);

  do {
    try {
      if (mutex_->TryLock(milliseconds)) {
        return true;
      }
    } catch (...) {
      throw SystemException("cannot Lock mutex");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMillis));
  } while (!now.IsElapsed(diff));

  return false;
}

FastMutexImpl::FastMutexImpl() : mutex_() {}

bool FastMutexImpl::TryLockImpl(int32 milliseconds) {
  const int kSleepMillis = 5;
  Timestamp now(Timestamp::Now());
  Timestamp::TimeDiff diff(Timestamp::TimeDiff(milliseconds)*1000);
  do {
    try {
      if (mutex_.try_lock_for(std::chrono::milliseconds(milliseconds))) {
        return true;
      }
    } catch (...) {
      throw SystemException("cannot Lock mutex");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMillis));
  }
  while (!now.IsElapsed(diff));
  return false;
}

} // namespace fun
