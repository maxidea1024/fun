#include "fun/base/timespan.h"
#include "fun/base/math/math_base.h"
#include "fun/base/string/string.h"
#include "fun/base/serialization/archive.h"
#include "fun/base/container/array.h"

namespace fun {

const int64 Timespan::MaxSeconds = int64_MAX / 1000;
const int64 Timespan::MinSeconds = int64_MIN / 1000;
const int64 Timespan::MaxMilliSeconds = int64_MAX / 1000;
const int64 Timespan::MinMilliSeconds = int64_MIN / 1000;

const Timespan Timespan::MinValue(int64_MIN);
const Timespan Timespan::MaxValue(int64_MAX);
const Timespan Timespan::Zero(0);

const Timespan Timespan::Infinite(Timespan::MaxValue);

const int64 Timespan::MILLISECONDS = DateTimeConstants::TICKS_PER_MILLISECOND;
const int64 Timespan::SECONDS = Timespan::MILLISECONDS*1000;
const int64 Timespan::MINUTES = Timespan::SECONDS*60;
const int64 Timespan::HOURS = Timespan::MINUTES*60;
const int64 Timespan::DAYS = Timespan::HOURS*24;

Timespan::Timespan(int64 ticks) : ticks_(ticks) {
  fun_check(ticks_ >= MinValue.ticks_ && ticks_ <= MaxValue.ticks_);
}

Timespan Timespan::FromDays(double days) {
  const int64 ticks = int64(days * DateTimeConstants::TICKS_PER_DAY);
  fun_check(ticks >= MinValue.ticks_ && ticks <= MaxValue.ticks_);
  return Timespan(ticks);
}

Timespan Timespan::FromHours(double hours) {
  const int64 ticks = int64(hours * DateTimeConstants::TICKS_PER_HOUR);
  fun_check(ticks >= MinValue.ticks_ && ticks <= MaxValue.ticks_);
  return Timespan(ticks);
}

Timespan Timespan::FromMinutes(double minutes) {
  const int64 ticks = int64(minutes * DateTimeConstants::TICKS_PER_MINUTE);
  fun_check(ticks >= MinValue.ticks_ && ticks <= MaxValue.ticks_);
  return Timespan(ticks);
}

Timespan Timespan::FromSeconds(double seconds) {
  const int64 ticks = int64(seconds * DateTimeConstants::TICKS_PER_SECOND);
  fun_check(ticks >= MinValue.ticks_ && ticks <= MaxValue.ticks_);
  return Timespan(ticks);
}

Timespan Timespan::FromMilliseconds(double milliseconds) {
  const int64 ticks = int64(milliseconds * DateTimeConstants::TICKS_PER_MILLISECOND);
  fun_check(ticks >= MinValue.ticks_ && ticks <= MaxValue.ticks_);
  return Timespan(ticks);
}

Timespan Timespan::FromMicroseconds(double microseconds) {
  const int64 ticks = int64(microseconds * DateTimeConstants::TICKS_PER_MICROSECOND);
  fun_check(ticks >= MinValue.ticks_ && ticks <= MaxValue.ticks_);
  return Timespan(ticks);
}

String Timespan::ToString() const {
  const int32 dd = MathBase::Abs(Days());
  const int32 hh = MathBase::Abs(Hours());
  const int32 mm = MathBase::Abs(Minutes());
  const int32 ss = MathBase::Abs(Seconds());
  const int32 ms = MathBase::Abs(Milliseconds());
  const int32 us = MathBase::Abs(Microseconds());

  const String sign = (ticks_ < 0) ? "-" : "";
  const String value = (dd == 0) ?
    String::Format("%d:%02d:%02d.%03d.%06d", hh, mm, ss, ms, us) :
    String::Format("%d.%02d:%02d:%02d.%03d.%06d", dd, hh, mm, ss, ms, us);
  return sign + value;
}

Timespan Timespan::Parse(const String& str) {
  Timespan result;
  if (TryParse(str, result)) {
    //throw error?
  }
  return result;
}

bool Timespan::TryParse(const String& str, Timespan& out_timespan) {
  String str2 = str;
  str2.Replace(".", ":");

  const bool is_negative = str2.StartsWith("-");

  str2.Replace("-", ":");

  auto tokens = str2.Split(":", 0, StringSplitOption::CullEmpty);

  if (tokens.Count() == 5) {
    tokens.Insert(String("0"), 0);
  }

  if (tokens.Count() == 6) {
    const int32 dd = CStringTraitsA::Atoi(*tokens[0]);
    const int32 hh = CStringTraitsA::Atoi(*tokens[1]);
    const int32 mm = CStringTraitsA::Atoi(*tokens[2]);
    const int32 ss = CStringTraitsA::Atoi(*tokens[3]);
    const int32 ms = CStringTraitsA::Atoi(*tokens[4]);
    const int32 us = CStringTraitsA::Atoi(*tokens[5]);

    if (hh < 0 || hh > 23) {
      return false;
    }

    if (mm < 0 || mm > 59) {
      return false;
    }

    if (ss < 0 || ss > 59) {
      return false;
    }

    if (ms < 0 || ms > 999) {
      return false;
    }

    if (us < 0 || us > 999999) {
      return false;
    }

    if (!out_timespan.TryAssign(dd, hh, mm, ss, ms, us)) {
      return false;
    }

    if (is_negative) {
      out_timespan.ticks_ = -out_timespan.ticks_;
    }

    return true;
  }

  return false;
}

bool Timespan::TryAssign(int32 days, int32 hours, int32 minutes, int32 seconds, int32 milliseconds, int32 microseconds) {
  const int64 ticks =
          days * DateTimeConstants::TICKS_PER_DAY +
          hours * DateTimeConstants::TICKS_PER_HOUR +
          minutes * DateTimeConstants::TICKS_PER_MINUTE +
          seconds * DateTimeConstants::TICKS_PER_SECOND +
          milliseconds * DateTimeConstants::TICKS_PER_MILLISECOND +
          microseconds * DateTimeConstants::TICKS_PER_MICROSECOND;

  if (ticks < MinValue.ticks_ || ticks > MaxValue.ticks_) {
    return false;
  }

  ticks_ = ticks;
  return true;
}

void Timespan::Assign(int32 days, int32 hours, int32 minutes, int32 seconds, int32 milliseconds, int32 microseconds) {
  if (!TryAssign(days, hours, minutes, seconds, milliseconds, microseconds)) {
    //throw error?
  }
}

uint32 HashOf(const Timespan& timespan) {
  return HashOf(timespan.ticks_);
}

Archive& operator & (Archive& ar, Timespan& timespan) {
  return ar & timespan.ticks_;
}

} // namespace fun
