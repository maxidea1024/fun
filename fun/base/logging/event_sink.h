#pragma once

#include "fun/base/base.h"

#define FUN_WITH_EVENT_SINK 0

#if FUN_WITH_EVENT_SINK

#include "fun/base/default_strategy.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/multicast_event.h"
#include "fun/base/runtime_class.h"

namespace fun {

/**
 * The EventSink fires the message_logged event for every log message
 * received. This can be used to hook custom log message processing into
 * the logging framework.
 */
class FUN_BASE_API EventSink : public LogSink {
 public:
  // TODO 범용 delegate로 교체하는게 좋을듯 한데...
  MulticastEvent<const LogMessage> message_logged;

  EventSink();

  // LogSink interface.
  void Log(const LogMessage& msg) override;

 protected:
  ~EventSink();
};

}  // namespace fun

#endif  // FUN_WITH_EVENT_SINK
