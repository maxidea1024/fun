#pragma once

#include "fun/base/base.h"
#include "fun/base/container/map.h"

namespace fun {

/**
 * This is the base class for all objects
 * that the ThreadLocalStorage class manages.
 */
class FUN_BASE_API TlsSlotBase {
 public:
  TlsSlotBase();
  virtual ~TlsSlotBase();
};

/**
 * The Slot template wraps another class
 * so that it can be stored in a ThreadLocalStorage
 * object. This class is used internally, and you
 * must not create instances of it yourself.
 */
template <typename T>
class TlsSlot : public TlsSlotBase {
 public:
  TlsSlot() : value_() {}
  ~TlsSlot() {}

  T& Value() { return value_; }

  TlsSlot(const TlsSlot&) = delete;
  TlsSlot& operator=(const TlsSlot&) = delete;

  T value_;
};

/**
 * This class manages the local storage for each thread.
 * Never use this class directly, always use the
 * ThreadLocal template for managing thread local storage.
 */
class FUN_BASE_API ThreadLocalStorage {
 public:
  /**
   * Creates the TLS.
   */
  ThreadLocalStorage();

  /**
   * Deletes the TLS.
   */
  ~ThreadLocalStorage();

  /**
   * Returns the slot for the given key.
   */
  TlsSlotBase*& Get(const void* key);

  /**
   * Returns the TLS object for the current thread
   * (which may also be the main thread).
   */
  static ThreadLocalStorage& Current();

  /**
   * Clears the current thread's TLS object.
   * Does nothing in the main thread.
   */
  static void Clear();

 private:
  Map<const void*, TlsSlotBase*> map_;

  friend class Thread;
};

/**
 * This template is used to declare type safe thread
 * local variables. It can basically be used like
 * a smart pointer class with the special feature
 * that it references a different object
 * in every thread. The underlying object will
 * be created when it is referenced for the first
 * time.
 * See the NestedDiagnosticContext class for an
 * example how to use this template.
 * Every thread only has access to its own
 * thread local data. There is no way for a thread
 * to access another thread's local data.
 */
template <typename T>
class ThreadLocal {
 public:
  typedef TlsSlot<T> Slot;

  ThreadLocal() {}
  ~ThreadLocal() {}

  T* operator->() { return &Get(); }

  /**
   * "Dereferences" the smart pointer and returns a reference
   * to the underlying data object. The reference can be used
   * to modify the object.
   */
  T& operator*() { return Get(); }

  /**
   * Returns a reference to the underlying data object.
   * The reference can be used to modify the object.
   */
  T& Get() {
    TlsSlotBase*& p = ThreadLocalStorage::Current().Get(this);
    if (p == nullptr) {
      p = new Slot;
    }
    return static_cast<Slot*>(p)->Value();
  }

  ThreadLocal(const ThreadLocal&) = delete;
  ThreadLocal& operator=(const ThreadLocal&) = delete;
};

}  // namespace fun
