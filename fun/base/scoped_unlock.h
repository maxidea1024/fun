#pragma once

#include "fun/base/base.h"

namespace fun {

template <typename _MutexType>
class ScopedUnlock {
 public:
  typedef _MutexType MutexType;

  explicit ScopedUnlock(const MutexType& mutex, bool initial_unlock = true)
    : mutex_(const_cast<MutexType*>(&mutex)) {
    if (initial_unlock) {
      mutex_->Unlock();
    }
  }

  ~ScopedUnlock() {
    try {
      mutex_->Lock();
    } catch (...) {
      fun_unexpected();
    }
  }

  // Disable default constructor and copy.
  ScopedUnlock() = delete;
  ScopedUnlock(const ScopedUnlock&) = delete;
  ScopedUnlock& operator = (const ScopedUnlock&) = delete;

 private:
  MutexType* mutex_;
};

} // namespace fun
