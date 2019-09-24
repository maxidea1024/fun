#include "fun/base/logging/formatting_sink.h"
//#include "fun/base/logging/log_formatter.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/logging/logging_registry.h"
#include "fun/base/str.h"

namespace fun {

//FUN_IMPLEMENT_RTCLASS(FormattingSink,Sink)

FormattingSink::FormattingSink()
  : formatter_(), sink_() {}

FormattingSink::FormattingSink(LogFormatter::Ptr formatter)
  : formatter_(formatter), sink_() {}

FormattingSink::FormattingSink( LogFormatter::Ptr formatter,
                                LogSink::Ptr sink)
  : formatter_(formatter), sink_(sink) {}

void FormattingSink::SetFormatter(LogFormatter::Ptr formatter) {
  formatter_ = formatter;
}

LogFormatter::Ptr FormattingSink::GetFormatter() const {
  return formatter_;
}

void FormattingSink::SetSink(LogSink::Ptr sink) {
  sink_ = sink;
}

LogSink::Ptr FormattingSink::GetSink() const {
  return sink_;
}

void FormattingSink::Log(const LogMessage& msg) {
  if (sink_) {
    if (formatter_) {
      String formatted_text;
      formatter_->Format(msg, formatted_text);
      sink_->Log(LogMessage(msg, formatted_text));
    } else {
      sink_->Log(msg);
    }
  }
}

void FormattingSink::SetProperty(const String& name, const String& value) {
  if (icompare(name, "Sink") == 0) {
    SetSink(LoggingRegistry::DefaultRegistry().SinkForName(value));
  } else if (icompare(name, "Formatter") == 0) {
    SetFormatter(LoggingRegistry::DefaultRegistry().FormatterForName(value));
  } else if (sink_) {
    sink_->SetProperty(name, value);
  }
}

void FormattingSink::Open() {
  if (sink_) {
    sink_->Open();
  }
}

void FormattingSink::Close() {
  if (sink_) {
    sink_->Close();
  }
}

} // namespace fun
