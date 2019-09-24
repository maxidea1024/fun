#pragma once

#include "fun/base/base.h"
#include "fun/base/container/map.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/mutex.h"
#include "fun/base/scoped_ptr.h"
#include "fun/base/string/string.h"

namespace fun {

class Exception;

/**
 * Logger is a special Sink that acts as the main
 * entry point into the logging framework.
 *
 * An application uses instances of the Logger class to generate its log
 * messages and send them on their way to their final destination. Logger
 * instances are organized in a hierarchical, tree-like manner and are
 * maintained by the framework. Every Logger object has exactly one direct
 * ancestor, with the exception of the root logger. A newly created logger
 * inherits its properties - sink and level - from its direct ancestor. Every
 * logger is connected to a sink, to which it passes on its messages.
 * Furthermore, every logger has a log level, which is used for filtering
 * messages based on their level. Only messages with a level equal to or higher
 * than the specified level are passed on. For example, if the level of a logger
 * is set to three (LogLevel::Error), only messages with level LogLevel::Error,
 * PRIO_CRITICAL and LogLevel::Fatal will propagate. If the level is set to
 * zero, the logger is effectively disabled.
 *
 * The name of a logger determines the logger's place within the logger
 * hierarchy. The name of the root logger is always "", the empty string. For
 * all other loggers, the name is made up of one or more components, separated
 * by a period. For example, the loggers with the name HTTPServer.RequestHandler
 * and HTTPServer.Listener are descendants of the logger HTTPServer, which
 * itself is a descendant of the root logger. There is no limit as to how deep
 * the logger hierarchy can become. Once a logger has been created and it has
 * inherited the sink and level from its ancestor, it loses the connection to
 * it. So, changes to the level or sink of a logger do not affect its
 * descendants. This greatly simplifies the implementation of the framework and
 * is no real restriction, because almost always levels and sinks are set up at
 * application startup and never changed afterwards. Nevertheless, there are
 * methods to simultaneously change the level and sink of all loggers in a
 * certain hierarchy.
 *
 * There are also convenience macros available that wrap the actual
 * logging statement into a check whether the Logger's log level
 * is sufficient to actually log the message. This allows to increase
 * the application performance if many complex log statements
 * are used. The macros also add the source file path and line
 * number into the log message so that it is available to formatters.
 * Variants of these macros that allow message formatting with fun::Format()
 * are also available. Up to four arguments are supported.
 *
 * Examples:
 *     fun_warning(logger, "This is a warning");
 *     fun_information_f2(logger, "An informational message with args: %d, %d",
 * 1, 2);
 */
class FUN_BASE_API Logger : public LogSink {
 public:
  using Ptr = RefCountedPtr<Logger>;

  /**
   * Returns the name of the logger, which is set as the
   * message source on all messages created by the logger.
   */
  const String& GetName() const;

  /**
   * Attaches the given Sink to the Logger.
   */
  void SetSink(LogSink::Ptr sink);

  /**
   * Returns the Sink attached to the logger.
   */
  LogSink::Ptr GetSink() const;

  /**
   * Sets the Logger's log level.
   *
   * See LogLevel for valid log levels.
   * Setting the log level to zero turns off
   * logging for that Logger.
   */
  void SetLevel(LogLevel::Type level);

  /**
   * Returns the Logger's log level.
   */
  LogLevel::Type GetLevel() const;

  /**
   * Sets the Logger's log level using a symbolic value.
   *
   * Valid values are:
   *   - None (turns off logging)
   *   - Fatal
   *   - Critical
   *   - Error
   *   - Warning
   *   - Notice
   *   - Information
   *   - Debug
   *   - Trace
   */
  void SetLevel(const String& level);

  /**
   * Sets or changes a configuration property.
   *
   * Only the "Sink" and "Level" properties are supported, which allow
   * setting the target sink and log level, respectively, via the
   * LoggingRegistry. The "Sink" and "Level" properties are set-only.
   */
  void SetProperty(const String& name, const String& value) override;

  //
  // Log
  //

  /**
   * Logs the given message if its level is
   * greater than or equal to the Logger's log level.
   */
  void Log(const LogMessage& msg) override;

  /**
   * Logs the given exception with level LogLevel::Error.
   */
  void Log(const Exception& e);

