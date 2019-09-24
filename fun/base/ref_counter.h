#pragma once

#include "fun/base/atomics.h"
#include "fun/base/base.h"

namespace fun {

/*!
\~korean
레퍼런스 초기 값은 다음과 같은 의미를 가짐.

   0 : 공유 불가
  -1 : Persistent (공유는 되지만, 카운팅은 안됨. Ref/Deref가 호출되더라도
무시됨.  주로, 읽기전용으로 공유되는 static 객체들에 사용됨.) 1 : 일반적인 공유
가능 상태. 정상적으로, 카운팅이 됨. (참조 동작에 의해서 증가/증감 후 카운터가
0이 되는 순간 객체 제거)

  어떠한 경우에도, -1 보다 크거나 같아야함.

\~english
The reference initial value has the following meaning.

   0: Not shareable
  -1: Persistent (shared, but not counted, ignored even when Ref / Deref is
called, mainly used for static objects that are shared read-only) 1: Common
shareable state. Normally, counting. (Increase / decrease by reference
operation, remove object at the moment when counter becomes 0)

  In any case, it must be greater than or equal to -1.
*/
class FUN_BASE_API RefCounter {
 public:
  static const int32 PERSISTENT_COUNTER_VALUE = -1;
  static const int32 UNSHARABLE_COUNTER_VALUE = 0;
  static const int32 INITIAL_OWNED_COUNTER_VALUE = 1;

  volatile int32 counter_;

  bool AddRef();
  bool DecRef();

  bool SetSharable(bool sharable);
  bool IsSharable() const;

  bool IsPersistent() const;

  bool IsShared() const;

  int32 GetCounter() const;

  void InitAsOwned();
  void InitAsUnsharable();
};

//
// inlines
//

FUN_ALWAYS_INLINE bool RefCounter::AddRef() {
  const int32 old_counter = counter_;

  // If it is set to unsharable, it does nothing and returns false.
  if (old_counter == UNSHARABLE_COUNTER_VALUE) {
    return false;
  }

  // Do not do reference counting if it is set to persistent.
  // Primarily used for static objects that are shared read-only.
  if (old_counter != PERSISTENT_COUNTER_VALUE) {
    Atomics::Increment(&counter_);
  }

  return true;
}

FUN_ALWAYS_INLINE bool RefCounter::DecRef() {
  const int32 old_counter = counter_;

  // If it is set to unsharable, it does nothing and returns false.
  if (old_counter == UNSHARABLE_COUNTER_VALUE) {
    return false;
  }

  // Do not do reference counting if it is set to persistent.
  // Primarily used for static objects that are shared read-only.
  if (old_counter == PERSISTENT_COUNTER_VALUE) {
    return true;
  }

  fun_check(old_counter > 0);
  return Atomics::Decrement(&counter_) != 0;
}

FUN_ALWAYS_INLINE bool RefCounter::SetSharable(bool sharable) {
  fun_check(IsShared());

  if (sharable) {
    return !!Atomics::CompareExchange(&counter_, 0, 1);
  } else {
    return !!Atomics::CompareExchange(&counter_, 1, 0);
  }
}

FUN_ALWAYS_INLINE bool RefCounter::IsSharable() const {
  // Sharable == Shared ownership.
  return counter_ != UNSHARABLE_COUNTER_VALUE;
}

FUN_ALWAYS_INLINE bool RefCounter::IsPersistent() const {
  return counter_ == PERSISTENT_COUNTER_VALUE;
}

FUN_ALWAYS_INLINE bool RefCounter::IsShared() const {
  return counter_ != INITIAL_OWNED_COUNTER_VALUE &&
         counter_ != UNSHARABLE_COUNTER_VALUE;
}

FUN_ALWAYS_INLINE int32 RefCounter::GetCounter() const { return counter_; }

FUN_ALWAYS_INLINE void RefCounter::InitAsOwned() {
  // Atomics::Exchange(&counter_, INITIAL_OWNED_COUNTER_VALUE);
  counter_ = INITIAL_OWNED_COUNTER_VALUE;
}

FUN_ALWAYS_INLINE void RefCounter::InitAsUnsharable() {
  // Atomics::Exchange(&counter_, UNSHARABLE_COUNTER_VALUE);
  counter_ = UNSHARABLE_COUNTER_VALUE;
}

}  // namespace fun
