#include "fun/sql/date.h"
#include "fun/base/date_time.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/number_formatter.h"
#include "fun/sql/dynamic_date_time.h"

using fun::DateTime;
using fun::NumberFormatter;
using fun::dynamic::Var;

namespace fun {
namespace sql {

Date::Date() {
  DateTime dt;
  Assign(dt.Year(), dt.Month(), dt.Day());
}

Date::Date(int year, int month, int day) { Assign(year, month, day); }

Date::Date(const DateTime& dt) { Assign(dt.Year(), dt.Month(), dt.Day()); }

Date::~Date() {}

void Date::Assign(int year, int month, int day) {
  if (year < 0 || year > 9999) {
    throw InvalidArgumentException("Year must be between 0 and 9999");
  }

  if (month < 1 || month > 12) {
    throw InvalidArgumentException("Month must be between 1 and 12");
  }

  if (day < 1 || day > DateTime::DaysOfMonth(year, month)) {
    throw InvalidArgumentException(
        "Month must be between 1 and " +
        NumberFormatter::Format(DateTime::DaysOfMonth(year, month)));
  }

  year_ = year;
  month_ = month;
  day_ = day;
}

bool Date::operator<(const Date& date) const {
  int year = date.Year();

  if (year_ < year)
    return true;
  else if (year_ > year)
    return false;
  else  // years equal
  {
    int month = date.Month();
    if (month_ < month)
      return true;
    else if (month_ > month)
      return false;
    else  // months equal
        if (day_ < date.Day())
      return true;
  }

  return false;
}

Date& Date::operator=(const Var& var) {
#ifndef __GNUC__
  // g++ used to choke on this, newer versions seem to digest it fine
  // TODO: determine the version able to handle it properly
  *this = var.Extract<Date>();
#else
  *this = var.operator Date();
#endif
  return *this;
}

}  // namespace sql
}  // namespace fun

#ifdef __GNUC__
// only needed for g++ (see comment in Date::operator = above)

namespace fun {
namespace Dynamic {

using fun::DateTime;
using fun::sql::Date;

template <>
Var::operator Date() const {
  VarHolder* holder = GetContent();

  if (!holder) {
    throw InvalidAccessException("Can not convert empty value.");
  }

  if (typeid(Date) == holder->Type()) {
    return Extract<Date>();
  } else {
    fun::DateTime result;
    holder->Convert(result);
    return Date(result);
  }
}

}  // namespace Dynamic
}  // namespace fun

#endif  // __GNUC__