  /**
   * Logs the given exception with level LogLevel::Error.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void Log(const Exception& e, const char* file, int32 line);

  //
  // Fatal
  //

  /**
   * If the Logger's log level is at least LogLevel::Fatal,
   * creates a LogMessage with level LogLevel::Fatal
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogFatal(const String& msg);

  /**
   * If the Logger's log level is at least LogLevel::Fatal,
   * creates a LogMessage with level LogLevel::Fatal
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogFatal(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogFatal(const String& fmt, const Args&... args) {
    // TODO
    // LogLog(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Fatal);
  }

  //
  // Critical
  //

  /**
   * If the Logger's log level is at least PRIO_CRITICAL,
   * creates a LogMessage with level PRIO_CRITICAL
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogCritical(const String& msg);

  /**
   * If the Logger's log level is at least PRIO_CRITICAL,
   * creates a LogMessage with level PRIO_CRITICAL
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogCritical(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogCritical(const String& fmt, const Args&... args) {
    // TODO
    // LogCritical(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Critical);
  }

  //
  // Error
  //

  /**
   * If the Logger's log level is at least LogLevel::Error,
   * creates a LogMessage with level LogLevel::Error
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogError(const String& msg);

  /**
   * If the Logger's log level is at least LogLevel::Error,
   * creates a LogMessage with level LogLevel::Error
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogError(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogError(const String& fmt, const Args&... args) {
    // TODO
    // LogError(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Error);
  }

  //
  // Warning
  //

  /**
   * If the Logger's log level is at least PRIO_WARNING,
   * creates a LogMessage with level PRIO_WARNING
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogWarning(const String& msg);

  /**
   * If the Logger's log level is at least PRIO_WARNING,
   * creates a LogMessage with level PRIO_WARNING
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogWarning(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogWarning(const String& fmt, const Args&... args) {
    // TODO
    // LogWarning(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Warning);
  }

  //
  // Notice
  //

  /**
   * If the Logger's log level is at least PRIO_NOTICE,
   * creates a LogMessage with level PRIO_NOTICE
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogNotice(const String& msg);

  /**
   * If the Logger's log level is at least PRIO_NOTICE,
   * creates a LogMessage with level PRIO_NOTICE
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogNotice(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogNotice(const String& fmt, const Args&... args) {
    // TODO
    // LogNotice(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Notice);
  }

  //
  // Information
  //

  /**
   * If the Logger's log level is at least PRIO_INFORMATION,
   * creates a LogMessage with level PRIO_INFORMATION
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogInformation(const String& msg);

  /**
   * If the Logger's log level is at least PRIO_INFORMATION,
   * creates a LogMessage with level PRIO_INFORMATION
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogInformation(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogInformation(const String& fmt, const Args&... args) {
    // TODO
    // LogInformation(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Information);
  }

  //
  // Debug
  //

  /**
   * If the Logger's log level is at least PRIO_DEBUG,
   * creates a LogMessage with level PRIO_DEBUG
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogDebug(const String& msg);

  /**
   * If the Logger's log level is at least PRIO_DEBUG,
   * creates a LogMessage with level PRIO_DEBUG
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogDebug(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogDebug(const String& fmt, const Args&... args) {
    // TODO
    // LogDebug(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Debug);
  }

  //
  // Trace
  //

  /**
   * If the Logger's log level is at least PRIO_TRACE,
   * creates a LogMessage with level PRIO_TRACE
   * and the given message text and sends it
   * to the attached sink.
   */
  void LogTrace(const String& msg);

