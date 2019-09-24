#include "fun/base/logging/file_sink.h"

#if FUN_WITH_FILE_SINK

#include "fun/base/logging/archive_strategy.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/logging/purge_strategy.h"
#include "fun/base/logging/rotate_strategy.h"
#include "fun/base/str.h"
//#include "fun/base/number_parser.h"
//#include "fun/base/datetime_formatter.h"
#include "fun/base/date_time.h"
//#include "fun/base/local_datetime.h"
#include "fun/base/exception.h"

namespace fun {

const String FileSink::PROP_PATH = "Path";
const String FileSink::PROP_ROTATION = "Rotation";
const String FileSink::PROP_ARCHIVE = "Archive";
const String FileSink::PROP_TIMES = "Times";
const String FileSink::PROP_COMPRESS = "Compress";
const String FileSink::PROP_PURGE_AGE = "PurgeAge";
const String FileSink::PROP_PURGE_COUNT = "PurgeCount";
const String FileSink::PROP_FLUSH = "Flush";
const String FileSink::PROP_ROTATE_ON_OPEN = "RotateOnOpen";

FileSink::FileSink()
    : times_("utc"),
      compress_(false),
      flush_(true),
      rotate_on_open_(false),
      log_file_(nullptr),
      rotate_strategy_(nullptr),
      archive_strategy_(new ArchiveByNumberStrategy),
      purge_strategy_(nullptr) {}

FileSink::FileSink(const String& path)
    : path_(path),
      times_("utc"),
      compress_(false),
      flush_(true),
      rotate_on_open_(false),
      log_file_(nullptr),
      rotate_strategy_(nullptr),
      archive_strategy_(new ArchiveByNumberStrategy),
      purge_strategy_(nullptr) {}

FileSink::~FileSink() {
  try {
    Close();

    delete rotate_strategy_;
    delete archive_strategy_;
    delete purge_strategy_;
  } catch (...) {
    fun_unexpected();
  }
}

void FileSink::Open() {
  ScopedLock<FastMutex> guard(mutex_);

  if (!log_file_) {
    log_file_ = new LogFile(path_);

    if (rotate_on_open_ && log_file_->GetSize() > 0) {
      try {
        log_file_ = archive_strategy_->Archive(log_file_);
        Purge();
      } catch (...) {
        log_file_ = new LogFile(path_);
      }
    }
  }
}

void FileSink::Close() {
  ScopedLock<FastMutex> guard(mutex_);

  delete log_file_;
  log_file_ = nullptr;
}

void FileSink::Log(const LogMessage& msg) {
  Open();

  ScopedLock<FastMutex> guard(mutex_);

  if (rotate_strategy_ && archive_strategy_ &&
      rotate_strategy_->MustRotate(log_file_)) {
    try {
      log_file_ = archive_strategy_->Archive(log_file_);
      Purge();
    } catch (...) {
      log_file_ = new LogFile(path_);
    }

    // we must call MustRotate() again to give the
    // RotateByIntervalStrategy a chance to write its timestamp
    // to the new file.
    rotate_strategy_->MustRotate(log_file_);
  }

  log_file_->Write(msg.GetText(), flush_);
}

void FileSink::SetProperty(const String& name, const String& value) {
  ScopedLock<FastMutex> guard(mutex_);

  if (icompare(name, PROP_TIMES) == 0) {
    times_ = value;

    if (!rotation_.IsEmpty()) {
      SetRotation(rotation_);
    }

    if (!archive_.IsEmpty()) {
      SetArchive(archive_);
    }
  } else if (icompare(name, PROP_PATH) == 0) {
    path_ = value;
  } else if (icompare(name, PROP_ROTATION) == 0) {
    SetRotation(value);
  } else if (icompare(name, PROP_ARCHIVE) == 0) {
    SetArchive(value);
  } else if (icompare(name, PROP_COMPRESS) == 0) {
    SetCompress(value);
  } else if (icompare(name, PROP_PURGE_AGE) == 0) {
    SetPurgeAge(value);
  } else if (icompare(name, PROP_PURGE_COUNT) == 0) {
    SetPurgeCount(value);
  } else if (icompare(name, PROP_FLUSH) == 0) {
    SetFlush(value);
  } else if (icompare(name, PROP_ROTATE_ON_OPEN) == 0) {
    SetRotateOnOpen(value);
  } else {
    LogSink::SetProperty(name, value);
  }
}

String FileSink::GetProperty(const String& name) const {
  if (icompare(name, PROP_TIMES) == 0) {
    return times_;
  } else if (icompare(name, PROP_PATH) == 0) {
    return path_;
  } else if (icompare(name, PROP_ROTATION) == 0) {
    return rotation_;
  } else if (icompare(name, PROP_ARCHIVE) == 0) {
    return archive_;
  } else if (icompare(name, PROP_COMPRESS) == 0) {
    return String(compress_ ? "True" : "False");
  } else if (icompare(name, PROP_PURGE_AGE) == 0) {
    return purge_age_;
  } else if (icompare(name, PROP_PURGE_COUNT) == 0) {
    return purge_count_;
  } else if (icompare(name, PROP_FLUSH) == 0) {
    return String(flush_ ? "True" : "False");
  } else if (icompare(name, PROP_ROTATE_ON_OPEN) == 0) {
    return String(rotate_on_open_ ? "True" : "False");
  } else {
    return LogSink::GetProperty(name);
  }
}

Timestamp FileSink::GetCreationDate() const {
  if (log_file_) {
    return log_file_->GetCreationDate();
  } else {
    return 0;
  }
}

uint64 FileSink::GetSize() const {
  if (log_file_) {
    return log_file_->GetSize();
  } else {
    return 0;
  }
}

const String& FileSink::GetPath() const { return path_; }

void FileSink::SetRotation(const String& rotation) {
  String::const_iterator it = rotation.begin();
  String::const_iterator end = rotation.end();
  int n = 0;
  while (it != end && CharTraitsA::IsWhitespace(*it)) ++it;
  while (it != end && CharTraitsA::IsDigit(*it)) {
    n *= 10;
    n += *it++ - '0';
  }
  while (it != end && CharTraitsA::IsWhitespace(*it)) ++it;
  String unit;
  while (it != end && CharTraitsA::IsAlpha(*it)) unit += *it++;

  RotateStrategy* strategy = nullptr;
  if ((rotation.find(',') != String::npos) ||
      (rotation.find(':') != String::npos)) {
    if (icompare(times_, "UTC") == 0) {
      strategy = new RotateAtTimeStrategy<DateTime>(rotation);
    } else if (icompare(times_, "Local") == 0) {
      strategy = new RotateAtTimeStrategy<LocalDateTime>(rotation);
    } else {
      throw PropertyNotSupportedException("Times", times_);
    }
  } else if (icompare(unit, "Daily") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(1 * Timespan::DAYS));
  } else if (icompare(unit, "Weekly") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(7 * Timespan::DAYS));
  } else if (icompare(unit, "Monthly") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(30 * Timespan::DAYS));
  } else if (icompare(unit, "Seconds") == 0) {  // for testing only
    strategy = new RotateByIntervalStrategy(Timespan(n * Timespan::SECONDS));
  } else if (icompare(unit, "Minutes") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(n * Timespan::MINUTES));
  } else if (icompare(unit, "Hours") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(n * Timespan::HOURS));
  } else if (icompare(unit, "Days") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(n * Timespan::DAYS));
  } else if (icompare(unit, "Weeks") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(n * 7 * Timespan::DAYS));
  } else if (icompare(unit, "Months") == 0) {
    strategy = new RotateByIntervalStrategy(Timespan(n * 30 * Timespan::DAYS));
  } else if (icompare(unit, "K") == 0) {
    strategy = new RotateBySizeStrategy(n * 1024);
  } else if (icompare(unit, "M") == 0) {
    strategy = new RotateBySizeStrategy(n * 1024 * 1024);
  } else if (unit.IsEmpty()) {
    strategy = new RotateBySizeStrategy(n);
  } else if (icompare(unit, "Never") != 0) {
    throw InvalidArgumentException("Rotation", rotation);
  }

  delete rotate_strategy_;
  rotate_strategy_ = strategy;
  rotation_ = rotation;
}

