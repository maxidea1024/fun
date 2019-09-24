#include "fun/base/logging/null_sink.h"

namespace fun {

NullSink::NullSink() {
  // NOOP
}

NullSink::~NullSink() {
  // NOOP
}

void NullSink::Log(const LogMessage& msg) {
  // NOOP
}

void NullSink::SetProperty(const String& name, const String& value) {
  // NOOP
}

} // namespace fun
