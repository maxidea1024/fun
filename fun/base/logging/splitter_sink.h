#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/mutex.h"
#include "fun/base/string/string.h"
//#include "fun/base/runtime_class.h"

namespace fun {

/**
 * This sink sends a message to multiple sinks simultanously.
 */
class FUN_BASE_API SplitterSink : public LogSink {
 public:
  /** Creates the SplitterSink. */
  SplitterSink();
  ~SplitterSink();

  /** Attaches a sink, which may not be null. */
  void AddSink(LogSink::Ptr sink);

  /** Detaches a sink. */
  void RemoveSink(LogSink::Ptr sink);

  // LogSink interace.
  void Log(const LogMessage& msg) override;
  void Close() override;

  // Configurable interface.
  void SetProperty(const String& name, const String& value) override;

  /** Returns the number of sinks in the SplitterSink. */
  int32 GetSinkCount() const;

 private:
  Mutex mutex_;
  Array<LogSink::Ptr> sinks_;
};

}  // namespace fun
