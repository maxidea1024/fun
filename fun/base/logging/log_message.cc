#include "fun/base/logging/log_message.h"
#include "fun/base/exception.h"

#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/process.h"
#endif

#include "fun/base/thread.h"

namespace fun {

LogMessage::LogMessage()
  : level_(LogLevel::Fatal),
    tid_(0),
    os_tid_(0),
    pid_(0),
    file_(0),
    line_(0),
    map_(nullptr) {
  Init();
}

LogMessage::LogMessage( const String& source,
                        const String& text,
                        LogLevel::Type level)
  : source_(source),
    text_(text),
    level_(level),
    tid_(0),
    os_tid_(0),
    pid_(0),
    file_(0),
    line_(0),
    map_(nullptr) {
  Init();
}

LogMessage::LogMessage( const String& source,
                        const String& text,
                        LogLevel::Type level,
                        const char* file,
                        int line)
  : source_(source),
    text_(text),
    level_(level),
    tid_(0),
    os_tid_(0),
    pid_(0),
    file_(file),
    line_(line),
    map_(nullptr) {
  Init();
}

LogMessage::LogMessage(const LogMessage& msg)
  : source_(msg.source_),
    text_(msg.text_),
    level_(msg.level_),
    time_(msg.time_),
    tid_(msg.tid_),
    os_tid_(msg.os_tid_),
    thread_(msg.thread_),
    pid_(msg.pid_),
    file_(msg.file_),
    line_(msg.line_) {
  if (msg.map_) {
    map_ = new StringMap(*msg.map_);
  } else {
    map_ = nullptr;
  }
}

LogMessage::LogMessage(LogMessage&& msg)
  : source_(MoveTemp(msg.source_)),
    text_(MoveTemp(msg.text_)),
    level_(MoveTemp(msg.level_)),
    time_(MoveTemp(msg.time_)),
    tid_(MoveTemp(msg.tid_)),
    thread_(MoveTemp(msg.thread_)),
    pid_(MoveTemp(msg.pid_)),
    file_(MoveTemp(msg.file_)),
    line_(MoveTemp(msg.line_)) {
  map_ = msg.map_;
  msg.map_ = nullptr;
}

LogMessage::LogMessage(const LogMessage& msg, const String& text)
  : source_(msg.source_),
    text_(text),
    level_(msg.level_),
    time_(msg.time_),
    tid_(msg.tid_),
    os_tid_(msg.os_tid_),
    thread_(msg.thread_),
    pid_(msg.pid_),
    file_(msg.file_),
    line_(msg.line_) {
  if (msg.map_) {
    map_ = new StringMap(*msg.map_);
  } else {
    map_ = nullptr;
  }
}

LogMessage::~LogMessage() {
  delete map_;
}

void LogMessage::Init() {
#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
  pid_ = Process::CurrentPid();
#endif

  os_tid_ = (intptr_t)Thread::CurrentTid();


  // 만약, 내부 Thread 클래스를 사용하지 않은 스레드일 경우에는
  // 스레드 ID 및 이름을 구할 수 없음.
  // 이때는 os_tid_만 유효하게됨.
  // (위에서 이미 구해 놓았음.)

  Thread* current_thread = Thread::Current();
  if (current_thread) {
    tid_ = current_thread->GetId();
    thread_ = current_thread->GetName();
  }
}

LogMessage& LogMessage::operator = (const LogMessage& other) {
  if (FUN_LIKELY(&other != this)) {
    LogMessage tmp(other);
    Swap(tmp);
  }
  return *this;
}

LogMessage& LogMessage::operator = (LogMessage&& other) {
  if (FUN_LIKELY(&other != this)) {
    source_ = MoveTemp(other.source_);
    text_ = MoveTemp(other.text_);
    level_ = MoveTemp(other.level_);
    time_ = MoveTemp(other.time_);
    tid_ = MoveTemp(other.tid_);
    thread_ = MoveTemp(other.thread_);
    pid_ = MoveTemp(other.pid_);
    file_ = MoveTemp(other.file_);
    line_ = MoveTemp(other.line_);

    delete map_;
    map_ = other.map_;
    other.map_ = nullptr;
  }

  return *this;
}

void LogMessage::Swap(LogMessage& other) {
  fun::Swap(source_, other.source_);
  fun::Swap(text_, other.text_);
  fun::Swap(level_, other.level_);
  fun::Swap(time_, other.time_);
  fun::Swap(tid_, other.tid_);
  fun::Swap(thread_, other.thread_);
  fun::Swap(pid_, other.pid_);
  fun::Swap(file_, other.file_);
  fun::Swap(line_, other.line_);
  fun::Swap(map_, other.map_);
}

void LogMessage::SetSource(const String& source) {
  source_ = source;
}

void LogMessage::SetText(const String& text) {
  text_ = text;
}

void LogMessage::SetLevel(LogLevel::Type level) {
  level_ = level;
}

void LogMessage::SetTime(const Timestamp& time) {
  time_ = time;
}

void LogMessage::SetThread(const String& thread) {
  thread_ = thread;
}

void LogMessage::SetTid(long tid) {
  tid_ = tid;
}

void LogMessage::SetPid(long pid) {
  pid_ = pid;
}

void LogMessage::SetSourceFile(const char* file) {
  file_ = file;
}

void LogMessage::SetSourceLine(int line) {
  line_ = line;
}

bool LogMessage::Has(const String& param) const {
  //return map_ && (map_->find(param) != map_->end());
  return map_ && map_->Contains(param);
}

const String& LogMessage::Get(const String& param) const {
  if (map_) {
    auto* found = map_->Find(param);
    if (found) {
      return *found;
    }
  }

  throw NotFoundException();
}

const String& LogMessage::Get(const String& param,
                              const String& default_value) const {
  if (map_) {
    if (auto* found = map_->Find(param)) {
      return *found;
    }
  }

  return default_value;
}

void LogMessage::Set(const String& param, const String& value) {
  if (!map_) {
    map_ = new StringMap;
  }

  map_->Add(param, value);
}

const String& LogMessage::operator [] (const String& param) const {
  if (map_) {
    return (*map_)[param];
  } else {
    throw NotFoundException();
  }
}

String& LogMessage::operator [] (const String& param) {
  if (!map_) {
    map_ = new StringMap;
  }

  auto* found = map_->Find(param);
  if (!found) {
    return map_->Emplace(param);
  }
  return *found;
}

} // namespace fun
