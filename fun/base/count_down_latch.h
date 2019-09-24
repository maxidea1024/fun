#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"
#include "fun/base/condition.h"

namespace fun {

class FUN_BASE_API CountDownLatch {
 public:
  explicit CountDownLatch(int32 count);
  
  void Wait();
  void Wait(int32 milliseconds);
  
  void CountDown();
  
  int32 GetCount() const;

  // Disable copy
  CountDownLatch(const CountDownLatch&) = delete;
  CountDownLatch& operator = (const CountDownLatch&) = delete;
    
 private:
  FastMutex mutex_;
  Condition cond_;
  int32 count_;
};

} // namespace fun
