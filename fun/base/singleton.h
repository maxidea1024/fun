#pragma once

#include "fun/base/base.h"
#include "fun/base/atomics.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/thread.h"

namespace fun {

/**
 * Thread safe singleton.
 */
template <typename T>
class Singleton {
 public:
  typedef SharedPtr<T> Ptr;

  FUN_NO_INLINE static Ptr GetPtr() {
    // 0=생성안됨.
    // 1=생성중.
    // 2=생성됨.
    volatile static int state = 0;

    // Instance pointer.
    static Ptr instance_ptr;

    // If the instance has already been created, return it immediately.
    if (state == 2) {
      return instance_ptr;
    }

    // Must wait if it is being spawned by another thread.
    if (Atomics::CompareExchange(&state, 1, 0) == 0) {
      instance_ptr.Reset(new T);
      Atomics::CompareExchange(&state, 2, 1);
      return instance_ptr;
    }

    // Creating an instance on another thread, so we wait
    // until the creation is complete.
    while (state != 2) {
      //Thread::Sleep(2); // 2msec wait
      Thread::Yield();
    }

    fun_check_ptr(instance_ptr);

    return instance_ptr;
  }

  static FUN_ALWAYS_INLINE T& GetUnsafePtr() {
    static T* instance = nullptr;
    if (!instance) {
      instance = GetPtr().Get();
    }

    return instance;
  }

  static FUN_ALWAYS_INLINE T& GetUnsafeRef() {
    return *GetUnsafePtr();
  }


  class Holder {
   public:
    Holder() : ptr_(Singleton<T>::GetPtr()) {}

    T* Get() {
      return ptr_.Get();// .GetUnsafePtr();
    }

    operator T* () {
      return Get();
    }

   private:
    Singleton::Ptr ptr_;
  };
};

} // namespace fun
