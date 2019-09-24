#pragma once

#include "fun/base/base.h"
#include "fun/base/configurable.h"
#include "fun/base/ref_counted.h"

namespace fun {

class LogMessage;

/**
 * The base class for all Sink classes.
 *
 * Supports reference counting based garbage
 * collection and provides trivial implementations
 * of GetProperty() and SetProperty().
 */
class FUN_BASE_API LogSink : public Configurable, public RefCountedObject {
 public:
  using Ptr = RefCountedPtr<LogSink>;

  /** Creates the sink and initializes the reference count to one. */
  LogSink();

  // Disable copy and assignment.
  LogSink(const LogSink&) = delete;
  LogSink& operator=(const LogSink&) = delete;

  /**
   * Does whatever is necessary to open the sink.
   * The default implementation does nothing.
   */
  virtual void Open();

  /**
   * Does whatever is necessary to close the sink.
   * The default implementation does nothing.
   */
  virtual void Close();

  /**
   * Logs the given message to the sink. Must be overridden by subclasses.
   *
   * If the sink has not been opened yet, the Log() method will open it.
   */
  virtual void Log(const LogMessage& msg) = 0;

  /** Throws a PropertyNotSupportedException. */
  void SetProperty(const String& name, const String& value) override;

  /** Throws a PropertyNotSupportedException. */
  String GetProperty(const String& name) const override;

 protected:
  virtual ~LogSink();
};

}  // namespace fun
