#include "fun/base/logging/logging_registry.h"
#include "fun/base/singleton.h"

namespace fun {

LoggingRegistry::LoggingRegistry() {}

LoggingRegistry::~LoggingRegistry() {}

LogSink::Ptr LoggingRegistry::SinkForName(const String& name) const {
  ScopedLock<FastMutex> guard(mutex_);

  LogSink::Ptr sink;
  if (!sinks_.TryGetValue(name, sink)) {
    throw NotFoundException("logging sink", name);
  }
  return sink;
}

LogFormatter::Ptr
LoggingRegistry::FormatterForName(const String& name) const {
  ScopedLock<FastMutex> guard(mutex_);

  LogFormatter::Ptr formatter;
  if (!formatters_.TryGetValue(name, formatter)) {
    throw NotFoundException("logging formatter", name);
  }
  return formatter;
}

void LoggingRegistry::RegisterSink(const String& name, LogSink::Ptr sink) {
  fun_check_ptr(sink);

  ScopedLock<FastMutex> guard(mutex_);

  sinks_.Add(name, sink);
}

void LoggingRegistry::RegisterFormatter(const String& name,
                                        LogFormatter::Ptr formatter) {
  fun_check_ptr(formatter);

  ScopedLock<FastMutex> guard(mutex_);

  formatters_.Add(name, formatter);
}

void LoggingRegistry::UnregisterSink(const String& name) {
  ScopedLock<FastMutex> guard(mutex_);

  sinks_.Remove(name);
}

void LoggingRegistry::UnregisterFormatter(const String& name) {
  ScopedLock<FastMutex> guard(mutex_);

  formatters_.Remove(name);
}

void LoggingRegistry::Clear() {
  ScopedLock<FastMutex> guard(mutex_);

  sinks_.Clear();
  formatters_.Clear();
}

LoggingRegistry& LoggingRegistry::DefaultRegistry() {
  static Singleton<LoggingRegistry>::Holder sh;
  return *sh.Get();
}

} // namespace fun
