#include "fun/base/logging/logging_factory.h"
#include "fun/base/logging/async_sink.h"
#include "fun/base/logging/file_sink.h"
#include "fun/base/logging/win_console_sink.h"
// TODO
//#include "fun/base/logging/simple_file_sink.h"
#include "fun/base/logging/formatting_sink.h"
#include "fun/base/logging/null_sink.h"
#include "fun/base/logging/splitter_sink.h"
// TODO
//#include "fun/base/logging/event_sink.h"
// TODO
//#include "fun/base/logging/event_log_sink.h"
#include "fun/base/logging/pattern_formatter.h"

namespace fun {

LoggingFactory::LoggingFactory() { RegisterBuiltins(); }

LoggingFactory::~LoggingFactory() {}

void LoggingFactory::RegisterSinkClass(const String& class_name,
                                       SinkFactory* factory) {
  sink_factory_.RegisterClass(class_name, factory);
}

void LoggingFactory::RegisterFormatterClass(const String& class_name,
                                            FormatterFactory* factory) {
  formatter_factory_.RegisterClass(class_name, factory);
}

LogSink::Ptr LoggingFactory::CreateSink(const String& class_name) const {
  return sink_factory_.CreateInstance(class_name);
  // return NewObject<LogSink>(class_name);
}

LogFormatter::Ptr LoggingFactory::CreateFormatter(
    const String& class_name) const {
  return formatter_factory_.CreateInstance(class_name);
  // return NewObject<LogFormatter>(class_name);
}

LoggingFactory& LoggingFactory::DefaultFactory() {
  static LoggingFactory instance;
  return instance;
}

void LoggingFactory::RegisterBuiltins() {
  sink_factory_.RegisterClass("AsyncSink",
                              new Instantiator<AsyncSink, LogSink>);

#if FUN_PLATFORM_WINDOWS_FAMILY
  sink_factory_.RegisterClass("ConsoleSink",
                              new Instantiator<WinConsoleSink, LogSink>);
  sink_factory_.RegisterClass("ColorConsoleSink",
                              new Instantiator<WinColorConsoleSink, LogSink>);
#else
  sink_factory_.RegisterClass("ConsoleSink",
                              new Instantiator<StdConsoleSink, LogSink>);
  sink_factory_.RegisterClass("ColorConsoleSink",
                              new Instantiator<StdColorConsoleSink, LogSink>);
#endif

  // TODO
  //#if !FUN_NO_FILECHANNEL
  //  sink_factory_.RegisterClass("FileSink", new Instantiator<FileSink,
  //  LogSink>); sink_factory_.RegisterClass("SimpleFileSink", new
  //  Instantiator<SimpleFileSink, LogSink>);
  //#endif

  sink_factory_.RegisterClass("FormattingSink",
                              new Instantiator<FormattingSink, LogSink>);

#if !FUN_NO_SPLITTERCHANNEL
  sink_factory_.RegisterClass("SplitterSink",
                              new Instantiator<SplitterSink, LogSink>);
#endif

  sink_factory_.RegisterClass("NullSink", new Instantiator<NullSink, LogSink>);
  // TODO
  // sink_factory_.RegisterClass("EventSink", new Instantiator<EventSink,
  // LogSink>);

#if FUN_PLTFORM_UNIX_FAMILY && !FUN_NO_SYSLOGCHANNEL
  sink_factory_.RegisterClass("SyslogSink",
                              new Instantiator<SyslogSink, LogSink>);
#endif

#if FUN_PLATFORM_WINDOWS_FAMILY
  // TODO
  // sink_factory_.RegisterClass("EventLogSink", new Instantiator<EventLogSink,
  // LogSink>);
#endif

  formatter_factory_.RegisterClass(
      "PatternFormatter", new Instantiator<PatternFormatter, LogFormatter>);
}

}  // namespace fun