  /**
   * If the Logger's log level is at least PRIO_TRACE,
   * creates a LogMessage with level PRIO_TRACE
   * and the given message text and sends it
   * to the attached sink.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void LogTrace(const String& msg, const char* file, int32 line);

  template <typename... Args>
  void LogTrace(const String& fmt, const Args&... args) {
    // TODO
    // LogTrace(sf::Format(fmt, sf::MakeFormatArgs<Args...>(args...)),
    // LogLevel::Trace);
  }

  void Dump(const String& msg, const void* buffer, size_t length,
            LogLevel::Type level = LogLevel::Debug);

  bool IsEnabledFor(LogLevel::Type level) const;

  bool IsEnabledForFatal() const;
  bool IsEnabledForCritical() const;
  bool IsEnabledForError() const;
  bool IsEnabledForWarning() const;
  bool IsEnabledForNotice() const;
  bool IsEnabledForInformation() const;
  bool IsEnabledForDebug() const;
  bool IsEnabledForTrace() const;

  // static String Format(const String& fmt, const String& arg);
  // static String Format(const String& fmt, const String& arg0, const String&
  // arg1); static String Format(const String& fmt, const String& arg0, const
  // String& arg1, const String& arg2); static String Format(const String& fmt,
  // const String& arg0, const String& arg1, const String& arg2, const String&
  // arg3);

  static void FormatDump(String& message, const void* buffer, size_t length);

  static void SetLevel(const String& logger_name, LogLevel::Type level);
  static void SetSink(const String& logger_name, LogSink::Ptr sink);
  static void SetProperty(const String& logger_name,
                          const String& property_name, const String& value);
  static Logger& Get(const String& logger_name);
  static Logger& UnsafeGet(const String& logger_name);

  static Logger& Create(const String& logger_name, LogSink::Ptr sink,
                        LogLevel::Type level = LogLevel::Information);

  static Logger& Root();

  static Ptr Has(const String& logger_name);

  static void Destroy(const String& logger_name);

  static void Shutdown();

  static void Names(Array<String>& logger_names);

  static LogLevel::Type ParseLevel(const String& level);

  /**
   * The name of the root logger ("").
   */
  static const String ROOT;

 protected:
  typedef Map<String, Ptr> LoggerMap;

  Logger(const String& name, LogSink::Ptr sink, LogLevel::Type level);
  ~Logger();

  void Log(const String& text, LogLevel::Type level);
  void Log(const String& text, LogLevel::Type level, const char* file,
           int line);

  // static String Format(const String& fmt, int argc, String argv[]);

  static Logger& GetParent(const String& logger_name);
  static void Add(Ptr logger);
  static Ptr Find(const String& logger_name);

 private:
  Logger() = delete;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  String name_;
  LogSink::Ptr sink_;
  LogLevel::Type level_;

  // definitions in base.cc
  static ScopedPtr<LoggerMap> logger_map_;
  static Mutex logger_map_mutex_;
};

//
// convenience macros
//

#define fun_fatal(logger, msg)                  \
  if ((logger).IsEnabledForFatal())             \
    (logger).LogFatal(msg, __FILE__, __LINE__); \
  else                                          \
    (void)0;

#define fun_fatal_f1(logger, fmt, arg1)                                 \
  if ((logger).IsEnabledForFatal())                                     \
    (logger).LogFatal(String::Format((fmt), arg1), __FILE__, __LINE__); \
  else                                                                  \
    (void)0;

#define fun_fatal_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForFatal())                                    \
    (logger).LogFatal(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                      __LINE__);                                       \
  else                                                                 \
    (void)0;

#define fun_fatal_f3(logger, fmt, arg1, arg2, arg3)                            \
  if ((logger).IsEnabledForFatal())                                            \
    (logger).LogFatal(String::Format((fmt), (arg1), (arg2), (arg3)), __FILE__, \
                      __LINE__);                                               \
  else                                                                         \
    (void)0;

#define fun_fatal_f4(logger, fmt, arg1, arg2, arg3, arg4)                    \
  if ((logger).IsEnabledForFatal())                                          \
    (logger).LogFatal(String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), \
                      __FILE__, __LINE__);                                   \
  else                                                                       \
    (void)0;

#define fun_critical(logger, msg)                  \
  if ((logger).IsEnabledForCritical())             \
    (logger).LogCritical(msg, __FILE__, __LINE__); \
  else                                             \
    (void)0;

#define fun_critical_f1(logger, fmt, arg1)                                   \
  if ((logger).IsEnabledForCritical())                                       \
    (logger).LogCritical(String::Format((fmt), (arg1)), __FILE__, __LINE__); \
  else                                                                       \
    (void)0;

#define fun_critical_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForCritical())                                    \
    (logger).LogCritical(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                         __LINE__);                                       \
  else                                                                    \
    (void)0;

