#pragma once

#include "fun/base/base.h"
#include "fun/base/async_result.h"
#include "fun/base/async_method.h"
#include "fun/base/mutex.h"
#include "fun/base/ftl/shared_ptr.h"

namespace fun {

template <
    typename ArgsType,
    typename StrategyType,
    typename DelegateType,
    typename MutexType = FastMutex
  >
class EventBase {
 public:
  typedef DelegateType* DelegateHandle;
  typedef ArgsType Args;

  EventBase()
    : execute_async_(this, &EventBase::ExecuteAsyncImpl), enabled_(true) {}

  EventBase(const StrategyType& strategy)
    : execute_async_(this, &EventBase::ExecuteAsyncImpl),
      strategy_(strategy),
      enabled_(true) {}

  virtual ~EventBase() {}

  void operator += (const DelegateType& delegate) {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Add(delegate);
  }

  void operator -= (const DelegateType& delegate) {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Remove(delegate);
  }

  DelegateHandle Add(const DelegateType& delegate) {
    ScopedLock<MutexType> guard(mutex_);
    return strategy_.Add(delegate);
  }

  void Remove(DelegateHandle delegate_handle) {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Remove(delegate_handle);
  }

  bool HasDelegates() const {
    return !strategy_.IsEmpty();
  }

  void operator () (const void* sender, ArgsType& args) {
    Notify(sender, args);
  }

  void operator () (ArgsType& args) {
    Notify(nullptr, args);
  }

  void Notify(const void* sender, ArgsType& args) {
    ScopedLockWithUnlock<MutexType> guard(mutex_);
    if (enabled_) {
      StrategyType strategy(strategy_);
      guard.Unlock();
      strategy.Notify(sender, args);
    }
  }

  AsyncResult<ArgsType> NotifyAsync(const void* sender, const ArgsType& args) {
    NotifyAsyncParams params(sender, args);
    {
      ScopedLock<MutexType> guard(mutex_);
      params.strategy = SharedPtr<StrategyType>(new StrategyType(strategy_));
      params.enabled = enabled_;
    }
    AsyncResult<ArgsType> result = execute_async_(params);
    return result;
  }

  void Enable() {
    ScopedLock<MutexType> guard(mutex_);
    enabled_ = true;
  }

  void Disable() {
    ScopedLock<MutexType> guard(mutex_);
    enabled_ = false;
  }

  bool IsEnabled() const {
    ScopedLock<MutexType> guard(mutex_);
    return enabled_;
  }

  void Clear() {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Clear();
  }

 protected:
  struct NotifyAsyncParams {
    SharedPtr<StrategyType> strategy;
    const void* sender;
    ArgsType args;
    bool enabled;

    NotifyAsyncParams(const void* sender, const ArgsType& args)
      : sender(sender), args(args), enabled(true) {}
  };

  AsyncMethod<ArgsType, NotifyAsyncParams, EventBase> execute_async_;

  ArgsType ExecuteAsyncImpl(const NotifyAsyncParams& params) {
    if (!params.enabled) {
      return params.args;
    }

    NotifyAsyncParams p2 = params;
    ArgsType ret_args(p2.args);
    p2.strategy->Notify(p2.sender, ret_args);
    return ret_args;
  }

  StrategyType strategy_;
  bool enabled_;
  MutexType mutex_;

 private:
  EventBase(const EventBase&) = delete;
  EventBase& operator = (const EventBase&) = delete;
};


template <
    typename StrategyType,
    typename DelegateType,
    typename MutexType
  >
class EventBase<void, StrategyType, DelegateType, MutexType> {
  typedef DelegateType* DelegateHandle;

  EventBase()
    : execute_async_(this, &EventBase::ExecuteAsyncImpl),
      enabled_(true) {}

  EventBase(const StrategyType& strategy)
    : execute_async_(this, &EventBase::ExecuteAsyncImpl),
      strategy_(strategy),
      enabled_(true) {}

  virtual ~EventBase() {}

  void operator += (const DelegateType& delegate) {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Add(delegate);
  }

  void operator -= (const DelegateType& delegate) {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Remove(delegate);
  }

  DelegateHandle Add(const DelegateType& delegate) {
    ScopedLock<MutexType> guard(mutex_);
    return strategy_.Add(delegate);
  }

  void Remove(DelegateHandle delegate_handle) {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Remove(delegate_handle);
  }

  bool HasDelegates() const {
    return !strategy_.IsEmpty();
  }

  void operator () (const void* sender) {
    Notify(sender);
  }

  void operator () () {
    Notify(nullptr);
  }

  void Notify(const void* sender) {
    ScopedLockWithUnlock<MutexType> guard(mutex_);
    if (enabled_) {
      StrategyType strategy(strategy_);
      guard.Unlock();
      strategy.Notify(sender);
    }
  }

  AsyncResult<void> NotifyAsync(const void* sender) {
    NotifyAsyncParams params(sender);
    {
      ScopedLock<MutexType> guard(mutex_);
      params.strategy = SharedPtr<StrategyType>(new StrategyType(strategy_));
      params.enabled = enabled_;
    }
    AsyncResult<void> result = execute_async_(params);
    return result;
  }

  void Enable() {
    ScopedLock<MutexType> guard(mutex_);
    enabled_ = true;
  }

  void Disable() {
    ScopedLock<MutexType> guard(mutex_);
    enabled_ = false;
  }

  bool IsEnabled() const {
    ScopedLock<MutexType> guard(mutex_);
    return enabled_;
  }

  void Clear() {
    ScopedLock<MutexType> guard(mutex_);
    strategy_.Clear();
  }

 protected:
  struct NotifyAsyncParams {
    SharedPtr<StrategyType> strategy;
    const void* sender;
    bool enabled;

    NotifyAsyncParams(const void* sender)
      : sender(sender), enabled(true) {}
  };

  AsyncMethod<void, NotifyAsyncParams, EventBase> execute_async_;

  void ExecuteAsyncImpl(const NotifyAsyncParams& params) {
    if (!params.enabled) {
      return params.args;
    }

    NotifyAsyncParams p2 = params;
    p2.strategy->Notify(p2.sender);
    return;
  }

  StrategyType strategy_;
  bool enabled_;
  MutexType mutex_;

 private:
  EventBase(const EventBase&) = delete;
  EventBase& operator = (const EventBase&) = delete;
};

} // namespace fun
