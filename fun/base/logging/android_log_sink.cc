#include "fun/base/logging/android_log_sink.h"

#if FUN_PLATFORM == FUN_PLATFORM_ANDROID

#include "fun/base/logging/log_message.h"

#include <android/log.h>

namespace fun {

AndroidLogSink::AndroidLogSink(const String& tag) : tag_(tag) {}

AndroidLogSink::~AndroidLogSink() {}

void AndroidLogSink::Log(const LogMessage& msg) {
  int prio = ANDROID_LOG_DEFAULT;
  switch (msg.GetLevel()) {
    case LogLevel::Fatal:
      prio = ANDROID_LOG_FATAL;
      break;
    case LogLevel::Critical:
    case LogLevel::Error:
      prio = ANDROID_LOG_ERROR;
      break;
    case LogLevel::Warning:
      prio = ANDROID_LOG_WARN;
      break;
    case LogLevel::Notice:
    case LogLevel::Information:
      prio = ANDROID_LOG_INFO;
      break;
    case LogLevel::Debug:
      prio = ANDROID_LOG_DEBUG;
      break;
    case LogLevel::Trace:
      prio = ANDROID_LOG_VERBOSE;
      break;
  }

  __android_log_write(prio, tag_.c_str(), msg.GetText().c_str());
}

} // namespace fun

#endif //#if FUN_PLATFORM == FUN_PLATFORM_ANDROID