void FileSink::SetArchive(const String& archive) {
  ArchiveStrategy* strategy = nullptr;
  if (icompare(archive, "Number") == 0) {
    strategy = new ArchiveByNumberStrategy;
  } else if (icompare(archive, "Timestamp") == 0) {
    if (icompare(times_, "UTC") == 0) {
      strategy = new ArchiveByTimestampStrategy<DateTime>;
    } else if (icompare(times_, "Local") == 0) {
      strategy = new ArchiveByTimestampStrategy<LocalDateTime>;
    } else {
      throw PropertyNotSupportedException("Times", times_);
    }
  } else {
    throw InvalidArgumentException("Archive", archive);
  }

  delete archive_strategy_;
  strategy->Compress(compress_);
  archive_strategy_ = strategy;
  archive_ = archive;
}

void FileSink::SetCompress(const String& compress) {
  compress_ = icompare(compress, "True") == 0;
  if (archive_strategy_) {
    archive_strategy_->Compress(compress_);
  }
}

void FileSink::SetPurgeAge(const String& age) {
  if (SetNoPurge(age)) {
    return;
  }

  String::const_iterator next_to_digit;
  int num = ExtractDigit(age, &next_to_digit);
  Timespan::TimeDiff factor = ExtractFactor(age, next_to_digit);

  SetPurgeStrategy(new PurgeByAgeStrategy(Timespan(num * factor)));
  purge_age_ = age;
}

