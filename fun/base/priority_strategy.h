#pragma once

#include "fun/base/base.h"
#include "fun/base/notification_strategy.h"
#include "fun/base/shared_ptr.h"
#include <vector>

namespace fun {

/**
 * NotificationStrategy for PriorityEvent.
 *
 * Delegates are kept in a std::vector<>, ordered
 * by their priority.
 */
template <typename ArgsType, typename DelegateType>
class PriorityStrategy : public NotificationStrategy<ArgsType, DelegateType> {
 public:
  typedef DelegateType* DelegateHandle;
  typedef SharedPtr<DelegateType> DelegatePtr;
  typedef std::vector<DelegatePtr> Delegates;
  typedef typename Delegates::iterator Iterator;

 public:
  PriorityStrategy() {}

  PriorityStrategy(const PriorityStrategy& s)
    : delegates_(s.delegates_) {}

  ~PriorityStrategy() {}

  void Notify(const void* sender, ArgsType& arguments) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      (*it)->Notify(sender, arguments);
    }
  }

  DelegateHandle Add(const DelegateType& delegate) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      if ((*it)->GetPriority() > delegate.GetPriority()) {
        DelegatePtr delegate_ptr(static_cast<DelegateType*>(delegate.Clone()));
        delegates_.insert(it, delegate_ptr);
        return delegate_ptr.Get();
      }
    }
    DelegatePtr delegate_ptr(static_cast<DelegateType*>(delegate.Clone()));
    delegates_.push_back(delegate_ptr);
    return delegate_ptr.Get();
  }

  void Remove(const DelegateType& delegate) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      if (delegate.Equals(**it)) {
        (*it)->Disable();
        delegates_.erase(it);
        return;
      }
    }
  }

  void Remove(DelegateHandle delegate_handle) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      if (*it == delegate_handle) {
        (*it)->Disable();
        delegates_.erase(it);
        return;
      }
    }
  }

  PriorityStrategy& operator = (const PriorityStrategy& s) {
    if (FUN_LIKELY(&s != this)) {
      delegates_ = s.delegates_;
    }
    return *this;
  }

  void Clear() {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      (*it)->Disable();
    }
    delegates_.clear();
  }

  bool IsEmpty() const {
    return delegates_.empty();
  }

 protected:
  Delegates delegates_;
};


/**
 * NotificationStrategy for PriorityEvent.
 *
 * Delegates are kept in a std::vector<>, ordered
 * by their priority.
 */
template <typename DelegateType>
class PriorityStrategy<void, DelegateType> {
 public:
  typedef DelegateType* DelegateHandle;
  typedef SharedPtr<DelegateType> DelegatePtr;
  typedef std::vector<DelegatePtr> Delegates;
  typedef typename Delegates::iterator Iterator;

 public:
  void Notify(const void* sender) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      (*it)->Notify(sender);
    }
  }

  DelegateHandle Add(const DelegateType& delegate) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      if ((*it)->GetPriority() > delegate.GetPriority()) {
        DelegatePtr delegate_ptr(static_cast<DelegateType*>(delegate.Clone()));
        delegates_.insert(it, delegate_ptr);
        return delegate_ptr.Get();
      }
    }
    DelegatePtr delegate_ptr(static_cast<DelegateType*>(delegate.Clone()));
    delegates_.push_back(delegate_ptr);
    return delegate_ptr.Get();
  }

  void Remove(const DelegateType& delegate) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      if (delegate.Equals(**it)) {
        (*it)->Disable();
        delegates_.erase(it);
        return;
      }
    }
  }

  void Remove(DelegateHandle delegate_handle) {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      if (*it == delegate_handle) {
        (*it)->Disable();
        delegates_.erase(it);
        return;
      }
    }
  }

  PriorityStrategy& operator = (const PriorityStrategy& s) {
    if (FUN_LIKELY(&s != this)) {
      delegates_ = s.delegates_;
    }
    return *this;
  }

  void Clear() {
    for (Iterator it = delegates_.begin(); it != delegates_.end(); ++it) {
      (*it)->Disable();
    }
    delegates_.clear();
  }

  bool IsEmpty() const {
    return delegates_.empty();
  }

 protected:
  Delegates delegates_;
};

} // namespace fun
