#pragma once

#include "fun/base/base.h"
#include "fun/base/date_time_types.h"
#include "fun/base/timespan.h"
#include "fun/base/timestamp.h"

//TODO TimeZone의존성이 좀 많아서... 여기서는 전방 선언만 해주어야하는데...
//#include "timezone.h"

namespace fun {

class TimeZone;

class Date {
 public:
  enum MonthNameType {
    DateFormat = 0,
    StandaloneFormat,
  };

  static const Date Null;
  static const Date MinValue;
  static const Date MaxValue;

  static const int32 NullDaysValue;

 private:
  Date(int32 days) : days_(days) {}

 public:
  Date() : days_(NullDaysValue) {}
  Date(int32 year, int32 month, int32 day) { SetDate(year, month, day); }
  Date(const Date& rhs) : days_(rhs.days_) {}
  Date& operator = (const Date& rhs) {
    days_ = rhs.days_;
    return *this;
  }

  void Swap(Date& rhs);

  friend void Swap(Date& a, Date& b) {
    a.Swap(b);
  }

  bool IsNull() const { return !IsValid(); }
  bool IsValid() const { return days_ >= MinValue.days_ && days_ <= MaxValue.days_; }

  int32 ToDays() const { return days_ == NullDaysValue ? 0 : days_; }

  int32 Year() const;
  int32 Month() const;
  int32 Day() const;
  DayOfWeekType DayOfWeek() const;
  int32 DayOfYear() const;
  int32 DaysInMonth() const;
  int32 DaysInYear() const;

  bool SetDate(int32 year, int32 month, int32 day);
  bool GetDate(int32* year, int32* month, int32* day) const;

  Date AddDays(int32 days) const;
  Date AddMonths(int32 months) const;
  Date AddYears(int32 days) const;

  Date& AddDaysInPlace(int32 days);
  Date& AddMonthsInPlace(int32 months);
  Date& AddYearsInPlace(int32 months);

  int32 DaysTo(const Date& to) const;
  FUN_ALWAYS_INLINE static Date FromDays(int32 days) {
    return (days >= MinValue && days <= MaxValue) ? Date(days) : Date();
  }

  double ToJulianDay() const;
  double ToModifiedJulianDay() const;
  static Date FromJulianDay(double julian_day);

  enum class DatePart {
    Year = 0,
    DayOfYear = 1,
    Month = 2,
    Day = 3,
  };

  int32 GetDatePart(DatePart part) const;

  FUN_ALWAYS_INLINE int32 Compare(const Date& other) const {
    return ToDays() == other.ToDays() ? 0 : (ToDays() < other.ToDays() ? -1 : +1);
  }
  FUN_ALWAYS_INLINE friend bool operator == (const Date& lhs, const Date& rhs) {
    return lhs.Compare(rhs) == 0;
  }
  FUN_ALWAYS_INLINE friend bool operator != (const Date& lhs, const Date& rhs) {
    return lhs.Compare(rhs) != 0;
  }
  FUN_ALWAYS_INLINE friend bool operator <  (const Date& lhs, const Date& rhs) {
    return lhs.Compare(rhs) <  0;
  }
  FUN_ALWAYS_INLINE friend bool operator <= (const Date& lhs, const Date& rhs) {
    return lhs.Compare(rhs) <= 0;
  }
  FUN_ALWAYS_INLINE friend bool operator >  (const Date& lhs, const Date& rhs) {
    return lhs.Compare(rhs) >  0;
  }
  FUN_ALWAYS_INLINE friend bool operator >= (const Date& lhs, const Date& rhs) {
    return lhs.Compare(rhs) >= 0;
  }

  static bool IsValid(int32 year, int32 month, int32 day);
  static int32 DaysInMonth(int32 year, int32 month);
  static int32 DaysInYear(int32 year);
  static bool IsLeapYear(int32 year);

  static Date Today();
  static Date UtcToday();

 public:
  static String GetShortMonthName(int32 month, MonthNameType type = DateFormat);
  static String GetShortDayName(DayOfWeekType weekday, MonthNameType type = DateFormat);
  static String GetLongMonthName(int32 month, MonthNameType type = DateFormat);
  static String GetLongDayName(DayOfWeekType weekday, MonthNameType type = DateFormat);

