#pragma once

#include "fun/base/base.h"
#include "fun/base/atomic_counter.h"

//TODO thread-safe issue checking.
//atomic...

/*

mutable std::atomic<int> counter_;

//TODO 카운터를 0으로 1로??
RefCounter::RefCounter() : counter_(0) {}

int RefCounter::operator ++ () {
  return counter_.fetch_add(1, std::memory_order_relaxed) + 1;
}
int RefCounter::operator ++ (int) {
  return counter_.fetch_add(1, std::memory_order_relaxed);
}

int RefCounter::operator -- () {
  return counter_.fetch_sub(1, std::memory_order_acquire) - 1;
}

int RefCounter::operator -- (int) {
  return counter_.fetch_sub(1, std::memory_order_acquire);
}

int RefCounter::operator int() {
  return counter_;
}

*/

namespace fun {

/**
 * A virtual interface for ref counted objects to implement.
 */
class IRefCountedObject {
 public:
  virtual ~IRefCountedObject() {}

  virtual uint32 AddRef() const = 0;
  virtual uint32 Release() const = 0;
  virtual uint32 GetReferencedCount() const = 0;
};

//TODO thread safety issue

/**
 * The base class of reference counted objects.
 */
class RefCountedObject : public IRefCountedObject {
 public:
  RefCountedObject() : referenced_count_(0) {}

  virtual ~RefCountedObject() {
    fun_check(referenced_count_ == 0);
  }

  uint32 AddRef() const {
    return uint32(++referenced_count_);
  }

  uint32 Release() const {
    uint32 refs = uint32(--referenced_count_);
    if (refs == 0) {
      delete this;
    }
    return refs;
  }

  uint32 GetReferencedCount() const {
    return uint32(referenced_count_);
  }

 private:
  mutable AtomicCounter32 referenced_count_;
};

} // namespace fun
