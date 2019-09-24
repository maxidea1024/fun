#include "fun/base/logging/async_sink.h"
#include "fun/base/exception.h"
#include "fun/base/logging/log_formatter.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/logging/logging_registry.h"
#include "fun/base/notification.h"
#include "fun/base/ref_counted_ptr.h"
#include "fun/base/str.h"

namespace fun {

namespace {

class MessageNotification : public Notification {
 public:
  MessageNotification(const LogMessage& msg) : msg_(msg) {}
  ~MessageNotification() {}

  const LogMessage& GetMessage() const { return msg_; }

 private:
  LogMessage msg_;
};

}  // namespace

//
// AsyncSink
//

AsyncSink::AsyncSink(LogSink::Ptr sink, Thread::Priority prio)
    : sink_(sink), thread_("AsyncSink") {
  thread_.SetPriority(prio);
}

AsyncSink::~AsyncSink() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

void AsyncSink::SetSink(LogSink::Ptr sink) { sink_ = sink; }

LogSink::Ptr AsyncSink::GetSink() const { return sink_; }

void AsyncSink::Open() {
  ScopedLock<FastMutex> guard(thread_mutex_);

  if (!thread_.IsRunning()) {
    thread_.Start(*this);
  }
}

void AsyncSink::Close() {
  if (thread_.IsRunning()) {
    // If there are messages that have not been processed yet, allow them to
    // clear and exit.
    while (!queue_.IsEmpty()) {
      Thread::Sleep(100);
    }

    // wake the thread briefly so that it can be terminated.
    do {
      queue_.WakeUpAll();
    } while (!thread_.TryJoin(100));
  }
}

void AsyncSink::Log(const LogMessage& msg) {
  Open();

  queue_.Enqueue(new MessageNotification(msg));
}

void AsyncSink::SetProperty(const String& name, const String& value) {
  if (icompare(name, "Sink") == 0) {
    SetSink(LoggingRegistry::DefaultRegistry().SinkForName(value));
  } else if (icompare(name, "Priority") == 0) {
    SetPriority(value);
  } else {
    LogSink::SetProperty(name, value);
  }
}

void AsyncSink::Run() {
  Notification::Ptr noti = queue_.WaitDequeue();
  while (noti) {
    // TODO In fact, we don't need dynamic_cast??
    MessageNotification* mn = dynamic_cast<MessageNotification*>(noti.Get());
    {
      ScopedLock<FastMutex> guard(sink_mutex_);

      if (mn && sink_) {
        sink_->Log(mn->GetMessage());
      }
    }

    noti = queue_.WaitDequeue();
  }
}

void AsyncSink::SetPriority(const String& value) {
  Thread::Priority prio = Thread::PRIO_NORMAL;

  if (icompare(value, "Lowest") == 0) {
    prio = Thread::PRIO_LOWEST;
  } else if (icompare(value, "Low") == 0) {
    prio = Thread::PRIO_LOW;
  } else if (icompare(value, "Normal") == 0) {
    prio = Thread::PRIO_NORMAL;
  } else if (icompare(value, "High") == 0) {
    prio = Thread::PRIO_HIGH;
  } else if (icompare(value, "Highest") == 0) {
    prio = Thread::PRIO_HIGHEST;
  } else {
    throw InvalidArgumentException("thread priority", value);
  }

  thread_.SetPriority(prio);
}

}  // namespace fun
