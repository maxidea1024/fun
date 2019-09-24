#pragma once

#include "fun/base/base.h"

namespace fun {

typedef Uuid EventId;
typedef Uuid EventTag;

extern const EventTag kNullEventTag;

class Events {
 public:
  typedef Function<void(const String& /*event_name*/,
                        const EventId& /*event_id*/,
                        const EventTag& /*event_tag*/,
                        const SharedPtr<Session>& /*associated_session*/
                        )>
      TimedoutHandler;

  template <typename EventTy>
  static void RegisterHandler(
      const Function<ChainAction(SharedPtr<const EventTy>)>& h) {
    VoidedHandler f;
    // TODO

    RegiserHandler(typeid(EventTy), f);
  }

  template <typename EventTy>
  static void RegisterHandler(ChainAction(handler)(SharedPtr<const EventTy>)) {
    RegiserHandler(typename EventTy::Handler(handler));
  }

  static void RegisterTimedoutHandler(const TimedoutHandler& handler);

  template <typename EventTy>
  static bool Post(SharedPtr<EventTy> event) {
    return Post(typeid(EventTy), event);
  }

  template <typename EventTy>
  static bool Post(SharedPtr<EventTy> event, const EventTag& event_tag) {
    return Post(typeid(EventTy), event, event_tag);
  }

  static void Resume(const EventTag& event_tag);

  static void Abort();

 private:
  typedef Function<ChainAction(SharedPtr<const void>)> VoidedHandler;

  static void RegisterHandler(const std::type_info&, const VoidedHandler&);
  static bool Post(const std::type_info&, SharedPtr<const void>);
  static bool Post(const std::type_info&, SharedPtr<const void>,
                   const EventTag& event_tag);
};

void SetCommonEventTag(const EventTag& tag);
bool IsCommonEventTag(const EventTag& tag);
const EventTag& GetCurrentEventTag();

// thread_local에 기록하면 되려나??
void DebugSetEventName(const String& name);
const String& DebugGetEventName();

}  // namespace fun
