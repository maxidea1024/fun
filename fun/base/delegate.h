#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate_base.h"
#include "fun/base/expire.h"
#include "fun/base/function_delegate.h"
#include "fun/base/mutex.h"

namespace fun {

template <typename ObjectType, typename ArgsType, bool with_sender = true>
class Delegate : public DelegateBase<ArgsType> {
 public:
  typedef void (ObjectType::*NotifyMethod)(const void*, ArgsType&);

  Delegate(ObjectType* object, NotifyMethod method)
      : receiver_object_(object), receiver_method_(method) {}

  Delegate(const Delegate& rhs)
      : DelegateBase<ArgsType>(rhs),
        receiver_object_(rhs.receiver_object_),
        receiver_method_(rhs.receiver_method_) {}

  ~Delegate() {}

  Delegate& operator=(const Delegate& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      receiver_object_ = rhs.receiver_object_;
      receiver_method_ = rhs.receiver_method_;
    }
    return *this;
  }

  bool Notify(const void* sender, ArgsType& args) {
    ScopedLock guard(mutex_);
    if (FUN_LIKELY(receiver_object_)) {
      (receiver_object_->*receiver_method_)(sender, args);
      return true;
    }

    return false;
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    const Delegate* other_delegate =
        dynamic_cast<const Delegate*>(other.Unwrap());
    return other_delegate &&
           receiver_object_ == other_delegate->receiver_object_ &&
           receiver_method_ == other_delegate->receiver_method_;
  }

  DelegateBase<ArgsType>* Clone() const { return new Delegate(*this); }

  void Disable() {
    ScopedLock guard(mutex_);
    receiver_object_ = nullptr;
  }

 protected:
  ObjectType* receiver_object_;
  NotifyMethod receiver_method_;
  Mutex mutex_;

 private:
  Delegate();
};

template <typename ObjectType, typename ArgsType>
class Delegate<ObjectType, ArgsType, false> : public DelegateBase<ArgsType> {
 public:
  typedef void (ObjectType::*NotifyMethod)(ArgsType&);

  Delegate(ObjectType* object, NotifyMethod method)
      : receiver_object_(object), receiver_method_(method) {}

  Delegate(const Delegate& rhs)
      : DelegateBase<ArgsType>(rhs),
        receiver_object_(rhs.receiver_object_),
        receiver_method_(rhs.receiver_method_) {}

  ~Delegate() {}

  Delegate& operator=(const Delegate& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      receiver_object_ = rhs.receiver_object_;
      receiver_method_ = rhs.receiver_method_;
    }
    return *this;
  }

  bool Notify(const void* sender, ArgsType& args) {
    ScopedLock guard(mutex_);
    if (FUN_LIKELY(receiver_object_)) {
      (receiver_object_->*receiver_method_)(args);
      return true;
    }

    return false;
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    const Delegate* other_delegate =
        dynamic_cast<const Delegate*>(other.Unwrap());
    return other_delegate &&
           receiver_object_ == other_delegate->receiver_object_ &&
           receiver_method_ == other_delegate->receiver_method_;
  }

  DelegateBase<ArgsType>* Clone() const { return new Delegate(*this); }

  void Disable() {
    ScopedLock guard(mutex_);
    receiver_object_ = nullptr;
  }

 protected:
  ObjectType* receiver_object_;
  NotifyMethod receiver_method_;
  Mutex mutex_;

 private:
  Delegate();
};

template <typename ObjectType, typename ArgsType>
FUN_ALWAYS_INLINE Delegate<ObjectType, ArgsType, true> MakeDelegate(
    ObjectType* object, void (ObjectType::*method)(const void*, ArgsType)) {
  return Delegate<ObjectType, ArgsType, true>(object, method);
}

template <typename ObjectType, typename ArgsType>
FUN_ALWAYS_INLINE Delegate<ObjectType, ArgsType, false> MakeDelegate(
    ObjectType* object, void (ObjectType::*method)(ArgsType)) {
  return Delegate<ObjectType, ArgsType, false>(object, method);
}

