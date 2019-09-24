#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/thread.h"
#include "fun/base/mutex.h"
#include "fun/base/runnable.h"
#include "fun/base/ref_counted.h"
#include "fun/base/notification_queue.h"

namespace fun {

/**
 * A sink uses a separate thread for logging.
 *
 * Using this sink can help to improve the performance of
 * applications that produce huge amounts of log messages or
 * that write log messages to multiple sinks simultaneously.
 *
 * All log messages are put into a queue and this queue is
 * then processed by a separate thread.
 */
class FUN_BASE_API AsyncSink : public LogSink, public Runnable {
 public:
  using Ptr = RefCountedPtr<AsyncSink>;

  AsyncSink(LogSink::Ptr sink = nullptr, Thread::Priority prio = Thread::PRIO_NORMAL);

  void SetSink(LogSink::Ptr sink);
  LogSink::Ptr GetSink() const;

  // LogSink interface.
  void Open() override;
  void Close() override;
  void Log(const LogMessage& msg) override;


  //
  // Configurable interface.
  //

  /**
   * Sets or changes a configuration property.
   *
   * The "Sink" property allows setting the target
   * sink via the LoggingRegistry.
   * The "Sink" property is set-only.
   *
   * The "Priority" property allows setting the thread
   * priority. The following values are supported:
   *    - Lowest
   *    - Low
   *    - Normal (Default)
   *    - High
   *    - Highest
   *
   * The "Priority" property is set-only.
   */
  void SetProperty(const String& name, const String& value) override;

 protected:
  ~AsyncSink();

  // Runnable interface.
  void Run() override;

  void SetPriority(const String& value);

 private:
  LogSink::Ptr sink_;
  Thread thread_;
  FastMutex thread_mutex_;
  FastMutex sink_mutex_;
  NotificationQueue queue_;
};

} // namespace fun
