//TODO FastMutex로 교체하는게 좋을까??

#pragma once

#include "fun/base/base.h"
#include "fun/base/priority_delegate_base.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * Wraps a freestanding function or static member function
 * for use as a PriorityDelegate.
 */
template <typename ArgsType, bool with_sender = true, bool sender_is_const = true>
class FunctionPriorityDelegate : public PriorityDelegateBase<ArgsType> {
 public:
  typedef void (*NotifyFunction)(const void*, ArgsType&);

  FunctionPriorityDelegate(NotifyFunction function, int prio)
    : PriorityDelegateBase<ArgsType>(prio), function_(function) {}

  FunctionPriorityDelegate(const FunctionPriorityDelegate& delegate)
    : PriorityDelegateBase<ArgsType>(delegate),
      function_(delegate.function_) {}

  FunctionPriorityDelegate& operator = (const FunctionPriorityDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      function_ = delegate.function_;
      priority_ = delegate.priority_;
    }
    return *this;
  }

  ~FunctionPriorityDelegate() {}

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
    const FunctionPriorityDelegate* other_delegate = dynamic_cast<const FunctionPriorityDelegate*>(other.Unwrap());
    return other_delegate && this->GetPriority() == other_delegate->GetPriority() && function_ == other_delegate->function_;
  }

  DelegateBase<ArgsType>* Clone() const {
    return new FunctionPriorityDelegate(*this);
  }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionPriorityDelegate();
};


template <typename ArgsType>
class FunctionPriorityDelegate<ArgsType, true, false> : public PriorityDelegateBase<ArgsType> {
 public:
  typedef void (*NotifyFunction)(void*, ArgsType&);

  FunctionPriorityDelegate(NotifyFunction function, int prio)
    : PriorityDelegateBase<ArgsType>(prio),
    , function_(function) {}

  FunctionPriorityDelegate(const FunctionPriorityDelegate& delegate)
    : PriorityDelegateBase<ArgsType>(delegate),
    , function_(delegate.function_) {}

  FunctionPriorityDelegate& operator = (const FunctionPriorityDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      function_ = delegate.function_;
      priority_ = delegate.priority_;
    }
    return *this;
  }

  ~FunctionPriorityDelegate() {}

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
    const FunctionPriorityDelegate* other_delegate = dynamic_cast<const FunctionPriorityDelegate*>(other.Unwrap());
    return other_delegate && this->GetPriority() == other_delegate->GetPriority() && function_ == other_delegate->function_;
  }

  DelegateBase<ArgsType>* Clone() const {
    return new FunctionPriorityDelegate(*this);
  }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionPriorityDelegate();
};


template <typename ArgsType>
class FunctionPriorityDelegate<ArgsType, false> : public PriorityDelegateBase<ArgsType> {
 public:
  typedef void (*NotifyFunction)(ArgsType&);

  FunctionPriorityDelegate(NotifyFunction function, int prio)
    : PriorityDelegateBase<ArgsType>(prio),
      function_(function) {}

  FunctionPriorityDelegate(const FunctionPriorityDelegate& delegate)
    : PriorityDelegateBase<ArgsType>(delegate),
      function_(delegate.function_) {}

  FunctionPriorityDelegate& operator = (const FunctionPriorityDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      function_ = delegate.function_;
      priority_ = delegate.priority_;
    }
    return *this;
  }

  ~FunctionPriorityDelegate() {}

  bool Notify(const void* sender, ArgsType& arguments) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)(arguments);
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    const FunctionPriorityDelegate* other_delegate = dynamic_cast<const FunctionPriorityDelegate*>(other.Unwrap());
    return other_delegate && this->GetPriority() == other_delegate->GetPriority() && function_ == other_delegate->function_;
  }

  DelegateBase<ArgsType>* Clone() const {
    return new FunctionPriorityDelegate(*this);
  }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionPriorityDelegate();
};


/**
 * Wraps a freestanding function or static member function
 * for use as a PriorityDelegate.
 */
template <>
class FunctionPriorityDelegate<void, true, true> : public PriorityDelegateBase<void> {
public:
  typedef void (*NotifyFunction)(const void*);

  FunctionPriorityDelegate(NotifyFunction function, int prio)
    : PriorityDelegateBase<void>(prio), function_(function) {}

  FunctionPriorityDelegate(const FunctionPriorityDelegate& delegate)
    : PriorityDelegateBase<void>(delegate), function_(delegate.function_) {}

  FunctionPriorityDelegate& operator = (const FunctionPriorityDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      function_ = delegate.function_;
      priority_ = delegate.priority_;
    }
    return *this;
  }

  ~FunctionPriorityDelegate() {}

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
    const FunctionPriorityDelegate* other_delegate = dynamic_cast<const FunctionPriorityDelegate*>(other.Unwrap());
    return other_delegate && this->GetPriority() == other_delegate->GetPriority() && function_ == other_delegate->function_;
  }

  DelegateBase<void>* Clone() const {
    return new FunctionPriorityDelegate(*this);
  }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionPriorityDelegate();
};


template <>
class FunctionPriorityDelegate<void, true, false> : public PriorityDelegateBase<void> {
 public:
  typedef void (*NotifyFunction)(void*);

  FunctionPriorityDelegate(NotifyFunction function, int prio)
    : PriorityDelegateBase<void>(prio), function_(function) {}

  FunctionPriorityDelegate(const FunctionPriorityDelegate& delegate)
    : PriorityDelegateBase<void>(delegate), function_(delegate.function_) {}

  FunctionPriorityDelegate& operator = (const FunctionPriorityDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      function_ = delegate.function_;
      priority_ = delegate.priority_;
    }
    return *this;
  }

  ~FunctionPriorityDelegate() {}

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
    const FunctionPriorityDelegate* other_delegate = dynamic_cast<const FunctionPriorityDelegate*>(other.Unwrap());
    return other_delegate && this->GetPriority() == other_delegate->GetPriority() && function_ == other_delegate->function_;
  }

  DelegateBase<void>* Clone() const {
    return new FunctionPriorityDelegate(*this);
  }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionPriorityDelegate();
};


template <>
class FunctionPriorityDelegate<void, false> : public PriorityDelegateBase<void> {
 public:
  typedef void (*NotifyFunction)();

  FunctionPriorityDelegate(NotifyFunction function, int prio)
    : PriorityDelegateBase<void>(prio), function_(function) {}

  FunctionPriorityDelegate(const FunctionPriorityDelegate& delegate)
    : PriorityDelegateBase<void>(delegate), function_(delegate.function_) {}

  FunctionPriorityDelegate& operator = (const FunctionPriorityDelegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      function_ = delegate.function_;
      priority_ = delegate.priority_;
    }
    return *this;
  }

  ~FunctionPriorityDelegate() {}

  bool Notify(const void* sender) {
    Mutex::ScopedLock guard(mutex_);
    if (FUN_LIKELY(function_)) {
      (*function_)();
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<void>& other) const {
    const FunctionPriorityDelegate* other_delegate = dynamic_cast<const FunctionPriorityDelegate*>(other.Unwrap());
    return other_delegate && this->GetPriority() == other_delegate->GetPriority() && function_ == other_delegate->function_;
  }

  DelegateBase<void>* Clone() const {
    return new FunctionPriorityDelegate(*this);
  }

  void Disable() {
    Mutex::ScopedLock guard(mutex_);
    function_ = 0;
  }

 protected:
  NotifyFunction function_;
  Mutex mutex_;

 private:
  FunctionPriorityDelegate();
};

} // namespace fun