void FileSink::SetPurgeCount(const String& count) {
  if (SetNoPurge(count)) {
    return;
  }

  SetPurgeStrategy(new PurgeByCountStrategy(ExtractDigit(count)));
  purge_count_ = count;
}

void FileSink::SetFlush(const String& flush) {
  flush_ = icompare(flush, "True") == 0;
}

void FileSink::SetRotateOnOpen(const String& rotate_on_open) {
  rotate_on_open_ = icompare(rotate_on_open, "True") == 0;
}

void FileSink::Purge() {
  if (purge_strategy_) {
    try {
      purge_strategy_->Purge(path_);
    } catch (...) {
    }
  }
}

bool FileSink::SetNoPurge(const String& value) {
  if (value.IsEmpty() || 0 == icompare(value, "None")) {
    delete purge_strategy_;
    purge_strategy_ = 0;
    purge_age_ = "None";
    return true;
  } else {
    return false;
  }
}

int FileSink::ExtractDigit(const String& value,
                           String::const_iterator* next_to_digit) const {
  String::const_iterator it = value.begin();
  String::const_iterator end = value.end();
  int digit = 0;

  while (it != end && CharTraitsA::IsWhitespace(*it)) ++it;
  while (it != end && CharTraitsA::IsDigit(*it)) {
    digit *= 10;
    digit += *it++ - '0';
  }

  if (digit == 0) {
    throw InvalidArgumentException("zero is not valid purge age.");
  }

  if (next_to_digit) {
    *next_to_digit = it;
  }
  return digit;
}

void FileSink::SetPurgeStrategy(PurgeStrategy* strategy) {
  delete purge_strategy_;
  purge_strategy_ = strategy;
}

Timespan::TimeDiff FileSink::ExtractFactor(const String& value,
                                           String::const_iterator start) const {
  while (start != value.end() && CharTraitsA::IsWhitespace(*start)) {
    ++start;
  }

  String unit;
  while (start != value.end() && CharTraitsA::IsAlpha(*start)) {
    unit += *start++;
  }

  if (icompare(unit, "Seconds") == 0) {
    return Timespan::SECONDS;
  } else if (icompare(unit, "Minutes") == 0) {
    return Timespan::MINUTES;
  } else if (icompare(unit, "Hours") == 0) {
    return Timespan::HOURS;
  } else if (icompare(unit, "Days") == 0) {
    return Timespan::DAYS;
  } else if (icompare(unit, "Weeks") == 0) {
    return 7 * Timespan::DAYS;
  } else if (icompare(unit, "Months") == 0) {
    return 30 * Timespan::DAYS;
  } else {
    throw InvalidArgumentException("PurgeAge", value);
  }

  return Timespan::TimeDiff();
}

}  // namespace fun

#endif  // FUN_WITH_FILE_SINK
