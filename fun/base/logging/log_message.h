#pragma once

#include "fun/base/base.h"
#include "fun/base/container/map.h"
#include "fun/base/logging/log_level.h"
#include "fun/base/string/string.h"
#include "fun/base/timestamp.h"

namespace fun {

// TODO __FUNCTION__도 지원하는건 어떨런지??

/**
 * This class represents a log message that is sent through a
 * chain of log sinks.
 *
 * A LogMessage contains a level denoting the severity of the
 * message, a source describing its origin, a text describing
 * its meaning, the time of its creation, and an identifier of
 * the process and thread that created the message.
 *
 * Optionally a LogMessage can also contain the source file path
 * and line number of the statement generating the message.
 *
 * A LogMessage can also contain any number of named parameters
 * that contain additional information about the event that
 * caused the message.
 */
class FUN_BASE_API LogMessage {
 public:
  /**
   * Creates an empty LogMessage.
   * The thread and process ids are set.
   */
  LogMessage();

  /**
   * Creates a LogMessage with the given source, text and level.
   * The thread and process ids are set.
   */
  LogMessage(const String& source, const String& text, LogLevel::Type level);

  /**
   * Creates a LogMessage with the given source, text, level,
   * source file path and line.
   *
   * The source file path must be a
   * static string with a lifetime that's at least the lifetime
   * of the message object (the string is not copied internally).
   * Usually, this will be the path string obtained from the
   * __FILE__ macro.
   *
   * The thread and process ids are set.
   */
  LogMessage(const String& source, const String& text, LogLevel::Type level,
             const char* file, int line);

  /**
   * Creates a LogMessage by copying another one.
   */
  LogMessage(const LogMessage& msg);

  /**
   * Creates a LogMessage by copying another one.
   */
  LogMessage(LogMessage&& msg);

  /**
   * Creates a LogMessage by copying all but the text from another message.
   */
  LogMessage(const LogMessage& msg, const String& text);

  /**
   * Destroys the LogMessage.
   */
  ~LogMessage();

  /**
   * Assignment operator.
   */
  LogMessage& operator=(const LogMessage& other);

  /**
   * Assignment operator.
   */
  LogMessage& operator=(LogMessage&& other);

  /**
   * Swaps the message with another one.
   */
  void Swap(LogMessage& other);

  /**
   * Sets the source of the message.
   */
  void SetSource(const String& source);

  /**
   * Returns the source of the message.
   */
  const String& GetSource() const;

  /**
   * Sets the text of the message.
   */
  void SetText(const String& text);

  /**
   * Returns the text of the message.
   */
  const String& GetText() const;

  /**
   * Sets the level of the message.
   */
  void SetLevel(LogLevel::Type level);

  /**
   * Returns the level of the message.
   */
  LogLevel::Type GetLevel() const;

  /**
   * Sets the time of the message.
   */
  void SetTime(const Timestamp& time);

  /**
   * Returns the time of the message.
   */
  const Timestamp& GetTime() const;

  /**
   * Sets the thread identifier for the message.
   */
  void SetThread(const String& thread);

  /**
   * Returns the thread identifier for the message.
   */
  const String& GetThread() const;

  /**
   * Sets the numeric thread identifier for the message.
   */
  void SetTid(long pid);

  /**
   * Returns the numeric thread identifier for the message.
   */
  long GetTid() const;

  /**
   * Returns the numeric OS thread identifier for the message.
   */
  intptr_t GetOsTid() const;

  /**
   * Sets the process identifier for the message.
   */
  void SetPid(long pid);

  /**
   * Returns the process identifier for the message.
   */
  long GetPid() const;

  /**
   * Sets the source file path of the statement
   * generating the log message.
   *
   * File must be a static string, such as the value of
   * the __FILE__ macro. The string is not copied
   * internally for performance reasons.
   */
  void SetSourceFile(const char* file);

  /**
   * Returns the source file path of the code creating
   * the message. May be 0 if not set.
   */
  const char* GetSourceFile() const;

  /**
   * Sets the source file line of the statement
   * generating the log message.
   *
   * This is usually the result of the __LINE__
   * macro.
   */
  void SetSourceLine(int line);

  /**
   * Returns the source file line of the statement
   * generating the log message. May be 0
   * if not set.
   */
  int GetSourceLine() const;

  /**
   * Returns true if a parameter with the given name exists.
   */
  bool Has(const String& param) const;

  /**
   * Returns a const reference to the value of the parameter
   * with the given name. Throws a NotFoundException if the
   * parameter does not exist.
   */
  const String& Get(const String& param) const;

  /**
   * Returns a const reference to the value of the parameter
   * with the given name. If the parameter with the given name
   * does not exist, then default_value is returned.
   */
  const String& Get(const String& param, const String& default_value) const;

  /**
   * Sets the value for a parameter. If the parameter does
   * not exist, then it is created.
   */
  void Set(const String& param, const String& value);

  /**
   * Returns a const reference to the value of the parameter
   * with the given name. Throws a NotFoundException if the
   * parameter does not exist.
   */
  const String& operator[](const String& param) const;

  /**
   * Returns a reference to the value of the parameter with the
   * given name. This can be used to set the parameter's value.
   * If the parameter does not exist, it is created with an
   * empty string value.
   */
  String& operator[](const String& param);

 protected:
  void Init();
  using StringMap = Map<String, String>;

 private:
  String source_;
  String text_;
  LogLevel::Type level_;
  Timestamp time_;
  long tid_;
  intptr_t os_tid_;
  String thread_;
  long pid_;
  const char* file_;
  int line_;
  StringMap* map_;
};

//
// inlines
//

FUN_ALWAYS_INLINE const String& LogMessage::GetSource() const {
  return source_;
}

FUN_ALWAYS_INLINE const String& LogMessage::GetText() const { return text_; }

FUN_ALWAYS_INLINE LogLevel::Type LogMessage::GetLevel() const { return level_; }

FUN_ALWAYS_INLINE const Timestamp& LogMessage::GetTime() const { return time_; }

FUN_ALWAYS_INLINE const String& LogMessage::GetThread() const {
  return thread_;
}

FUN_ALWAYS_INLINE long LogMessage::GetTid() const { return tid_; }

FUN_ALWAYS_INLINE intptr_t LogMessage::GetOsTid() const { return os_tid_; }

FUN_ALWAYS_INLINE long LogMessage::GetPid() const { return pid_; }

FUN_ALWAYS_INLINE const char* LogMessage::GetSourceFile() const {
  return file_;
}

FUN_ALWAYS_INLINE int LogMessage::GetSourceLine() const { return line_; }

// TODO 제거하는 쪽으로 해보자...
// FUN_ALWAYS_INLINE void Swap(LogMessage& lhs, LogMessage& rhs) {
//  lhs.Swap(rhs);
//}

}  // namespace fun