#define fun_critical_f3(logger, fmt, arg1, arg2, arg3)                  \
  if ((logger).IsEnabledForCritical())                                  \
    (logger).LogCritical(String::Format((fmt), (arg1), (arg2), (arg3)), \
                         __FILE__, __LINE__);                           \
  else                                                                  \
    (void)0;

#define fun_critical_f4(logger, fmt, arg1, arg2, arg3, arg4)             \
  if ((logger).IsEnabledForCritical())                                   \
    (logger).LogCritical(                                                \
        String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), __FILE__, \
        __LINE__);                                                       \
  else                                                                   \
    (void)0;

#define fun_error(logger, msg)                  \
  if ((logger).IsEnabledForError())             \
    (logger).LogError(msg, __FILE__, __LINE__); \
  else                                          \
    (void)0;

#define fun_error_f1(logger, fmt, arg1)                                   \
  if ((logger).IsEnabledForError())                                       \
    (logger).LogError(String::Format((fmt), (arg1)), __FILE__, __LINE__); \
  else                                                                    \
    (void)0;

#define fun_error_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForError())                                    \
    (logger).LogError(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                      __LINE__);                                       \
  else                                                                 \
    (void)0;

#define fun_error_f3(logger, fmt, arg1, arg2, arg3)                            \
  if ((logger).IsEnabledForError())                                            \
    (logger).LogError(String::Format((fmt), (arg1), (arg2), (arg3)), __FILE__, \
                      __LINE__);                                               \
  else                                                                         \
    (void)0;

#define fun_error_f4(logger, fmt, arg1, arg2, arg3, arg4)                    \
  if ((logger).IsEnabledForError())                                          \
    (logger).LogError(String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), \
                      __FILE__, __LINE__);                                   \
  else                                                                       \
    (void)0;

#define fun_warning(logger, msg)                  \
  if ((logger).IsEnabledForWarning())             \
    (logger).LogWarning(msg, __FILE__, __LINE__); \
  else                                            \
    (void)0;

#define fun_warning_f1(logger, fmt, arg1)                                   \
  if ((logger).IsEnabledForWarning())                                       \
    (logger).LogWarning(String::Format((fmt), (arg1)), __FILE__, __LINE__); \
  else                                                                      \
    (void)0;

#define fun_warning_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForWarning())                                    \
    (logger).LogWarning(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                        __LINE__);                                       \
  else                                                                   \
    (void)0;

#define fun_warning_f3(logger, fmt, arg1, arg2, arg3)                  \
  if ((logger).IsEnabledForWarning())                                  \
    (logger).LogWarning(String::Format((fmt), (arg1), (arg2), (arg3)), \
                        __FILE__, __LINE__);                           \
  else                                                                 \
    (void)0;

#define fun_warning_f4(logger, fmt, arg1, arg2, arg3, arg4)                    \
  if ((logger).IsEnabledForWarning())                                          \
    (logger).LogWarning(String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), \
                        __FILE__, __LINE__);                                   \
  else                                                                         \
    (void)0;

#define fun_notice(logger, msg)                  \
  if ((logger).IsEnabledForNotice())             \
    (logger).LogNotice(msg, __FILE__, __LINE__); \
  else                                           \
    (void)0;

#define fun_notice_f1(logger, fmt, arg1)                                   \
  if ((logger).IsEnabledForNotice())                                       \
    (logger).LogNotice(String::Format((fmt), (arg1)), __FILE__, __LINE__); \
  else                                                                     \
    (void)0;

#define fun_notice_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForNotice())                                    \
    (logger).LogNotice(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                       __LINE__);                                       \
  else                                                                  \
    (void)0;

#define fun_notice_f3(logger, fmt, arg1, arg2, arg3)                  \
  if ((logger).IsEnabledForNotice())                                  \
    (logger).LogNotice(String::Format((fmt), (arg1), (arg2), (arg3)), \
                       __FILE__, __LINE__);                           \
  else                                                                \
    (void)0;

#define fun_notice_f4(logger, fmt, arg1, arg2, arg3, arg4)                    \
  if ((logger).IsEnabledForNotice())                                          \
    (logger).LogNotice(String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), \
                       __FILE__, __LINE__);                                   \
  else                                                                        \
    (void)0;

