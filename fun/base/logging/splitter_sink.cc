#include "fun/base/logging/splitter_sink.h"
#include "fun/base/logging/logging_registry.h"

namespace fun {

SplitterSink::SplitterSink() {}

SplitterSink::~SplitterSink() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

void SplitterSink::AddSink(LogSink::Ptr sink) {
  fun_check_ptr(sink);

  ScopedLock<Mutex> guard(mutex_);
  sinks_.Add(sink);
}

void SplitterSink::RemoveSink(LogSink::Ptr sink) {
  fun_check_ptr(sink);

  ScopedLock<Mutex> guard(mutex_);
  sinks_.Remove(sink);
}

void SplitterSink::Log(const LogMessage& msg) {
  ScopedLock<Mutex> guard(mutex_);
  for (auto& sink : sinks_) {
    sink->Log(msg);
  }
}

void SplitterSink::SetProperty(const String& name, const String& value) {
  if (name.StartsWith("Sink", CaseSensitivity::IgnoreCase)) {
    Array<String> sink_names;
    sink_names = value.Split(",;", 0, StringSplitOption::TrimmingAndCullEmpty);
    for (const auto& sink_name : sink_names) {
      AddSink(LoggingRegistry::DefaultRegistry().SinkForName(sink_name));
    }
  } else {
    LogSink::SetProperty(name, value);
  }
}

void SplitterSink::Close() {
  ScopedLock<Mutex> guard(mutex_);
  sinks_.Clear();
}

int32 SplitterSink::GetSinkCount() const {
  ScopedLock<Mutex> guard(mutex_);
  return sinks_.Count();
}

} // namespace fun
