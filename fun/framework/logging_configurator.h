#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/configuration_base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/logging/log_formatter.h"

namespace fun {
namespace framework {

//TODO private code임과 동시에 static으로 해도 무방할듯...

/**
 * This utility class uses a configuration object to configure the
 * logging subsystem of an application.
 *
 * The LoggingConfigurator sets up and connects formatters, sinks
 * and loggers. To accomplish its work, the LoggingConfigurator relies on the
 * functionality provided by the LoggingFactory and LoggingRegistry classes.
 *
 * The LoggingConfigurator expects all configuration data to be under a root
 * property named "logging".
 *
 * Configuring Formatters
 *
 * A formatter is configured using the "logging.formatters" property. Every
 * formatter has an internal name, which is only used for referring to it
 * during configuration time. This name becomes part of the property name.
 * Every formatter has a mandatory "class" property, which specifies the actual
 * class implementing the formatter. Any other properties are passed on to
 * the formatter by calling its SetProperty() method.
 *
 * A typical formatter definition looks as follows:
 *     logging.formatters.f1.class = PatternFormatter
 *     logging.formatters.f1.pattern = %s: [%p] %t
 *     logging.formatters.f1.times = UTC
 *
 * Configuring Sinks
 *
 * A sink is configured using the "logging.sinks" property. Like with
 * Formatters, every sink has an internal name, which is used during
 * configuration only. The name becomes part of the property name.
 * Every sink has a mandatory "class" property, which specifies the actual
 * class implementing the sink. Any other properties are passed on to
 * the formatter by calling its SetProperty() method.
 *
 * For convenience, the "formatter" property of a sink is treated
 * specifically. The "formatter" property can either be used to refer to
 * an already defined formatter, or it can be used to specify an "FUN_ALWAYS_INLINE"
 * formatter definition. In either case, when a "formatter" property is
 * present, the sink is automatically "wrapped" in a FormattingSink
 * object.
 *
 * Similarly, a sink supports also a "pattern" property, which results
 * in the automatic instantiation of a FormattingSink object with a
 * connected PatternFormatter.
 *
 * Examples:
 *  logging.sinks.c1.class = ConsoleSink
 *  logging.sinks.c1.formatter = f1
 *  logging.sinks.c2.class = FileSink
 *  logging.sinks.c2.path = ${system.temp_dir}/sample.log
 *  logging.sinks.c2.formatter.class = PatternFormatter
 *  logging.sinks.c2.formatter.pattern = %s: [%p] %t
 *  logging.sinks.c3.class = ConsoleSink
 *  logging.sinks.c3.pattern = %s: [%p] %t
 *
 * Configuring Loggers
 *
 * A logger is configured using the "logging.loggers" property. Like with
 * sinks and formatters, every logger has an internal name, which, however,
 * is only used to ensure the uniqueness of the property names. Note that this
 * name is different from the logger's full name, which is used to access
 * the logger at runtime.
 * Every logger except the root logger has a mandatory "name" property which
 * is used to specify the logger's full name.
 * Furthermore, a "sink" property is supported, which can either refer
 * to a named sink, or which can contain an FUN_ALWAYS_INLINE sink definition.
 *
 * Examples:
 *  logging.loggers.root.sink = c1
 *  logging.loggers.root.level = warning
 *  logging.loggers.l1.name = logger1
 *  logging.loggers.l1.sink.class = ConsoleSink
 *  logging.loggers.l1.sink.pattern = %s: [%p] %t
 *  logging.loggers.l1.level = information
 */
class FUN_FRAMEWORK_API LoggingConfigurator {
 public:
  LoggingConfigurator();
  ~LoggingConfigurator();

  /**
   * Configures the logging subsystem based on the given configuration.
   *
   * A ConfigurationView can be used to pass only a part of a larger configuration.
   */
  void Configure(ConfigurationBase::Ptr config);

  // Disable copy and assignment.
  LoggingConfigurator(const LoggingConfigurator&) = delete;
  LoggingConfigurator& operator = (const LoggingConfigurator&) = delete;

 private:
  void ConfigureFormatters(ConfigurationBase::Ptr config);
  void ConfigureSinks(ConfigurationBase::Ptr config);
  void ConfigureLoggers(ConfigurationBase::Ptr config);
  LogFormatter::Ptr CreateFormatter(ConfigurationBase::Ptr config);
  LogSink::Ptr CreateSink(ConfigurationBase::Ptr config);
  void ConfigureSink(LogSink::Ptr sink, ConfigurationBase::Ptr config);
  void ConfigureLogger(ConfigurationBase::Ptr config);
};

} // namespace framework
} // namespace fun