#define fun_information(logger, msg)                  \
  if ((logger).IsEnabledForInformation())             \
    (logger).LogInformation(msg, __FILE__, __LINE__); \
  else                                                \
    (void)0;

#define fun_information_f1(logger, fmt, arg1)                        \
  if ((logger).IsEnabledForInformation())                            \
    (logger).LogInformation(String::Format((fmt), (arg1)), __FILE__, \
                            __LINE__);                               \
  else                                                               \
    (void)0;

#define fun_information_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForInformation())                                    \
    (logger).LogInformation(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                            __LINE__);                                       \
  else                                                                       \
    (void)0;

#define fun_information_f3(logger, fmt, arg1, arg2, arg3)                  \
  if ((logger).IsEnabledForInformation())                                  \
    (logger).LogInformation(String::Format((fmt), (arg1), (arg2), (arg3)), \
                            __FILE__, __LINE__);                           \
  else                                                                     \
    (void)0;

#define fun_information_f4(logger, fmt, arg1, arg2, arg3, arg4)          \
  if ((logger).IsEnabledForInformation())                                \
    (logger).LogInformation(                                             \
        String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), __FILE__, \
        __LINE__);                                                       \
  else                                                                   \
    (void)0;

#if defined(_DEBUG) || defined(FUN_LOG_DEBUG)
#define fun_debug(logger, msg)                  \
  if ((logger).IsEnabledForDebug())             \
    (logger).LogDebug(msg, __FILE__, __LINE__); \
  else                                          \
    (void)0;

#define fun_debug_f1(logger, fmt, arg1)                                   \
  if ((logger).IsEnabledForDebug())                                       \
    (logger).LogDebug(String::Format((fmt), (arg1)), __FILE__, __LINE__); \
  else                                                                    \
    (void)0;

#define fun_debug_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForDebug())                                    \
    (logger).LogDebug(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                      __LINE__);                                       \
  else                                                                 \
    (void)0;

#define fun_debug_f3(logger, fmt, arg1, arg2, arg3)                            \
  if ((logger).IsEnabledForDebug())                                            \
    (logger).LogDebug(String::Format((fmt), (arg1), (arg2), (arg3)), __FILE__, \
                      __LINE__);                                               \
  else                                                                         \
    (void)0;

#define fun_debug_f4(logger, fmt, arg1, arg2, arg3, arg4)                    \
  if ((logger).IsEnabledForDebug())                                          \
    (logger).LogDebug(String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), \
                      __FILE__, __LINE__);                                   \
  else                                                                       \
    (void)0;

#define fun_trace(logger, msg)                  \
  if ((logger).IsEnabledForTrace())             \
    (logger).LogTrace(msg, __FILE__, __LINE__); \
  else                                          \
    (void)0;

#define fun_trace_f1(logger, fmt, arg1)                                   \
  if ((logger).IsEnabledForTrace())                                       \
    (logger).LogTrace(String::Format((fmt), (arg1)), __FILE__, __LINE__); \
  else                                                                    \
    (void)0;

#define fun_trace_f2(logger, fmt, arg1, arg2)                          \
  if ((logger).IsEnabledForTrace())                                    \
    (logger).LogTrace(String::Format((fmt), (arg1), (arg2)), __FILE__, \
                      __LINE__);                                       \
  else                                                                 \
    (void)0;

#define fun_trace_f3(logger, fmt, arg1, arg2, arg3)                            \
  if ((logger).IsEnabledForTrace())                                            \
    (logger).LogTrace(String::Format((fmt), (arg1), (arg2), (arg3)), __FILE__, \
                      __LINE__);                                               \
  else                                                                         \
    (void)0;

#define fun_trace_f4(logger, fmt, arg1, arg2, arg3, arg4)                    \
  if ((logger).IsEnabledForTrace())                                          \
    (logger).LogTrace(String::Format((fmt), (arg1), (arg2), (arg3), (arg4)), \
                      __FILE__, __LINE__);                                   \
  else                                                                       \
    (void)0;
