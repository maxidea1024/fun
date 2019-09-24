#include "fun/base/logging/simple_file_sink.h"
#include "fun/base/exception.h"
#include "fun/base/file.h"
#include "fun/base/logging/log_file.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/str.h"
#include "fun/base/string/string.h"

namespace fun {

const String SimpleFileSink::PROP_PATH = "Path";
const String SimpleFileSink::PROP_SECONDARYPATH = "SecondaryPath";
const String SimpleFileSink::PROP_ROTATION = "Rotation";
const String SimpleFileSink::PROP_FLUSH = "Flush";

SimpleFileSink::SimpleFileSink() : limit_(0), flush_(true), file_(0) {}

SimpleFileSink::SimpleFileSink(const String& path)
    : path_(path),
      secondard_path_(path + ".0"),
      limit_(0),
      flush_(true),
      file_(0) {}

SimpleFileSink::~SimpleFileSink() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

void SimpleFileSink::Open() {
  FastMutex::ScopedLock lock(mutex_);

  if (!file_) {
    File primary(path_);
    File secondary(secondard_path_);
    Timestamp pt = primary.Exists() ? primary.GetLastModified() : 0;
    Timestamp st = secondary.Exists() ? secondary.GetLastModified() : 0;
    String path_string;
    if (pt >= st) {
      path_string = path_;
    } else {
      path_string = secondard_path_;
    }
    file_ = new LogFile(path_string);
  }
}

void SimpleFileSink::Close() {
  FastMutex::ScopedLock lock(mutex_);

  delete file_;
  file_ = 0;
}

void SimpleFileSink::Log(const LogMessage& msg) {
  Open();

  FastMutex::ScopedLock lock(mutex_);

  if (limit_ > 0 && file_->GetSize() >= limit_) {
    Rotate();
  }

  file_->Write(msg.GetText(), flush_);
}

void SimpleFileSink::SetProperty(const String& name, const String& value) {
  FastMutex::ScopedLock lock(mutex_);

  if (icompare(name, PROP_PATH) == 0) {
    path_ = value;
    if (secondard_path_.IsEmpty()) {
      secondard_path_ = path_ + ".0";
    }
  } else if (icompare(name, PROP_SECONDARYPATH) == 0) {
    secondard_path_ = value;
  } else if (icompare(name, PROP_ROTATION) == 0) {
    SetRotation(value);
  } else if (icompare(name, PROP_FLUSH) == 0) {
    SetFlush(value);
  } else {
    LogSink::SetProperty(name, value);
  }
}

String SimpleFileSink::GetProperty(const String& name) const {
  if (icompare(name, PROP_PATH) == 0) {
    return path_;
  } else if (icompare(name, PROP_SECONDARYPATH) == 0) {
    return secondard_path_;
  } else if (icompare(name, PROP_ROTATION) == 0) {
    return rotation_;
  } else if (icompare(name, PROP_FLUSH) == 0) {
    return String(flush_ ? "True" : "False");
  } else {
    return LogSink::GetProperty(name);
  }
}

Timestamp SimpleFileSink::GetCreationDate() const {
  if (file_) {
    return file_->GetCreationDate();
  } else {
    return 0;
  }
}

uint64 SimpleFileSink::GetSize() const {
  if (file_) {
    return file_->GetSize();
  } else {
    return 0;
  }
}

const String& SimpleFileSink::GetPath() const { return path_; }

const String& SimpleFileSink::GetSecondaryPath() const {
  return secondard_path_;
}

void SimpleFileSink::SetRotation(const String& rotation) {
  String::const_iterator it = rotation.begin();
  String::const_iterator end = rotation.end();
  uint64 n = 0;
  while (it != end && CharTraitsA::IsWhitespace(*it)) ++it;
  while (it != end && CharTraitsA::IsDigit(*it)) {
    n *= 10;
    n += *it++ - '0';
  }
  while (it != end && CharTraitsA::IsWhitespace(*it)) ++it;
  String unit;
  while (it != end && CharTraitsA::IsAlpha(*it)) unit += *it++;

  if (icompare(unit, "K") == 0) {
    limit_ = n * 1024;
  } else if (icompare(unit, "M") == 0) {
    limit_ = n * 1024 * 1024;
  } else if (unit.IsEmpty()) {
    limit_ = n;
  } else if (icompare(unit, "Never") == 0) {
    limit_ = 0;
  } else {
    throw InvalidArgumentException("Rotation", rotation);
  }

  rotation_ = rotation;
}

void SimpleFileSink::SetFlush(const String& flush) {
  flush_ = icompare(flush, "True") == 0;
}

void SimpleFileSink::Rotate() {
  String new_path;
  if (file_->GetPath() == path_) {
    new_path = secondard_path_;
  } else {
    new_path = path_;
  }

  File f(new_path);
  if (f.Exists()) {
    try {
      f.Remove();
    } catch (...) {
    }
  }
  delete file_;
  file_ = new LogFile(new_path);
}

}  // namespace fun
