#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/logging/log_formatter.h"
#include "fun/base/dynamic_factory.h"
#include "fun/base/ftl/shared_ptr.h"

namespace fun {

/**
 * An extensible factory for sinks and formatters.
 *
 * The following sink classes are pre-registered:
 *   - AsyncSink
 *   - ConsoleSink
 *   - EventLogSink (Windows platforms only)
 *   - FileSink
 *   - FormattingSink
 *   - NullSink
 *   - SplitterSink
 *   - SyslogSink (Unix platforms only)
 *
 * The following formatter classes are pre-registered:
 *   - PatternFormatter
 */
class FUN_BASE_API LoggingFactory {
 public:
  using SinkFactory = InstantiatorBase<LogSink>;
  using FormatterFactory = InstantiatorBase<LogFormatter>;

  /**
   * Creates the LoggingFactory.
   *
   * Automatically registers class factories for the
   * built-in sink and formatter classes.
   */
  LoggingFactory();

  /**
   * Destroys the LoggingFactory.
   */
  ~LoggingFactory();

  /**
   * Registers a sink class with the LoggingFactory.
   */
  void RegisterSinkClass(const String& class_name, SinkFactory* factory);

  /**
   * Registers a formatter class with the LoggingFactory.
   */
  void RegisterFormatterClass(const String& class_name, FormatterFactory* factory);

  /**
   * Creates a new Sink instance from specified class.
   *
   * Throws a NotFoundException if the specified sink class
   * has not been registered.
   */
  LogSink::Ptr CreateSink(const String& class_name) const;

  /**
   * Creates a new LogFormatter instance from specified class.
   *
   * Throws a NotFoundException if the specified formatter class
   * has not been registered.
   */
  LogFormatter::Ptr CreateFormatter(const String& class_name) const;

  /**
   * Returns a reference to the default
   * LoggingFactory.
   */
  static LoggingFactory& DefaultFactory();

 private:
  void RegisterBuiltins();

  DynamicFactory<LogSink> sink_factory_;
  DynamicFactory<LogFormatter> formatter_factory_;
};

} // namespace fun
