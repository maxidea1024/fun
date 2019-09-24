#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/function.h"
#include "fun/base/ftl/weak_ptr.h"

namespace fun {

template <typename C, typename... Args>
class WeakCallback {
 public:
  WeakCallback(const WeakPtr<C>& object,
               const TFunction<void(C*, Args...)>& func)
      : object_(object), function_(func) {
    fun_check_ptr(object);
    fun_check_ptr(function_);
  }

  void operator()(Args&&... args) const {
    SharedPtr<C> ptr(object_.Lock());
    if (ptr) {
      function_(ptr.Get(), Forward<Args>(args)...);
    } else {
      // FUN_LOG << "expired";
    }
  }

 private:
  WeakPtr<C> object_;
  TFunction<void(C*, Args...)> function_;
};

//
// inlines
//

template <typename C, typename... Args>
WeakCallback<C, Args...> MakeWeakCallback(const SharedPtr<C>& object,
                                          void (C::*func)(Args...)) {
  return WeakCallback<C, Args...>(object, func);
}

template <typename C, typename... Args>
WeakCallback<C, Args...> MakeWeakCallback(const SharedPtr<C>& object,
                                          void (C::*func)(Args...) const) {
  return WeakCallback<C, Args...>(object, func);
}

}  // namespace fun