template <typename ObjectType, typename ArgsType>
FUN_ALWAYS_INLINE Expire<ArgsType> MakeDelegate(
    ObjectType* object, void (ObjectType::*method)(const void*, ArgsType),
    Timestamp::TimeDiff expire) {
  return Expire<ArgsType>(Delegate<ObjectType, ArgsType, true>(object, method),
                          expire);
}

template <typename ObjectType, typename ArgsType>
FUN_ALWAYS_INLINE Expire<ArgsType> MakeDelegate(
    ObjectType* object, void (ObjectType::*method)(ArgsType),
    Timestamp::TimeDiff expire) {
  return Expire<ArgsType>(Delegate<ObjectType, ArgsType, false>(object, method),
                          expire);
}

template <typename ArgsType>
FUN_ALWAYS_INLINE Expire<ArgsType> MakeDelegate(void (*func)(const void*,
                                                             ArgsType),
                                                Timestamp::TimeDiff expire) {
  return Expire<ArgsType>(FunctionDelegate<true, true>(func), expire);
}

template <typename ArgsType>
FUN_ALWAYS_INLINE Expire<ArgsType> MakeDelegate(void (*func)(ArgsType),
                                                Timestamp::TimeDiff expire) {
  return Expire<ArgsType>(FunctionDelegate<true, false>(func), expire);
}

template <typename ArgsType>
FUN_ALWAYS_INLINE Expire<ArgsType> MakeDelegate(void (*func)(ArgsType&),
                                                Timestamp::TimeDiff expire) {
  return Expire<ArgsType>(FunctionDelegate<ArgsType, false>(func), expire);
}

template <typename ArgsType>
FUN_ALWAYS_INLINE FunctionDelegate<ArgsType, true, true> MakeDelegate(
    void (*func)(const void*, ArgsType&)) {
  return FunctionDelegate<ArgsType, true, true>(func);
}

template <typename ArgsType>
FUN_ALWAYS_INLINE FunctionDelegate<ArgsType, true, false> MakeDelegate(
    void (*func)(void*, ArgsType&)) {
  return FunctionDelegate<ArgsType, true, false>(func);
}

template <typename ArgsType>
FUN_ALWAYS_INLINE FunctionDelegate<ArgsType, false> MakeDelegate(
    void (*func)(ArgsType&)) {
  return FunctionDelegate<ArgsType, false>(func);
}

template <typename ObjectType>
class Delegate<ObjectType, void, true> : public DelegateBase<void> {
 public:
  typedef void (ObjectType::*NotifyMethod)(const void*);

  Delegate(ObjectType* object, NotifyMethod method)
      : receiver_object_(object), receiver_method_(method) {}

  Delegate(const Delegate& delegate)
      : DelegateBase<void>(delegate),
        receiver_object_(delegate.receiver_object_),
        receiver_method_(delegate.receiver_method_) {}

  ~Delegate() {}

  Delegate& operator=(const Delegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      this->receiver_object_ = delegate.receiver_object_;
      this->receiver_method_ = delegate.receiver_method_;
    }
    return *this;
  }

  bool Notify(const void* sender) {
    ScopedLock guard(mutex_);
    if (FUN_LIKELY(receiver_object_)) {
      (receiver_object_->*receiver_method_)(sender);
      return true;
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<void>& other) const {
    const Delegate* other_delegate =
        dynamic_cast<const Delegate*>(other.Unwrap());
    return other_delegate &&
           receiver_object_ == other_delegate->receiver_object_ &&
           receiver_method_ == other_delegate->receiver_method_;
  }

  DelegateBase<void>* Clone() const { return new Delegate(*this); }

  void Disable() {
    ScopedLock guard(mutex_);
    receiver_object_ = 0;
  }

 protected:
  ObjectType* receiver_object_;
  NotifyMethod receiver_method_;
  Mutex mutex_;

 private:
  Delegate();
};

