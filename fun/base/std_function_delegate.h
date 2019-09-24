#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate_base.h"
#include "fun/base/mutex.h"
#include "fun/base/ftl/function.h"

#include <atomic>

namespace fun {

/**
 * Wraps a std::function or lambda for use as a Delegate.
 */
template <typename ArgsType, bool with_sender = true, bool sender_is_const = true>
class StdFunctionDelegate : public DelegateBase<ArgsType> {
 public:
  typedef TFunction<void (const void*, ArgsType&)> NotifyMethod;

  StdFunctionDelegate() = delete;

  StdFunctionDelegate(NotifyMethod method)
    : DelegateBase<ArgsType>(),
      receiver_method_(method),
      id_(++id_generator) {
  }

  StdFunctionDelegate(const StdFunctionDelegate& rhs)
    : DelegateBase<ArgsType>(rhs),
      receiver_method_(rhs.receiver_method_),
      id_(rhs.id_) {
  }

  ~StdFunctionDelegate() {
  }

  StdFunctionDelegate& operator = (const StdFunctionDelegate& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      target_ = rhs.target_;
      receiver_method_ = rhs.receiver_method_;
      id_ = rhs.id_;
    }
    return *this;
  }

  bool Notify(const void* sender, ArgsType& args) {
    ScopedLock guard(mutex_);
    if (FUN_LIKELY(receiver_method_)) {
      receiver_method_(sender, args);
      return true;
    }

    return false;
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    const StdFunctionDelegate* other_delegate = dynamic_cast<const StdFunctionDelegate*>(other.Unwrap());
    return other_delegate && id_ == other_delegate->id_;
  }

  DelegateBase<ArgsType>* Clone() const {
    return StdFunctionDelegate(*this);
  }

  void Disable() {
    ScopedLock guard(mutex_);
    receiver_method_ = nullptr;
  }

 protected:
  NotifyMethod receiver_method_;
  Mutex mutex_;
  int32 id_;

 private:
  static std::atomic_int id_generator_;
};


template <typename ArgsType, bool with_sender, bool sender_is_const>
std::atomic_int StdFunctionDelegate<ArgsType, with_sender, sender_is_const>::id_generator_;

} // namespace fun
