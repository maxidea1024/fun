#include "fun/base/count_down_latch.h"

namespace fun {

CountDownLatch::CountDownLatch(int32 count)
  : count_(count),
    mutex_(),
    cond_() {
  fun_check(count_ >= 0);
}

void CountDownLatch::Wait() {
  ScopedLock<FastMutex> guard(mutex_);

  while (count_ > 0) {
    cond_.Wait(mutex_);
  }
}

void CountDownLatch::Wait(int32 milliseconds) {
  ScopedLock<FastMutex> guard(mutex_);

  //TODO 조건에 맞추어서 대기하는게 바람직할듯...
  while (count_ > 0) {
    cond_.Wait(mutex_, milliseconds);
  }
}

void CountDownLatch::CountDown() {
  ScopedLock<FastMutex> guard(mutex_);

  if (--count_ == 0) {
    cond_.NotifyAll();
  }
}

int32 CountDownLatch::GetCount() const {
  ScopedLock<FastMutex> guard(mutex_);

  return count_;
}

} // namespace fun
