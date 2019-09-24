#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/windows_less.h"
#include "fun/base/string/string.h"

#define FUN_WITH_EVENT_LOG_SINK  0

#if FUN_WITH_EVENT_LOG_SINK

namespace fun {

/**
 * This Windows-only channel works with the Windows NT Event Log
 * service.
 *
 * To work properly, the EventLogSink class requires that either
 * the PocoFoundation.dll or the PocoMsg.dll Dynamic Link Library
 * containing the message definition resources can be found in $PATH.
 */
class FUN_BASE_API EventLogSink : public LogSink {
 public:
  using Ptr = RefCountedPtr<EventLogSink>;

  /**
   * Creates the EventLogSink.
   * The name of the current application (or more correctly,
   * the name of its executable) is taken as event source name.
   */
  EventLogSink();

  /**
   * Creates the EventLogSink with the given event source name.
   */
  EventLogSink(const String& name);

  /**
   * Creates an EventLogSink with the given event source
   * name that routes messages to the given host.
   */
  EventLogSink(const String& name, const String& host);

  /**
   * Opens the EventLogSink. If necessary, the
   * required registry entries to register a
   * message resource DLL are made.
   */
  void Open() override;

  /**
   * Closes the EventLogSink.
   */
  void Close() override;

  /**
   * Logs the given message to the Windows Event Log.
   *
   * The message type and priority are mapped to
   * appropriate values for Event Log type and category.
   */
  void Log(const LogMessage& msg) override;

  /**
   * Sets or changes a configuration property.
   *
   * The following properties are supported:
   *
   *   * Name:    The name of the event source.
   *   * LogHost: The name of the host where the Event Log service is running.
   *              The default is "localhost".
   *   * Host:    same as host.
   *   * LogFile: The name of the log file. The default is "Application".
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Returns the value of the given property.
   */
  String GetProperty(const String& name) const override;

  static const String PROP_NAME;
  static const String PROP_HOST;
  static const String PROP_LOGHOST;
  static const String PROP_LOGFILE;

 protected:
  ~EventLogSink();
  static int32 GetType(const LogMessage& msg);
  static int32 GetCategory(const LogMessage& msg);
  void SetupRegistry() const;
  static UString FindLibrary(const wchar_t* name);

 private:
  String name_;
  String host_;
  String log_file_;
  HANDLE handle_;
};

} // namespace fun

#endif // FUN_WITH_EVENT_LOG_SINK
