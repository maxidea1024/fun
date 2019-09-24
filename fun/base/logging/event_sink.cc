#include "fun/base/logging/event_sink.h"

#if FUN_WITH_EVENT_SINK

namespace fun {

EventSink::EventSink() {
  // NOOP
}

EventSink::~EventSink() {
  // NOOP
}

void EventSink::Log(const LogMessage& msg) { message_logged(msg); }

}  // namespace fun

#endif  // FUN_WITH_EVENT_SINK
