#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate_base.h"

namespace fun {

/**
 * Base class for PriorityDelegate and PriorityExpire.
 *
 * Extends DelegateBase with a priority value.
 */
template <typename ArgsType>
class PriorityDelegateBase : public DelegateBase<ArgsType> {
 public:
  PriorityDelegateBase(int32 priority)
    : priority_(priority) {}

  PriorityDelegateBase(const PriorityDelegateBase& delegate)
    : DelegateBase<ArgsType>(delegate),
      priority_(delegate.priority_) {}

  virtual ~PriorityDelegateBase() {}

  int32 GetPriority() const {
    return priority_;
  }

 protected:
  int32 priority_;
};

} // namespace fun