  String ToString(DateFormatType format = DateFormatType::TextDate) const;
  String ToString(const String& format) const;

  static Date FromString(const String& string, DateFormatType format = DateFormatType::TextDate);
  static Date FromString(const String& string, const String& format);

 private:
  int32 days_;

  FUN_BASE_API friend uint32 HashOf(const Date& date);
  FUN_BASE_API friend Archive& operator & (Archive& ar, Date& date);
};


class Time {
 public:
  static const Time Null;
  static const Time MinValue;
  static const Time MaxValue;

 private:
  explicit Time(int64 ticks_in_day) : ticks_in_day_(ticks_in_day) {}

 public:
  Time() : ticks_in_day_(InvalidTime) {}
  Time(int32 hour, int32 minute, int32 second, int32 millisecond = 0, int32 microsecond = 0);
  Time(const Time& rhs) : ticks_in_day_(rhs.ticks_in_day_) {}
  Time& operator = (const Time& rhs) {
    ticks_in_day_ = rhs.ticks_in_day_;
    return *this;
  }

  void Swap(Time& rhs);

  friend void Swap(Time& a, Time& b) {
    a.Swap(b);
  }

  FUN_ALWAYS_INLINE bool IsNull() const {
    return ticks_in_day_ == InvalidTime;
  }
  bool IsValid() const;

  int32 Hour() const;
  int32 Minute() const;
  int32 Second() const;
  int32 Millisecond() const;
  int32 Microsecond() const;

  int32 HourAMPM() const;
  bool IsAM() const;
  bool IsPM() const;

  bool SetTime(int32 hour, int32 minute, int32 second, int32 millisecond = 0, int32 microsecond = 0);

  Time AddHours(double hours) const;
  Time AddMinutes(double minutes) const;
  Time AddSeconds(double seconds) const;
  Time AddMilliseconds(double milliseconds) const;
  Time AddMicroseconds(double microseconds) const;
  Time AddTicks(int64 ticks) const;

  Time& AddHoursInPlace(double hours);
  Time& AddMinutesInPlace(double minutes);
  Time& AddSecondsInPlace(double seconds);
  Time& AddMillisecondsInPlace(double milliseconds);
  Time& AddMicrosecondsInPlace(double microseconds);
  Time& AddTicksInPlace(int64 ticks);

  double HoursTo(const Time& to) const;
  double MinutesTo(const Time& to) const;
  double SecondsTo(const Time& to) const;
  double MillisecondsTo(const Time& to) const;
  double MicrosecondsTo(const Time& to) const;
  int64 TicksTo(const Time& to) const;

  FUN_ALWAYS_INLINE int32 Compare(const Time& other) const { return ticks_in_day_ == other.ticks_in_day_ ? 0 : (ticks_in_day_ < other.ticks_in_day_ ? -1 : +1); }
  FUN_ALWAYS_INLINE friend bool operator == (const Time& lhs, const Time& rhs) { return lhs.Compare(rhs) == 0; }
  FUN_ALWAYS_INLINE friend bool operator != (const Time& lhs, const Time& rhs) { return lhs.Compare(rhs) != 0; }
  FUN_ALWAYS_INLINE friend bool operator <  (const Time& lhs, const Time& rhs) { return lhs.Compare(rhs) <  0; }
  FUN_ALWAYS_INLINE friend bool operator <= (const Time& lhs, const Time& rhs) { return lhs.Compare(rhs) <= 0; }
  FUN_ALWAYS_INLINE friend bool operator >  (const Time& lhs, const Time& rhs) { return lhs.Compare(rhs) >  0; }
  FUN_ALWAYS_INLINE friend bool operator >= (const Time& lhs, const Time& rhs) { return lhs.Compare(rhs) >= 0; }

  static bool IsValid(int32 hour, int32 minute, int32 second, int32 millisecond = 0, int32 microsecond = 0);

  static Time CurrentTime();
  static Time UtcCurrentTime();

