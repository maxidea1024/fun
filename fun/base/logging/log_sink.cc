#include "fun/base/logging/log_sink.h"
#include "fun/base/exception.h"

namespace fun {

LogSink::LogSink() {
  // NOOP
}

LogSink::~LogSink() {
  // NOOP
}

void LogSink::Open() {
  // NOOP
}

void LogSink::Close() {
  // NOOP
}

void LogSink::SetProperty(const String& name, const String& value) {
  throw PropertyNotSupportedException(name);
}

String LogSink::GetProperty(const String& name) const {
  throw PropertyNotSupportedException(name);
}

} // namespace fun
