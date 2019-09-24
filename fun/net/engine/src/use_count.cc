#include "NetEnginePrivate.h"
#include "UseCount.h"

namespace fun {
namespace net {

UseCount::UseCount()  : in_use_count_(0) {}

UseCount::~UseCount() {
  fun_check(in_use_count_ == 0);
}

// 이 값이 0을 리턴할 경우 더 이상 이 객체를 참조하는 곳이 없음을 보장한다.
// 이 메서드를 콜 하기 전에 lock(main)이어야 한다.
long UseCount::GetUseCount() {
  return in_use_count_;
}

// 이 메서드를 콜 하기 전에 lock(main)이어야 한다.
// 안그러면 UseCount=0인 순간 다른 스레드에 의해 delete하는 사태 발생.
void UseCount::IncreaseUseCount() {
  Atomics::Increment(&in_use_count_);
  fun_check(in_use_count_);
}

void UseCount::DecreaseUseCount() {
  fun_check(in_use_count_ != 0);
  fun_check(IsLockedByCurrentThread() == false);
  Atomics::Decrement(&in_use_count_);
  fun_check(in_use_count_ >= 0);
}

void UseCount::AssertIsZeroUseCount() {
  fun_check(in_use_count_ >= 0);
}

ScopedUseCounter::ScopedUseCounter(UseCount& in_use_count) {
  counter_ = &in_use_count;
  counter_->IncreaseUseCount();
}

ScopedUseCounter::~ScopedUseCounter() {
  fun_check_ptr(counter_);
  fun_check(counter_->IsLockedByCurrentThread() == false);
  counter_->DecreaseUseCount();
}

} // namespace net
} // namespace fun
