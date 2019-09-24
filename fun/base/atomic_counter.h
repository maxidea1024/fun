#pragma once

#include "fun/base/base.h"

#include <atomic>

namespace fun {

/**
 * This class implements a simple counter, which
 * provides atomic operations that are safe to
 * use in a multithreaded environment.
 *
 * Typical usage of AtomicCounter is for implementing
 * reference counting and similar functionality.
 */
template <typename CounterValueType>
class FUN_BASE_API AtomicCounter {
 public:
  /**
   * The underlying integer type.
   */
  using ValueType = CounterValueType;

  AtomicCounter() : counter_(0) {}

  explicit AtomicCounter(ValueType initial_value) : counter_(initial_value) {}

  AtomicCounter(const AtomicCounter& rhs) : counter_(rhs.counter_) {}

  ~AtomicCounter() {}

  AtomicCounter& operator=(const AtomicCounter& rhs) {
    counter_.store(rhs.counter_.load());
    return *this;
  }

  void Set(ValueType value) { counter_.store(value); }

  AtomicCounter& operator=(ValueType value) { counter_.store(value); }

  operator ValueType() const { return counter_.load(); }

  ValueType Value() const { return counter_.load(); }

  ValueType operator++() { return ++counter_; }

  ValueType operator++(int) { return counter_++; }

  ValueType operator--() { return --counter_; }

  ValueType operator--(int) { return counter_--; }

  bool operator!() const { return counter_.load() == 0; }

 private:
  std::atomic<ValueType> counter_;
};

using AtomicCounter32 = AtomicCounter<int32>;
using AtomicCounter64 = AtomicCounter<int64>;

}  // namespace fun
