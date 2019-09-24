#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/logging/log_formatter.h"
#include "fun/base/mutex.h"
#include "fun/base/container/map.h"

namespace fun {

/**
 * A registry for sinks and formatters.
 *
 * The LoggingRegistry class is used for configuring
 * the logging framework.
 */
class FUN_BASE_API LoggingRegistry {
 public:
  /**
   * Creates the LoggingRegistry.
   */
  LoggingRegistry();

  /**
   * Destroys the LoggingRegistry.
   */
  ~LoggingRegistry();

  /**
   * Returns the Sink object which has been registered
   * under the given name.
   *
   * Throws a NotFoundException if the name is unknown.
   */
  LogSink::Ptr SinkForName(const String& name) const;

  /**
   * Returns the LogFormatter object which has been registered
   * under the given name.
   *
   * Throws a NotFoundException if the name is unknown.
   */
  LogFormatter::Ptr FormatterForName(const String& name) const;

  /**
   * Registers a sink under a given name.
   * It is okay to re-register a different sink under an
   * already existing name.
   */
  void RegisterSink(const String& name, LogSink::Ptr sink);

  /**
   * Registers a formatter under a given name.
   * It is okay to re-register a different formatter under an
   * already existing name.
   */
  void RegisterFormatter(const String& name, LogFormatter::Ptr formatter);

  /**
   * Unregisters the given sink.
   *
   * Throws a NotFoundException if the name is unknown.
   */
  void UnregisterSink(const String& name);

  /**
   * Unregisters the given formatter.
   *
   * Throws a NotFoundException if the name is unknown.
   */
  void UnregisterFormatter(const String& name);

  /**
   * Unregisters all registered sinks and formatters.
   */
  void Clear();

  /**
   * Returns a reference to the default
   * LoggingRegistry.
   */
  static LoggingRegistry& DefaultRegistry();

 private:
  FastMutex mutex_;
  Map<String, LogSink::Ptr> sinks_;
  Map<String, LogFormatter::Ptr> formatters_;
};

} // namespace fun
