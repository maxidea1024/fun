#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_formatter.h"
#include "fun/base/logging/log_sink.h"

namespace fun {

// class LogFormatter;

/**
 * The FormattingSink is a filter sink that routes
 * a LogMessage through a LogFormatter before passing it on
 * to the destination sink.
 */
class FUN_BASE_API FormattingSink : public LogSink {
  // TODO
  // FUN_DECLARE_RTCLASS(FormattingSink, LogSink)

 public:
  using Ptr = RefCountedPtr<FormattingSink>;

  /** Creates a FormattingSink. */
  FormattingSink();

  /**
   * Creates a FormattingSink and attaches a LogFormatter.
   */
  FormattingSink(LogFormatter::Ptr formatter);

  /**
   * Creates a FormattingSink and attaches a LogFormatter and a sink.
   */
  FormattingSink(LogFormatter::Ptr formatter, LogSink::Ptr sink);

  /**
   * Sets the LogFormatter used to format the messages
   * before they are passed on. If null, the message
   * is passed on unmodified.
   */
  void SetFormatter(LogFormatter::Ptr formatter);

  /**
   * Returns the LogFormatter used to format messages,
   * which may be null.
   */
  LogFormatter::Ptr GetFormatter() const;

  /**
   * Sets the destination sink to which the formatted
   * messages are passed on.
   */
  void SetSink(LogSink::Ptr sink);

  /**
   * Returns the sink to which the formatted
   * messages are passed on.
   */
  LogSink::Ptr GetSink() const;

  /**
   * Formats the given LogMessage using the LogFormatter and
   * passes the formatted message on to the destination sink.
   */
  void Log(const LogMessage& msg) override;

  /**
   * Sets or changes a configuration property.
   *
   * Only the "Sink" and "Formatter" properties are supported, which allow
   * setting the target sink and formatter, respectively, via the
   * LoggingRegistry. The "Sink" and "Formatter" properties are set-only.
   *
   * Unsupported properties are passed to the attached sink.
   */
  void SetProperty(const String& name, const String& value) override;

  /** Opens the attached sink. */
  void Open() override;

  /** Closes the attached sink. */
  void Close() override;

 private:
  LogFormatter::Ptr formatter_;
  LogSink::Ptr sink_;
};

}  // namespace fun
