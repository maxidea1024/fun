#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"
#include "fun/base/condition.h"
#include "fun/base/ref_counted.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/container/array.h"

namespace fun {

/**
 * A PoolableObjectFactory is responsible for creating and resetting
 * objects managed by an ObjectPool.
 *
 * Together with an ObjectPool, a PoolableObjectFactory is used as
 * a policy class to change the behavior of the ObjectPool when
 * creating new objects, returning used objects back to the pool
 * and destroying objects, when the pool itself is destroyed or
 * shrunk.
 */
template <typename C, typename P = C*>
class PoolableObjectFactory {
 public:
  P CreateObject() { return new C; }
  bool ValidateObject(P object) { return true; }
  void ActivateObject(P object) {}
  void DeactivateObject(P object) {}
  void DestroyObject(P object) { delete object; }
};

template <typename C>
class PoolableObjectFactory<C, RefCountedPtr<C>> {
 public:
  RefCountedPtr<C> CreateObject() { return new C; }
  bool ValidateObject(RefCountedPtr<C> object) { return true; }
  void ActivateObject(RefCountedPtr<C> object) {}
  void DeactivateObject(RefCountedPtr<C> object) {}
  void DestroyObject(RefCountedPtr<C> object) {}
};

template <typename C>
class PoolableObjectFactory<C, SharedPtr<C>> {
 public:
  SharedPtr<C> CreateObject() { return new C; }
  bool ValidateObject(SharedPtr<C> object) { return true; }
  void ActivateObject(SharedPtr<C> object) {}
  void DeactivateObject(SharedPtr<C> object) {}
  void DestroyObject(SharedPtr<C> object) {}
};

/**
 * An ObjectPool manages a pool of objects of a certain class.
 *
 * The number of objects managed by the pool can be restricted.
 *
 * When an object is requested from the pool:
 *   - If an object is available from the pool, an object from the pool is
 *     removed from the pool, activated (using the factory) and returned.
 *   - Otherwise, if the peak capacity of the pool has not yet been reached,
 *     a new object is created and activated, using the object factory, and returned.
 *   - If the peak capacity has already been reached, null is returned after timeout.
 *
 * When an object is returned to the pool:
 *   - If the object is valid (checked by calling ValidateObject()
 *     from the object factory), the object is deactivated. If the
 *     number of objects in the pool is below the capacity,
 *     the object is added to the pool. Otherwise it is destroyed.
 *   - If the object is not valid, it is destroyed immediately.
 */
template <typename C, typename P = C*, typename F = PoolableObjectFactory<C, P> >
class ObjectPool {
 public:
  ObjectPool(size_t capacity, size_t peak_capacity)
    : capacity_(capacity),
      peak_capacity_(peak_capacity),
      used_count_(0) {
    fun_check(capacity <= peak_capacity);
  }

  ObjectPool(const F& factory, size_t capacity, size_t peak_capacity)
    : factory_(factory),
      capacity_(capacity),
      peak_capacity_(peak_capacity),
      used_count_(0) {
    fun_check(capacity <= peak_capacity);
  }

  ~ObjectPool() {
    try {
      for (auto& item : pool_) {
        factory_.DestroyObject(item);
      }
    } catch (...) {
      fun_unexpected();
    }
  }

  // Disable default constructor and copy.
  ObjectPool() = delete;
  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator = (const ObjectPool&) = delete;

  P BorrowObject(long timeout_msecs = 0) {
    ScopedLock<FastMutex> guard(mutex_);

    if (!pool_.IsEmpty()) {
      // 가장 최근에 사용하던것을 꺼내옴.
      // cache locality가 우수할것이므로...
      P object = pool_.Last();
      pool_.PopBack();
      return ActivateObject(object);
    }

    if (used_count_ >= peak_capacity_) {
      if (timeout_msecs == 0) {
        return nullptr;
      }

      while (used_count_ >= peak_capacity_) {
        if (!available_cond_.TryWait(mutex_, timeout_msecs)) {
          // timeout
          return nullptr;
        }
      }
    }

    // used_count_ < peak_capacity_
    P object = factory_.CreateObject();
    ActivateObject(object);
    used_count_++;
    return object;
  }

  void ReturnObject(P object) {
    ScopedLock<FastMutex> guard(mutex_);

    if (factory_.ValidateObject(object)) {
      factory_.DeactivateObject(object);

      if (pool_.Count() < capacity_) {
        try {
          pool_.Add(object);
          return;
        } catch (...) {
        }
      }
    }
    factory_.DestroyObject(object);
    used_count_--;
    available_cond_.Signal();
  }

  size_t Capacity() const {
    return capacity_;
  }

  size_t PeakCapacity() const {
    return peak_capacity_;
  }

  size_t UsedCount() const {
    ScopedLock<FastMutex> guard(mutex_);

    return used_count_;
  }

  size_t AvailableCount() const {
    ScopedLock<FastMutex> guard(mutex_);

    return pool_.Count() + peak_capacity_ - used_count_;
  }

 protected:
  P ActivateObject(P object) {
    try {
      factory_.ActivateObject(object);
    } catch (...) {
      factory_.DestroyObject(object);
      throw;
    }
    return object;
  }

 private:
  F factory_;
  size_t capacity_;
  size_t peak_capacity_;
  size_t used_count_;
  Array<P> pool_;
  mutable FastMutex mutex_;
  Condition available_cond_;
};

} // namespace fun
