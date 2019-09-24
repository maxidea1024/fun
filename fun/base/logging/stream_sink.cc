#include "fun/base/logging/stream_sink.h"
#include "fun/base/logging/log_message.h"

namespace fun {

StreamSink::StreamSink(std::ostream& str) : str_(str) {}

StreamSink::~StreamSink() {}

void StreamSink::Log(const LogMessage& msg) {
  FastMutex::ScopedLock guard(mutex_);

  str_ << msg.GetText().c_str() << std::endl;
}

}  // namespace fun