template <typename ObjectType>
class Delegate<ObjectType, void, false> : public DelegateBase<void> {
 public:
  typedef void (ObjectType::*NotifyMethod)();

  Delegate(ObjectType* object, NotifyMethod method)
      : receiver_object_(object), receiver_method_(method) {}

  Delegate(const Delegate& delegate)
      : DelegateBase<void>(delegate),
        receiver_object_(delegate.receiver_object_),
        receiver_method_(delegate.receiver_method_) {}

  ~Delegate() {}

  Delegate& operator=(const Delegate& delegate) {
    if (FUN_LIKELY(&delegate != this)) {
      receiver_object_ = delegate.receiver_object_;
      receiver_method_ = delegate.receiver_method_;
    }
    return *this;
  }

  bool Notify(const void*) {
    ScopedLock guard(mutex_);
    if (FUN_LIKELY(receiver_object_)) {
      (receiver_object_->*receiver_method_)();
      return true;
    }

    return false;
  }

  bool Equals(const DelegateBase<void>& other) const {
    const Delegate* other_delegate =
        dynamic_cast<const Delegate*>(other.Unwrap());
    return other_delegate &&
           receiver_object_ == other_delegate->receiver_object_ &&
           receiver_method_ == other_delegate->receiver_method_;
  }

  DelegateBase<void>* Clone() const { return new Delegate(*this); }

  void Disable() {
    ScopedLock guard(mutex_);
    receiver_object_ = 0;
  }

 protected:
  ObjectType* receiver_object_;
  NotifyMethod receiver_method_;
  Mutex mutex_;

 private:
  Delegate();
};

template <typename ObjectType>
FUN_ALWAYS_INLINE Delegate<ObjectType, void, true> MakeDelegate(
    ObjectType* object, void (ObjectType::*method)(const void*)) {
  return Delegate<ObjectType, void, true>(object, method);
}

template <typename ObjectType>
FUN_ALWAYS_INLINE Delegate<ObjectType, void, false> MakeDelegate(
    ObjectType* object, void (ObjectType::*method)()) {
  return Delegate<ObjectType, void, false>(object, method);
}

template <typename ObjectType>
FUN_ALWAYS_INLINE Expire<void> MakeDelegate(
    ObjectType* object, void (ObjectType::*method)(const void*),
    Timestamp::TimeDiff expire) {
  return Expire<void>(Delegate<ObjectType, void, true>(object, method), expire);
}

template <typename ObjectType>
FUN_ALWAYS_INLINE Expire<void> MakeDelegate(ObjectType* object,
                                            void (ObjectType::*method)(),
                                            Timestamp::TimeDiff expire) {
  return Expire<void>(Delegate<ObjectType, void, false>(object, method),
                      expire);
}

FUN_ALWAYS_INLINE Expire<void> MakeDelegate(void (*func)(const void*),
                                            Timestamp::TimeDiff expire) {
  return Expire<void>(FunctionDelegate<void, true, true>(func), expire);
}

FUN_ALWAYS_INLINE Expire<void> MakeDelegate(void (*func)(void*),
                                            Timestamp::TimeDiff expire) {
  return Expire<void>(FunctionDelegate<void, true, false>(func), expire);
}

FUN_ALWAYS_INLINE Expire<void> MakeDelegate(void (*func)(),
                                            Timestamp::TimeDiff expire) {
  return Expire<void>(FunctionDelegate<void, false>(func), expire);
}

FUN_ALWAYS_INLINE FunctionDelegate<void, true, true> MakeDelegate(
    void (*func)(const void*)) {
  return FunctionDelegate<void, true, true>(func);
}

FUN_ALWAYS_INLINE FunctionDelegate<void, true, false> MakeDelegate(
    void (*func)(void*)) {
  return FunctionDelegate<void, true, false>(func);
}

FUN_ALWAYS_INLINE FunctionDelegate<void, false> MakeDelegate(void (*func)()) {
  return FunctionDelegate<void, false>(func);
}

}  // namespace fun
