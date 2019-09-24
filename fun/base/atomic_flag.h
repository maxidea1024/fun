#pragma once

#include "fun/base/base.h"
#include <atomic>

namespace fun {

/**
* This class implements an atomic boolean flag by wrapping
* the std::atomic_flag. It is guaranteed to be thread-safe
* and lock-free.
* 
* Only default-construction is allowed, objects of this
* class are not copyable, assignable or movable.
* 
* Note that this class is not a replacement for (atomic)
* bool type because its value can not be read without also
* being set to true. However, after being set (either
* explicitly, through the set() call or implicitly, via
* operator bool()), it can be reset and reused.
* 
* The class is useful in scenarios such as busy-wait or
* one-off code blocks, e.g.:
* 
*  class MyClass {
*   public:
*    void MyFunc() {
*      if (!been_here_) DoOnce();
*      DoMany();
*    }
* 
*    void DoOnce() {
*      // executed once
*    }
* 
*    void DoMany() {
*      // executed multiple times
*    }
* 
*   private:
*    AtomicFlag been_here_;
*  }
* 
*  MyClass my_class;
*  int i = 0;
*  while (++i < 10) my_class.MyFunc();
*/
class AtomicFlag {
 public:
  AtomicFlag() {}
  ~AtomicFlag() {}

  /**
   * Sets the flag to true and returns previously held value.
   */
  bool Set() {
    return flag_.test_and_set(std::memory_order_acquire);
  }

  /**
   * Resets the flag to false.
   */
  void Reset() {
    flag_.clear(std::memory_order_release);
  }

  /**
   * Sets the flag to true and returns previously held value.
   */
  explicit operator bool () {
    return Set();
  }

  AtomicFlag(const AtomicFlag&) = delete;
  AtomicFlag& operator = (const AtomicFlag&) = delete;
  AtomicFlag(AtomicFlag&&) = delete;
  AtomicFlag& operator = (AtomicFlag&&) = delete;

 private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

} // namespace fun
