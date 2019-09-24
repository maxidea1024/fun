#pragma once

#include "fun/base/base.h"

namespace fun {

enum class TimeSpec {
  Local = 0,
  UTC = 1,
  OffsetFromUTC = 2,
  TimeZone = 3,
};

/** Enumerates the days of the week in 7-day calendars. */
enum class DayOfWeekType {
  Sunday = 0,
  Monday = 1,
  Tuesday = 2,
  Wednesday = 3,
  Thursday = 4,
  Friday = 5,
  Saturday = 6,
};

FUN_BASE_API Archive& operator & (Archive& ar, DayOfWeekType& value);

/** Enumerates the months of the year in 12-month calendars. */
enum class MonthOfYearType {
  January = 1,
  February,
  March,
  April,
  May,
  June,
  July,
  August,
  September,
  October,
  November,
  December
};

FUN_BASE_API Archive& operator & (Archive& ar, MonthOfYearType& value);

enum class DateFormatType {
  // default
  TextDate,
  // ISO 8601
  ISODate,
  SystemLocaleShortDate,
  SystemLcaleLongDate,
  DefaultLocaleShortDate,
  DefaultLocaleLongDate,
  // RFC 2822 (+ 850 and 1036 during parsing)
  RFC2822Date,
  ISODateWithMs
};

class Date;
class Time;
class DateTime;
class Timespan;

namespace DateTimeConstants {

static const int64 TICKS_PER_MICROSECOND = 10LL;
static const int64 TICKS_PER_MILLISECOND = 10000LL;
static const int64 TICKS_PER_SECOND = 10000000LL;
static const int64 TICKS_PER_MINUTE = 600000000LL;
static const int64 TICKS_PER_HOUR = 36000000000LL;
static const int64 TICKS_PER_DAY = 864000000000LL;
static const int64 TICKS_PER_WEEK = 6048000000000LL;

// since 1970-1-1 00:00:00 (unix epoch)
static const int64 EPOCH_TICKS = 621355968000000000LL;

// int maximum 2037-12-31T23:59:59 UTC
static const int64 TIME_T_MAX = 2145916799;

} // namespace DateTimeConstants

} // namespace fun
