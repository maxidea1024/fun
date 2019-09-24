#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/notification_strategy.h"

namespace fun {

/**
 * Default notification strategy.
 *
 * Internally, a fun::Array<> is used to store
 * delegate objects. Delegates are invoked in the
 * order in which they have been registered.
 */
template <typename ArgsType, typename DelegateType>
class DefaultStrategy : public NotificationStrategy<ArgsType, DelegateType> {
 public:
  using DelegateHandle = DelegateType*;
  using DelegatePtr = SharedPtr<DelegateType>;
  using DelegateArray = Array<DelegatePtr>;
  using Iterator = typename DelegateArray::Iterator;

  DefaultStrategy() = default;
  DefaultStrategy(const DefaultStrategy& rhs) = default;
  DefaultStrategy& operator=(const DefaultStrategy& rhs) = default;

  ~DefaultStrategy() {}

  void Notify(const void* sender, ArgsType& args) {
    for (auto& delegate : delegates_) {
      delegate->Notify(sender, args);
    }
  }

  DelegateHandle Add(const DelegateType& delegate) {
    DelegatePtr cloned_delegate(static_cast<DelegateType*>(delegate.Clone()));
    delegates_.Add(cloned_delegate);
    return cloned_delegate.Get();
  }

  void Remove(const DelegateType& delegate) {
    // TODO
    fun_check(0);
  }

  void Remove(DelegateHandle delegate_handle) {
    // TODO
    fun_check(0);
  }

  void Clear() {
    for (auto& delegate : delegates_) {
      delegate->Disable();
    }
    delegates_.Clear();
  }

  bool IsEmpty() const { return delegates_.IsEmpty(); }

 protected:
  Array<DelegatePtr> delegates_;
};

// Specialization for void argument.
template <typename DelegateType>
class DefaultStrategy<void, DelegateType>
    : public NotificationStrategy<void, DelegateType> {
 public:
  using DelegateHandle = DelegateType*;
  using DelegatePtr = SharedPtr<DelegateType>;
  using DelegateArray = Array<DelegatePtr>;
  using Iterator = typename DelegateArray::Iterator;

  DefaultStrategy() = default;
  DefaultStrategy(const DefaultStrategy& rhs) = default;
  DefaultStrategy& operator=(const DefaultStrategy& rhs) = default;

  ~DefaultStrategy() {}

  void Notify(const void* sender) {
    for (auto& delegate : delegates_) {
      delegate->Notify(sender);
    }
  }

  DelegateHandle Add(const DelegateType& delegate) {
    DelegatePtr cloned_delegate(static_cast<DelegateType>(delegate.Clone()));
    delegates_.Add(cloned_delegate);
    return cloned_delegate.Get();
  }

  void Remove(const DelegateType& delegate) {
    // TODO
    fun_check(0);
  }

  void Remove(DelegateHandle delegate_handle) {
    // TODO
    fun_check(0);
  }

  void Clear() {
    for (auto& delegate : delegates_) {
      delegate->Disable();
    }
    delegates_.Clear();
  }

  bool IsEmpty() const { return delegates_.IsEmpty(); }

 protected:
  Array<DelegatePtr> delegates_;
};

}  // namespace fun
