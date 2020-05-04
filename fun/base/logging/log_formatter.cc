#include "fun/base/logging/log_formatter.h"

#include "fun/base/exception.h"

namespace fun {

LogFormatter::LogFormatter() {
  // NOOP
}

LogFormatter::~LogFormatter() {
  // NOOP
}

void LogFormatter::SetProperty(const String& name, const String& value) {
  throw PropertyNotSupportedException();
}

String LogFormatter::GetProperty(const String& name) const {
  throw PropertyNotSupportedException();
}

}  // namespace fun
