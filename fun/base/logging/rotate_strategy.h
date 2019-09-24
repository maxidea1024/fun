//TODO Local/UTC 구분을 지어질수 있도록 해야할까?
#pragma once

#include "fun/base/base.h"
#include "fun/base/timespan.h"
#include "fun/base/timestamp.h"
#include "fun/base/exception.h"
#include "fun/base/logging/log_file.h"
#include "fun/base/string/string.h"

namespace fun {

/**
 * The RotateStrategy is used by LogFile to determine when
 * a file must be rotated.
 */
class FUN_BASE_API RotateStrategy {
 public:
  RotateStrategy();
  virtual ~RotateStrategy();

  /**
   * Returns true if the given log file must
   * be rotated, false otherwise.
   */
  virtual bool MustRotate(LogFile* file) = 0;

  FUN_DISALLOW_COPY_AND_ASSIGNMENT(RotateStrategy);
};

/**
 * The file is rotated at specified [day,][hour]:minute
 */
template <typename DT>
class RotateAtTimeStrategy : public RotateStrategy {
 public:
  RotateAtTimeStrategy(const String& rtime)
    : day_(-1), hour_(-1), minute_(0) {
    if (rtime.IsEmpty()) {
      throw InvalidArgumentException("rotation time must be specified.");
    }

    if ((rtime.find(',') != rtime.npos) && (rtime.find(':') == rtime.npos)) {
      throw InvalidArgumentException("invalid rotation time specified.");
    }

    Array<String> timestr = rtime.Split(",:", 0, StringSplitOption::TrimmingAndCullEmpty);
    int32 index = 0;

    switch (timestr.Count()) {
      case 3: { // day,hh:mm
        String::const_iterator it = timestr[index].begin();
        day_ = DateTimeParser::ParseDayOfWeek(it, timestr[index].end());
        ++index;
      }
      case 2: // hh:mm
        hour_ = NumberParser::parse(timestr[index]);
        ++index;
      case 1: // mm
        minute_ = NumberParser::parse(timestr[index]);
        break;
      default:
        throw InvalidArgumentException("invalid rotation time specified.");
    }

    GetNextRollover();
  }

  ~RotateAtTimeStrategy() {}

  bool MustRotate(LogFile* file) override {
    FUN_UNUSED(file);

    if (DT() >= threshold_) {
      GetNextRollover();
      return true;
    }

    return false;
  }

 private:
  void GetNextRollover() {
    Timespan tsp(0, 0, 1, 0, 1000); // 0,00:01:00.001
    do {
      threshold_ += tsp;
    } while (!(threshold_.Minute() == minute_ &&
              (-1 == hour_ || threshold_.Hour() == hour_) &&
              (-1 == day_  || threshold_.DayOfWeek() == day_)));

    // round to :00.0 seconds
    threshold_.Assign(threshold_.Year(),
                      threshold_.Month(),
                      threshold_.Day(),
                      threshold_.Hour(),
                      threshold_.Minute());
  }

  DT threshold_;
  int32 day_;
  int32 hour_;
  int32 minute_;
};

/**
 * The file is rotated when the log file
 * exceeds a given age.
 *
 * For this to work reliably across all platforms and file systems
 * (there are severe issues on most platforms finding out the real
 * creation date of a file), the creation date of the file is
 * written into the log file as the first entry.
 */
class FUN_BASE_API RotateByIntervalStrategy : public RotateStrategy {
 public:
  RotateByIntervalStrategy(const Timespan& span);
  ~RotateByIntervalStrategy();

  bool MustRotate(LogFile* file) override;

 private:
  Timespan span_;
  Timestamp last_rotated_at_;

  static const String ROTATE_TEXT;
};

/**
 * The file is rotated when the log file
 * exceeds a given size.
 */
class FUN_BASE_API RotateBySizeStrategy : public RotateStrategy {
 public:
  RotateBySizeStrategy(uint64 size);
  ~RotateBySizeStrategy();

  bool MustRotate(LogFile* file) override;

 private:
  uint64 size_;
};

} // namespace fun
