#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"

namespace fun {

/**
 * The NullSink is the /dev/null of sinks.
 * 
 * A NullSink discards all information sent to it.
 * Furthermore, its SetProperty method ignores
 * all properties, so it the NullSink has the
 * nice feature that it can stand in for any
 * other sink class in a logging configuration.
 */
class FUN_BASE_API NullSink : public LogSink {
 public:
  NullSink();
  ~NullSink();

  // LogSink interface.
  void Log(const LogMessage& msg) override;

  void SetProperty(const String& name, const String& value) override;
};

} // namespace fun
