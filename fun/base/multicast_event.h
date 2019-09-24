#pragma once

#include "fun/base/base.h"
#include "fun/base/default_strategy.h"
#include "fun/base/delegate_base.h"
#include "fun/base/event_base.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * A MulticastEvent uses the DefaultStrategy which
 * invokes delegates in the order they have been registered.
 * 
 * Please see the DelegateBase class template documentation
 * for more information.
 */
template <typename ArgsType, typename MutexType = FastMutex>
class MulticastEvent
  : public EventBase<
      ArgsType,
      DefaultStrategy<ArgsType, DelegateBase<ArgsType>>,
      DelegateBase<ArgsType>,
      MutexType
    > {
 public:
  MulticastEvent() {
    // NOOP
  }

  ~MulticastEvent() {
    // NOOP
  }

  // Disable copy.
  MulticastEvent(const MulticastEvent&) = delete;
  MulticastEvent& operator = (const MulticastEvent&) = delete;
};

} // namespace fun