#else
#define fun_debug(logger, msg)
#define fun_debug_f1(logger, fmt, arg1)
#define fun_debug_f2(logger, fmt, arg1, arg2)
#define fun_debug_f3(logger, fmt, arg1, arg2, arg3)
#define fun_debug_f4(logger, fmt, arg1, arg2, arg3, arg4)
#define fun_trace(logger, msg)
#define fun_trace_f1(logger, fmt, arg1)
#define fun_trace_f2(logger, fmt, arg1, arg2)
#define fun_trace_f3(logger, fmt, arg1, arg2, arg3)
#define fun_trace_f4(logger, fmt, arg1, arg2, arg3, arg4)
#endif

//
// inlines
//

FUN_ALWAYS_INLINE const String& Logger::GetName() const { return name_; }

FUN_ALWAYS_INLINE LogLevel::Type Logger::GetLevel() const { return level_; }

FUN_ALWAYS_INLINE void Logger::Log(const String& text, LogLevel::Type level) {
  if (level_ >= level && sink_) {
    sink_->Log(LogMessage(name_, text, level));
  }
}

FUN_ALWAYS_INLINE void Logger::Log(const String& text, LogLevel::Type level,
                                   const char* file, int line) {
  if (level_ >= level && sink_) {
    sink_->Log(LogMessage(name_, text, level, file, line));
  }
}

FUN_ALWAYS_INLINE void Logger::LogFatal(const String& msg) {
  Log(msg, LogLevel::Fatal);
}

FUN_ALWAYS_INLINE void Logger::LogFatal(const String& msg, const char* file,
                                        int line) {
  Log(msg, LogLevel::Fatal, file, line);
}

FUN_ALWAYS_INLINE void Logger::LogCritical(const String& msg) {
  Log(msg, LogLevel::Critical);
}

FUN_ALWAYS_INLINE void Logger::LogCritical(const String& msg, const char* file,
                                           int line) {
  Log(msg, LogLevel::Critical, file, line);
}

FUN_ALWAYS_INLINE void Logger::LogError(const String& msg) {
  Log(msg, LogLevel::Error);
}

FUN_ALWAYS_INLINE void Logger::LogError(const String& msg, const char* file,
                                        int line) {
  Log(msg, LogLevel::Error, file, line);
}

FUN_ALWAYS_INLINE void Logger::LogWarning(const String& msg) {
  Log(msg, LogLevel::Warning);
}

FUN_ALWAYS_INLINE void Logger::LogWarning(const String& msg, const char* file,
                                          int line) {
  Log(msg, LogLevel::Warning, file, line);
}

FUN_ALWAYS_INLINE void Logger::LogNotice(const String& msg) {
  Log(msg, LogLevel::Notice);
}

FUN_ALWAYS_INLINE void Logger::LogNotice(const String& msg, const char* file,
                                         int line) {
  Log(msg, LogLevel::Notice, file, line);
}

FUN_ALWAYS_INLINE void Logger::LogInformation(const String& msg) {
  Log(msg, LogLevel::Information);
}

FUN_ALWAYS_INLINE void Logger::LogInformation(const String& msg,
                                              const char* file, int line) {
  Log(msg, LogLevel::Information, file, line);
}

FUN_ALWAYS_INLINE void Logger::LogDebug(const String& msg) {
  Log(msg, LogLevel::Debug);
}

FUN_ALWAYS_INLINE void Logger::LogDebug(const String& msg, const char* file,
                                        int line) {
  Log(msg, LogLevel::Debug, file, line);
}

FUN_ALWAYS_INLINE void Logger::LogTrace(const String& msg) {
  Log(msg, LogLevel::Trace);
}

FUN_ALWAYS_INLINE void Logger::LogTrace(const String& msg, const char* file,
                                        int line) {
  Log(msg, LogLevel::Trace, file, line);
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledFor(LogLevel::Type level) const {
  return level_ >= level;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForFatal() const {
  return level_ >= LogLevel::Fatal;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForCritical() const {
  return level_ >= LogLevel::Critical;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForError() const {
  return level_ >= LogLevel::Error;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForWarning() const {
  return level_ >= LogLevel::Warning;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForNotice() const {
  return level_ >= LogLevel::Notice;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForInformation() const {
  return level_ >= LogLevel::Information;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForDebug() const {
  return level_ >= LogLevel::Debug;
}

FUN_ALWAYS_INLINE bool Logger::IsEnabledForTrace() const {
  return level_ >= LogLevel::Trace;
}

}  // namespace fun