  FUN_ALWAYS_INLINE static Time FromTicksStartOfDay(int64 ticks_in_day) { return Time(ticks_in_day); }
  FUN_ALWAYS_INLINE int64 ToTicksStartOfDay() const { return ticks_in_day_ == InvalidTime ? 0 : ticks_in_day_; }
  FUN_ALWAYS_INLINE int64 ToMSecsStartOfDay() const { return ToTicksStartOfDay() / DateTimeConstants::TICKS_PER_MILLISECOND; }

 public:
  String ToString(DateFormatType format = DateFormatType::TextDate) const;
  String ToString(const String& format) const;

  static Time FromString(const String& string, DateFormatType format = DateFormatType::TextDate);
  static Time FromString(const String& string, const String& format);

 public:
  // Utilities
  void Start();
  int64 Restart();
  int64 Elapsed() const;

 private:
  static const int64 InvalidTime = -1;

  int64 ticks_in_day_;

  FUN_BASE_API friend uint32 HashOf(const Time& time);
  FUN_BASE_API friend Archive& operator & (Archive& ar, Time& time);
};


/**
*/
class FUN_BASE_API DateTime {
 public:
  static const DateTime None;
  static const DateTime Null;
  static const DateTime MinValue;
  static const DateTime MaxValue;

  DateTime();
  explicit DateTime(const Date& date);
  DateTime(const Date& date, const Time& time, TimeSpec spec = TimeSpec::Local);
  DateTime(const Date& date, const Time& time, TimeSpec spec, int32 offset_seconds);
  DateTime(const Date& date, const Time& time, const TimeZone& timezone);
  DateTime(const Timestamp& ts);

  DateTime(const DateTime& rhs);
  DateTime(DateTime&& rhs);
  DateTime& operator = (const DateTime& rhs);
  DateTime& operator = (DateTime&& rhs);

  void Swap(DateTime& rhs);

  friend void Swap(DateTime& a, DateTime& b) {
    a.Swap(b);
  }

  bool IsNull() const;
  bool IsValid() const;
  FUN_ALWAYS_INLINE static bool IsValid(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond, int32 microsecond) {
    return Date::IsValid(year, month, day) && Time::IsValid(hour, minute, second, millisecond, microsecond);
  }

  int32 Year() const;
  int32 Month() const;
  int32 Day() const;
  DayOfWeekType DayOfWeek() const;
  int32 DayOfYear() const;
  int32 DaysInMonth() const;
  int32 DaysInYear() const;
  FUN_ALWAYS_INLINE bool IsLeapYear() const { return Date::IsLeapYear(Year()); }

  int32 Hour() const;
  int32 Minute() const;
  int32 Second() const;
  int32 Millisecond() const;
  int32 Microsecond() const;

  int32 HourAMPM() const;
  bool IsAM() const;
  bool IsPM() const;

  Date GetDate() const;
  Time GetTime() const;

  TimeSpec GetTimeSpec() const;
  int32 GetOffsetFromUtc() const;

  TimeZone GetTimeZone() const;
  String GetTimeZoneAbbreviation() const;
  bool IsDaylightTime() const;

  int64 ToUtcTicks() const;
  int64 ToUtcTicksSinceEpoch() const;

  void SetDateOnly(const Date& date);
  void SetTimeOnly(const Time& time);
  void SetTimeSpec(TimeSpec spec);
  void SetOffsetFromUtc(int32 offset_seconds);
  void SetTimeZone(const TimeZone& timezone);
  void SetUtcTicks(int64 utc_ticks);
  void SetUtcTicksSinceEpoch(int64 utc_ticks_since_epoch);

  DateTime AddDays(int64 days) const;
  DateTime AddMonths(int32 months) const;
  DateTime AddYears(int32 years) const;
  DateTime AddHours(int64 hours) const;
  DateTime AddMinutes(int64 minutes) const;
  DateTime AddSeconds(int64 seconds) const;
  DateTime AddMilliseconds(int64 milliseconds) const;
  DateTime AddMicroseconds(int64 microseconds) const;
  DateTime AddTicks(int64 ticks) const;

