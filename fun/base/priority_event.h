#pragma once

#include "fun/base/event_base.h"
#include "fun/base/priority_delegate_base.h"
#include "fun/base/priority_strategy.h"

namespace fun {

/**
 * A PriorityEvent uses internally a PriorityStrategy which
 * invokes delegates in order of priority (lower priorities first).
 * PriorityEvent's can only be used together with PriorityDelegate's.
 * PriorityDelegate's are sorted according to the priority value, when
 * two delegates have the same priority, they are invoked in
 * an arbitrary manner.
 */
template <typename ArgsType, typename MutexType = FastMutex>
class PriorityEvent
    : public EventBase<
          ArgsType, PriorityStrategy<ArgsType, PriorityDelegateBase<ArgsType> >,
          PriorityDelegateBase<ArgsType>, MutexType> {
 public:
  PriorityEvent() {
    // NOOP
  }

  ~PriorityEvent() {
    // NOOP
  }

 private:
  PriorityEvent(const PriorityEvent&) = delete;
  PriorityEvent& operator=(const PriorityEvent&) = delete;
};

}  // namespace fun
