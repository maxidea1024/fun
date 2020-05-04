#pragma once

#include <ostream>

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * A sink that writes to an ostream.
 *
 * Only the message's text is written, followed by a newline.
 *
 * Chain this sink to a FormattingSink with an
 * appropriate LogFormatter to control what is contained
 * in the text.
 */
class FUN_BASE_API StreamSink : public LogSink {
 public:
  using Ptr = RefCountedPtr<StreamSink>;

  /**
   * Creates the sink.
   */
  StreamSink(std::ostream& str);

  // LogSink interface
  void Log(const LogMessage& msg) override;

 protected:
  virtual ~StreamSink();

 private:
  std::ostream& str_;
  FastMutex mutex_;
};

}  // namespace fun