  DateTime& AddDaysInPlace(int64 days);
  DateTime& AddMonthsInPlace(int32 months);
  DateTime& AddYearsInPlace(int32 years);
  DateTime& AddHoursInPlace(int64 hours);
  DateTime& AddMinutesInPlace(int64 minutes);
  DateTime& AddSecondsInPlace(int64 seconds);
  DateTime& AddMillisecondsInPlace(int64 milliseconds);
  DateTime& AddMicrosecondsInPlace(int64 microseconds);
  DateTime& AddTicksInPlace(int64 tickstoadd);

  FUN_ALWAYS_INLINE DateTime& operator += (const Timespan& span) {
    return AddTicksInPlace(span.ToTicks());
  }

  FUN_ALWAYS_INLINE DateTime& operator -= (const Timespan& span) {
    return AddTicksInPlace(-span.ToTicks());
  }

  FUN_ALWAYS_INLINE friend DateTime operator + (const DateTime& datetime, const Timespan& span) {
    return datetime.AddTicks(span.ToTicks());
  }

  FUN_ALWAYS_INLINE friend DateTime operator - (const DateTime& datetime, const Timespan& span) {
    return datetime.AddTicks(-span.ToTicks());
  }

  FUN_ALWAYS_INLINE friend Timespan operator - (const DateTime& lhs, const DateTime& rhs) {
    return Timespan(lhs.ToUtcTicks() - rhs.ToUtcTicks());
  }

  double DaysTo(const DateTime& to) const;
  double HoursTo(const DateTime& to) const;
  double SecondsTo(const DateTime& to) const;
  double MillisecondTo(const DateTime& to) const;
  double MicrosecondsTo(const DateTime& to) const;
  int64 TicksTo(const DateTime& to) const;

  int32 Compare(const DateTime& other) const;
  friend bool operator == (const DateTime& lhs, const DateTime& rhs);
  friend bool operator != (const DateTime& lhs, const DateTime& rhs);
  friend bool operator <  (const DateTime& lhs, const DateTime& rhs);
  friend bool operator <= (const DateTime& lhs, const DateTime& rhs);
  friend bool operator >  (const DateTime& lhs, const DateTime& rhs);
  friend bool operator >= (const DateTime& lhs, const DateTime& rhs);

  DateTime ToTimeSpec(TimeSpec spec) const;
  DateTime ToLocalTime() const;
  DateTime ToUniversalTime() const;
  DateTime ToOffsetFromUTC(int32 offset_seconds) const;
  DateTime ToTimeZone(const TimeZone& timezone) const;

  static DateTime FromUtcTicks(int64 utc_ticks);
  static DateTime FromUtcTicks(int64 utc_ticks, TimeSpec spec, int32 offset_from_utc = 0);
  static DateTime FromUtcTicks(int64 utc_ticks, const TimeZone& timezone);

  static DateTime FromUtcTicksSinceEpoch(int64 utc_ticks_since_epoch);
  static DateTime FromUtcTicksSinceEpoch(int64 utc_ticks_since_epoch, TimeSpec spec, int32 offset_from_utc = 0);
  static DateTime FromUtcTicksSinceEpoch(int64 utc_ticks_since_epoch, const TimeZone& timezone);

  static DateTime Now();
  static DateTime UtcNow();

  String ToString(DateFormatType format = DateFormatType::TextDate) const;
  String ToString(const String& format) const;

  static DateTime FromString(const String& string, DateFormatType format = DateFormatType::TextDate);
  static DateTime FromString(const String& string, const String& format);

  // 어떤식으로 처리하는게 바람직하려나?
  int32 ToShortValue() const;
  static DateTime FromShortValue(int32 value);

 public:
  struct Data {
    int64 ticks;
    int32 offset_from_utc;
    uint8 status;
    TimeZone* timezone;

    Data(TimeSpec spec = TimeSpec::Local);
    ~Data();
    Data(const Data& rhs);
    Data& operator = (const Data& rhs);
  };
  Data data_;

  FUN_BASE_API friend uint32 HashOf(const DateTime& d);
  FUN_BASE_API friend Archive& operator & (Archive& ar, DateTime& d);
};

//TODO 제거하도록 하자.
namespace Lex {
FUN_BASE_API String ToString(const Date& value);
FUN_BASE_API String ToString(const Time& value);
FUN_BASE_API String ToString(const DateTime& value);
}

} // namespace fun
