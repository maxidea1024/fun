#include "fun/sql/time.h"
#include "fun/base/date_time.h"
#include "fun/base/dynamic/var.h"
#include "fun/sql/dynamic_date_time.h"

using fun::DateTime;
using fun::dynamic::Var;

namespace fun {
namespace sql {

Time::Time() {
  DateTime dt;
  Assign(dt.Hour(), dt.Minute(), dt.Second());
}

Time::Time(int32 hour, int32 minute, int32 second) {
  Assign(hour, minute, second);
}

Time::Time(const DateTime& dt) { Assign(dt.Hour(), dt.Minute(), dt.Second()); }

Time::~Time() {}

void Time::Assign(int32 hour, int32 minute, int32 second) {
  if (hour < 0 || hour > 23) {
    throw InvalidArgumentException("Hour must be between 0 and 23.");
  }

  if (minute < 0 || minute > 59) {
    throw InvalidArgumentException("Minute must be between 0 and 59.");
  }

  if (second < 0 || second > 59) {
    throw InvalidArgumentException("Second must be between 0 and 59.");
  }

  hour_ = hour;
  minute_ = minute;
  second_ = second;
}

bool Time::operator<(const Time& time) const {
  int32 hour = time.Hour();

  if (hour_ < hour)
    return true;
  else if (hour_ > hour)
    return false;
  else  // hours equal
  {
    int32 minute = time.Minute();
    if (minute_ < minute)
      return true;
    else if (minute_ > minute)
      return false;
    else  // minutes equal
        if (second_ < time.Second())
      return true;
  }

  return false;
}

Time& Time::operator=(const Var& var) {
#ifndef __GNUC__
  // g++ used to choke on this, newer versions seem to digest it fine
  // TODO: determine the version able to handle it properly
  *this = var.Extract<Time>();
#else
  *this = var.operator Time();
#endif
  return *this;
}

}  // namespace sql
}  // namespace fun

#ifdef __GNUC__
// only needed for g++ (see comment in Time::operator = above)

namespace fun {
namespace Dynamic {

using fun::DateTime;
using fun::sql::Time;

template <>
Var::operator Time() const {
  VarHolder* holder = GetContent();

  if (!holder) {
    throw InvalidAccessException("Can not convert empty value.");
  }

  if (typeid(Time) == holder->Type()) {
    return Extract<Time>();
  } else {
    fun::DateTime result;
    holder->Convert(result);
    return Time(result);
  }
}

}  // namespace Dynamic
}  // namespace fun

#endif  // __GNUC__
