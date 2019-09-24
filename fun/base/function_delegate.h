#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate_base.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * Wraps a freestanding function or static member function
 * for use as a Delegate.
 */
template <typename ArgsType, bool with_sender = true,
          bool sender_is_const = true>
class FunctionDelegate : public DelegateBase<ArgsType> {
 public:
  typedef void (*NotifyFunction)(const void*, ArgsType&);

  FunctionDelegate(NotifyFunction function) : function_(function) {}

  FunctionDelegate(const FunctionDelegate& delegate)
      : DelegateBase<ArgsType>(delegate), function_(delegate.function_) {}

  ~FunctionDelegate() {}

  FunctionDelegate& operator=(const FunctionDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      this->function_ = delegate.function_;
    }
    return *this;
  }

  bool Notify(const void* sender, ArgsType& arguments) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)(sender, arguments);
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    const FunctionDelegate* other_delegate =
        dynamic_cast<const FunctionDelegate*>(other.Unwrap());
    return other_delegate && function_ == other_delegate->function_;
  }

  DelegateBase<ArgsType>* Clone() const { return new FunctionDelegate(*this); }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionDelegate() = delete;
};

template <typename ArgsType>
class FunctionDelegate<ArgsType, true, false> : public DelegateBase<ArgsType> {
 public:
  typedef void (*NotifyFunction)(void*, ArgsType&);

  FunctionDelegate(NotifyFunction function) : function_(function) {}

  FunctionDelegate(const FunctionDelegate& delegate)
      : DelegateBase<ArgsType>(delegate), function_(delegate.function_) {}

  ~FunctionDelegate() {}

  FunctionDelegate& operator=(const FunctionDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      this->function_ = delegate.function_;
    }
    return *this;
  }

  bool Notify(const void* sender, ArgsType& arguments) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)(const_cast<void*>(sender), arguments);
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    const FunctionDelegate* other_delegate =
        dynamic_cast<const FunctionDelegate*>(other.Unwrap());
    return other_delegate && function_ == other_delegate->function_;
  }

  DelegateBase<ArgsType>* Clone() const { return new FunctionDelegate(*this); }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionDelegate() = delete;
};

template <typename ArgsType, bool sender_is_const>
class FunctionDelegate<ArgsType, false, sender_is_const>
    : public DelegateBase<ArgsType> {
 public:
  typedef void (*NotifyFunction)(ArgsType&);

  FunctionDelegate(NotifyFunction function) : function_(function) {}

  FunctionDelegate(const FunctionDelegate& delegate)
      : DelegateBase<ArgsType>(delegate), function_(delegate.function_) {}

  ~FunctionDelegate() {}

  FunctionDelegate& operator=(const FunctionDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      this->function_ = delegate.function_;
    }
    return *this;
  }

  bool Notify(const void* /*sender*/, ArgsType& arguments) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)(arguments);
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    const FunctionDelegate* other_delegate =
        dynamic_cast<const FunctionDelegate*>(other.Unwrap());
    return other_delegate && function_ == other_delegate->function_;
  }

  DelegateBase<ArgsType>* Clone() const { return new FunctionDelegate(*this); }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionDelegate() = delete;
};

/**
 * Wraps a freestanding function or static member function
 * for use as a Delegate.
 */
template <>
class FunctionDelegate<void, true, true> : public DelegateBase<void> {
 public:
  typedef void (*NotifyFunction)(const void*);

  FunctionDelegate(NotifyFunction function) : function_(function) {}

  FunctionDelegate(const FunctionDelegate& delegate)
      : DelegateBase<void>(delegate), function_(delegate.function_) {}

  ~FunctionDelegate() {}

  FunctionDelegate& operator=(const FunctionDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      this->function_ = delegate.function_;
    }
    return *this;
  }

  bool Notify(const void* sender) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)(sender);
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<void>& other) const {
    const FunctionDelegate* other_delegate =
        dynamic_cast<const FunctionDelegate*>(other.Unwrap());
    return other_delegate && function_ == other_delegate->function_;
  }

  DelegateBase<void>* Clone() const { return new FunctionDelegate(*this); }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionDelegate() = delete;
};

template <>
class FunctionDelegate<void, true, false> : public DelegateBase<void> {
 public:
  typedef void (*NotifyFunction)(void*);

  FunctionDelegate(NotifyFunction function) : function_(function) {}

  FunctionDelegate(const FunctionDelegate& delegate)
      : DelegateBase<void>(delegate), function_(delegate.function_) {}

  ~FunctionDelegate() {}

  FunctionDelegate& operator=(const FunctionDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      this->function_ = delegate.function_;
    }
    return *this;
  }

  bool Notify(const void* sender) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)(const_cast<void*>(sender));
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<void>& other) const {
    const FunctionDelegate* other_delegate =
        dynamic_cast<const FunctionDelegate*>(other.Unwrap());
    return other_delegate && function_ == other_delegate->function_;
  }

  DelegateBase<void>* Clone() const { return new FunctionDelegate(*this); }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionDelegate() = delete;
};

template <bool sender_is_const>
class FunctionDelegate<void, false, sender_is_const>
    : public DelegateBase<void> {
 public:
  typedef void (*NotifyFunction)();

  FunctionDelegate(NotifyFunction function) : function_(function) {}

  FunctionDelegate(const FunctionDelegate& delegate)
      : DelegateBase<void>(delegate), function_(delegate.function_) {}

  ~FunctionDelegate() {}

  FunctionDelegate& operator=(const FunctionDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      this->function_ = delegate.function_;
    }
    return *this;
  }

  bool Notify(const void* /*sender*/) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)();
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<void>& other) const {
    const FunctionDelegate* other_delegate =
        dynamic_cast<const FunctionDelegate*>(other.Unwrap());
    return other_delegate && function_ == other_delegate->function_;
  }

  DelegateBase<void>* Clone() const { return new FunctionDelegate(*this); }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionDelegate() = delete;
};

}  // namespace fun
