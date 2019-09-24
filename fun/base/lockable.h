#pragma once

#include "fun/base/base.h"

namespace fun {

// TODO 기본을 FastMutex, Mutex 어떤걸로 하는게 유리할까??
class FastMutex;

template <typename T, typename MutexType = FastMutex>
class Lockable : public T, public MutexType {
 public:
  template <typename... Args>
  FUN_ALWAYS_INLINE Lockable(Args&... args) : T(args...) {}

  template <typename OtherType>
  FUN_ALWAYS_INLINE T& operator=(const OtherType& other) {
    *(T*)this = other;
    return *this;
  }
};

template <typename MutexType>
class Lockable<void, MutexType> : public MutexType {};

}  // namespace fun
