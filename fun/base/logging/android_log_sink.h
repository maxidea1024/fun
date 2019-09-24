#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"

#if FUN_PLATFORM == FUN_PLATFORM_ANDROID

namespace fun {

/**
 * A logsink that writes to the Android log subsystem.
 *
 * Only the message's text is written, followed
 * by a newline, using the tag passed on the constructor.
 */
class FUN_BASE_API AndroidLogSink : public LogSink {
 public:
  /**
   * Creates the AndroidLogSink.
   */
  AndroidLogSink(const String& tag);

  // LogSink interface.
  void Log(const LogMessage& msg) override;

 protected:
  ~AndroidLogSink();

 private:
  String tag_;
};

} // namespace fun

#endif //#if FUN_PLATFORM == FUN_PLATFORM_ANDROID
