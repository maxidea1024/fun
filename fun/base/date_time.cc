/*
http://www.csharp-examples.net/string-format-datetime/

Specifier   DateTimeFormatInfo property Pattern value (for en-US culture)
t   ShortTimePattern    h:mm tt
d   ShortDatePattern    M/d/yyyy
T   LongTimePattern h:mm:ss tt
d   LongDatePattern dddd, MMMM dd, yyyy
f   (combination of d and t)    dddd, MMMM dd, yyyy h:mm tt
F   FullDateTimePattern dddd, MMMM dd, yyyy h:mm:ss tt
g   (combination of d and t)    M/d/yyyy h:mm tt
G   (combination of d and T)    M/d/yyyy h:mm:ss tt
m, M    MonthDayPattern MMMM dd
y, Y    YearMonthPattern    MMMM, yyyy
r, R    RFC1123Pattern  ddd, dd MMM yyyy hh':'mm':'ss 'GMT' (*)
s   SortableDateTi­mePattern    yyyy'-'mm'-'dd'T'hh':'mm':'ss (*)
u   UniversalSorta­bleDateTimePat­tern  yyyy'-'mm'-'dd hh':'mm':'ss'Z' (*)
    (*) = culture independent
Following examples show usage of standard format specifiers in string.Format method and the resulting output.

[C#]
string.Format("{0:t}", dt);  // "4:05 PM"                         ShortTime
string.Format("{0:d}", dt);  // "3/9/2008"                        ShortDate
string.Format("{0:T}", dt);  // "4:05:07 PM"                      LongTime
string.Format("{0:d}", dt);  // "Sunday, March 09, 2008"          LongDate
string.Format("{0:f}", dt);  // "Sunday, March 09, 2008 4:05 PM"  LongDate+ShortTime
string.Format("{0:F}", dt);  // "Sunday, March 09, 2008 4:05:07 PM" FullDateTime
string.Format("{0:g}", dt);  // "3/9/2008 4:05 PM"                ShortDate+ShortTime
string.Format("{0:G}", dt);  // "3/9/2008 4:05:07 PM"             ShortDate+LongTime
string.Format("{0:m}", dt);  // "March 09"                        MonthDay
string.Format("{0:y}", dt);  // "March, 2008"                     YearMonth
string.Format("{0:r}", dt);  // "Sun, 09 Mar 2008 16:05:07 GMT"   RFC1123
string.Format("{0:s}", dt);  // "2008-03-09T16:05:07"             SortableDateTime
string.Format("{0:u}", dt);  // "2008-03-09 16:05:07Z"            UniversalSortableDateTime
*/

//TODO 유효한 값의 정리가 필요해 보임.
//CTime의 유효 값은 정리가 되었고, CDate와 CDateTime의 유효값이 정리가 아직 안되었음.
//
//전반적으로 정리는 된듯 싶고, 유닛 테스트를 통해서 검증만 하면 될듯 함.

#include "fun/base/date_time.h"
#include "fun/base/time_zone_impl.h"
#include "fun/base/date_time_parser.h"
//TODO
//#include "fun/base/regex.h"
#include "fun/base/system_time.h"
#include "fun/base/locale/locale.h"

#include <time.h>

//TODO 임시로 truncation warning 꺼줌.. 추후에 코드 정리하자!
#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

namespace fun {

enum DaylightStatus {
  UnknownDaylightTime = -1,
  StandardTime = 0,
  DaylightTime = 1,
};

enum StatusFlag {
  ValidDate = 0x01,
  ValidTime = 0x02,
  ValidDateTime = 0x04,
  TimeSpecMask = 0x30,
  SetToStandardTime = 0x40,
  SetToDaylightTime = 0x80,
};

enum {
  TimeSpecShift = 4,
  ValidityMask = ValidDate | ValidTime | ValidDateTime,
  DaylightMask = SetToStandardTime | SetToDaylightTime,
};

FUN_ALWAYS_INLINE uint8 MergeSpec(uint8 status, TimeSpec spec) {
  status &= ~TimeSpecMask;
  status |= uint8(spec) << TimeSpecShift;
  return status;
}

FUN_ALWAYS_INLINE TimeSpec GetSpec(uint8 status) {
  return TimeSpec((status & TimeSpecMask) >> TimeSpecShift);
}

FUN_ALWAYS_INLINE uint8 MergeDaylightStatus(uint8 status, DaylightStatus daylight) {
  status &= ~DaylightMask;

  if (daylight == DaylightStatus::DaylightTime) {
    status |= SetToDaylightTime;
  }
  else if (daylight == DaylightStatus::StandardTime) {
    status |= SetToStandardTime;
  }

  return status;
}

FUN_ALWAYS_INLINE DaylightStatus ExtractDaylightStatus(uint8 status) {
  if (status & SetToDaylightTime) {
    return DaylightStatus::DaylightTime;
  } else if (status & SetToStandardTime) {
    return DaylightStatus::StandardTime;
  } else {
    return DaylightStatus::UnknownDaylightTime;
  }
}

static void CheckValidDateTime(DateTime::Data& d);

static void SetDateAndTime(DateTime::Data& d, const Date& date, const Time& time) {
  d.status &= ~(ValidityMask | DaylightMask);

  // If the date is valid and the time is not we set time to 00:00:00
  Time use_time = time;
  if (!use_time.IsValid() && date.IsValid()) {
    use_time = Time::FromTicksStartOfDay(0);
  }

  // Set date value and status
  int64 days = 0;
  if (date.IsValid()) {
    days = date.ToDays();
    d.status |= ValidDate;
  }

  // Set time value and status
  int64 ticks = 0;
  if (use_time.IsValid()) {
    ticks = use_time.ToTicksStartOfDay();
    d.status |= ValidTime;
  }

  d.ticks = days * DateTimeConstants::TICKS_PER_DAY + ticks;

  CheckValidDateTime(d);
}

static void fun_tzset() {
#if FUN_PLATFORM_WINDOWS_FAMILY
  _tzset();
#else
  tzset();
#endif
}

static int32 fun_timezone() {
#if FUN_PLATFORM_WINDOWS_FAMILY
  long offset;
  _get_timezone(&offset);
  return int32(offset);
#elif FUN_PLATFORM == PLATFORM_BSD4 && FUN_PLATFORM != PLATFORM_DARWIN
  __time64_t clock = time(nullptr);
  struct tm tm;
  localtime_r(&clock, &tm);
  return -tm.tm_gmtoff + (tm.tm_isdst ? 3600 : 0);
#else
  return timezone;
#endif
}

#if FUN_PLATFORM_WINDOWS_FAMILY
// UTF가 아닌 Local MBCS이므로 windows 함수를 사용해서 변환해야함.
// MBCS -> WIDE -> Utf8 과정을 거침. (비효율적이다...)
static String MbcsToUnicode(const char* mbcs) {
  const DWORD wide_len = MultiByteToWideChar(CP_ACP, 0, mbcs, -1, nullptr, 0);

  Array<wchar_t> wide(wide_len, NoInit);
  MultiByteToWideChar(CP_ACP, 0, mbcs, -1, wide.MutableData(), wide_len);

  String result = WCHAR_TO_UTF8(wide.ConstData());
  return result;
}
#endif //FUN_PLATFORM_WINDOWS_FAMILY

static String fun_tzname(DaylightStatus daylight_status) {
  int is_dst = (daylight_status == DaylightStatus::DaylightTime) ? 1 : 0;
#if defined(_MSC_VER) && _MSC_VER >= 1400
  size_t s = 0;
  char name[512];
  if (_get_tzname(&s, name, 512, is_dst) != 0) {
    return String();
  }
  return MbcsToUnicode(name);
#else
  return String(tzname[is_dst]);
#endif
}

static int64 fun_mktime(Date* date, Time* time, DaylightStatus* daylight_status, String* abbreviation, bool* ok = nullptr) {
  const int64 fraction = time->ToTicksStartOfDay() % DateTimeConstants::TICKS_PER_SECOND;
  int32 year, month, day;
  date->GetDate(&year, &month, &day);

  // All other platforms provide standard C library time functions
  tm local_tm;
  UnsafeMemory::Memset(&local_tm, 0x00, sizeof(local_tm));
  local_tm.tm_sec = time->Second();
  local_tm.tm_min = time->Minute();
  local_tm.tm_hour = time->Hour();
  local_tm.tm_mday = day;
  local_tm.tm_mon = month - 1;
  local_tm.tm_year = year - 1900;
  if (daylight_status) {
    local_tm.tm_isdst = int32(*daylight_status);
  } else {
    local_tm.tm_isdst = -1;
  }

#if FUN_PLATFORM_WINDOWS_FAMILY
  const int32 hour = local_tm.tm_hour;
#endif

  __time64_t secs_since_epoch = _mktime64(&local_tm);
  if (secs_since_epoch != __time64_t(-1)) {
    *date = Date(local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday);
    *time = Time(local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
    time->AddTicksInPlace(fraction); // add fraction.

  #if FUN_PLATFORM_WINDOWS_FAMILY
    // Windows mktime for the missing hour subtracts 1 hour from the time
    // instead of adding 1 hour.  If time differs and is standard time then
    // this has happened, so add 2 hours to the time and 1 hour to the msecs
    if (local_tm.tm_isdst == 0 && local_tm.tm_hour != hour) {
      if (time->Hour() >= 22) {
        *date = date->AddDays(1);
      }
      *time = time->AddHours(2);
      secs_since_epoch += 3600; // add 1 hour(3600 sec)
      local_tm.tm_isdst = 1;
    }
  #endif

    if (local_tm.tm_isdst >= 1) { // daylight
      if (daylight_status) { *daylight_status = DaylightStatus::DaylightTime; }
      if (abbreviation) { *abbreviation = fun_tzname(DaylightStatus::DaylightTime); }
    } else if (local_tm.tm_isdst == 0) { // standard
      if (daylight_status) { *daylight_status = DaylightStatus::StandardTime; }
      if (abbreviation) { *abbreviation = fun_tzname(DaylightStatus::StandardTime); }
    } else { // unknown
      if (daylight_status) { *daylight_status = DaylightStatus::UnknownDaylightTime; }
      if (abbreviation) { *abbreviation = fun_tzname(DaylightStatus::StandardTime); }
    }

    if (ok) {
      *ok = true;
    }
  } else { // _mktime64 fail
    *date = Date::Null;
    *time = Time::Null;
    if (daylight_status) { *daylight_status = DaylightStatus::UnknownDaylightTime; }
    if (abbreviation) { *abbreviation = String(); }

    if (ok) {
      *ok = false;
    }
  }

  return ((int64)secs_since_epoch * DateTimeConstants::TICKS_PER_SECOND) + fraction;
}

// Calls the platform variant of localtime for the given msecs, and updates
// the date, time, and DST status with the returned values.
static bool fun_localtime(int64 ticks_since_epoch, Date* local_date, Time* local_time, DaylightStatus* daylight_status) {
  const __time64_t secs_since_epoch = ticks_since_epoch / DateTimeConstants::TICKS_PER_SECOND;
  const int32 fraction = ticks_since_epoch % DateTimeConstants::TICKS_PER_SECOND;

  tm local_tm;
  bool is_valid = false;

  // localtime() is required to work as if tzset() was called before it.
  // localtime_r() does not have this requirement, so make an explicit call.
  // The explicit call should also request the timezone info be re-parsed.
  fun_tzset();

#if defined(_MSC_VER) && _MSC_VER >= 1400
  if (_localtime64_s(&local_tm, &secs_since_epoch) == 0) {
    is_valid = true;
  }
#else
  if (const tm* res = localtime64_r(&secs_since_epoch, &local_tm)) {
    local_tm = *res;
    is_valid = true;
  }
#endif

  if (is_valid) {
    *local_date = Date(local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday);
    *local_time = Time(local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
    local_time->AddTicksInPlace(fraction); // add fraction

    if (daylight_status) {
      if (local_tm.tm_isdst > 0) {
        *daylight_status = DaylightStatus::DaylightTime;
      } else if (local_tm.tm_isdst < 0) {
        *daylight_status = DaylightStatus::UnknownDaylightTime;
      } else {
        *daylight_status = DaylightStatus::StandardTime;
      }
    }

    return true;
  } else {
    *local_date = Date::Null;
    *local_time = Time::Null;

    if (daylight_status) {
      *daylight_status = DaylightStatus::UnknownDaylightTime;
    }

    return false;
  }
}

// Converts an msecs value into a date and time.
FUN_ALWAYS_INLINE void TicksToDateTime(int64 ticks, Date* date, Time* time) {
  if (date) {
    *date = Date::FromDays(ticks / DateTimeConstants::TICKS_PER_DAY);
  }

  if (time) {
    *time = Time::FromTicksStartOfDay(ticks % DateTimeConstants::TICKS_PER_DAY);
  }
}

// Converts a date/time value into msecs.
FUN_ALWAYS_INLINE int64 DateTimeToTicks(const Date& date, const Time& time) {
  return date.ToDays() * DateTimeConstants::TICKS_PER_DAY + time.ToTicksStartOfDay();
}

// Convert an msecs since utc into local time.
static bool UtcTicksToLocalDateTime(int64 utc_ticks, Date* local_date, Time* local_time, DaylightStatus* daylight_status = nullptr) {
  const int64 ticks_since_epoch = utc_ticks - DateTimeConstants::EPOCH_TICKS;

  if (ticks_since_epoch < 0) {
    //Epoch 이전임. DST 적용을 할 수 없으므로, 표준시를 적용해서 처리해줌.

    // Docs state any local_time before 1970-01-01 will *not* have any daylight time applied
    // Instead just use the standard offset from UTC to convert to UTC time
    fun_tzset();

    TicksToDateTime(ticks_since_epoch - fun_timezone() * DateTimeConstants::TICKS_PER_SECOND + DateTimeConstants::EPOCH_TICKS, local_date, local_time);

    if (daylight_status) {
      *daylight_status = DaylightStatus::StandardTime;
    }

    return true;

  } else if (ticks_since_epoch > int64((int64)DateTimeConstants::TIME_T_MAX * DateTimeConstants::TICKS_PER_SECOND)) {
    // 2037년을 넘을 경우에도 DST를 구할 수 없음.

    // Docs state any local_time after 2037-12-31 *will* have any DST applied
    // but this may fall outside the supported __time64_t range, so need to fake it.
    // Use existing method to fake the conversion, but this is deeply flawed as it may
    // apply the conversion from the wrong day number, e.g. if rule is last Sunday of month

    Date utc_date;
    Time utc_time;
    TicksToDateTime(ticks_since_epoch + DateTimeConstants::EPOCH_TICKS, &utc_date, &utc_time);

    int32 year, month, day;
    utc_date.GetDate(&year, &month, &day);
    // 2037 is not a leap year, so make sure date isn't Feb 29
    if (month == 2 && day == 29) {
      --day;
    }

    Date fake_date(2037, month, day);
    const int64 face_ticks_since_epoch = DateTime(fake_date, utc_time, TimeSpec::UTC).ToUtcTicksSinceEpoch();
    const bool ok = fun_localtime(face_ticks_since_epoch, local_date, local_time, daylight_status);
    *local_date = local_date->AddDays(fake_date.DaysTo(utc_date));
    return ok;
  } else {
    // Falls inside __time64_t suported range so can use localtime
    return fun_localtime(utc_ticks - DateTimeConstants::EPOCH_TICKS, local_date, local_time, daylight_status);
  }
}

//TODO 기존 코드에서는 2037년도까지 있었지만, 이젠 제한을 안해도 될듯... 기존 32비트 api에서 64비트로 변경했으니...
// 윈도우즈는 3000년까지, linux는 9999년까지 지원한다고 한다.
// Convert a local_time expressed in local msecs encoding and the corresponding
// DST status into a UTC msecs. Optionally populate the returned
// values from mktime for the adjusted local date and time.
static int64 LocalToUtcTicks( int64 local_ticks,
                              DaylightStatus* daylight_status,
                              Date* local_date = nullptr,
                              Time* local_time = nullptr,
                              String* abbreviation = nullptr) {
  Date date;
  Time time;
  TicksToDateTime(local_ticks, &date, &time);

  const int64 local_ticks_since_epoch = local_ticks - DateTimeConstants::EPOCH_TICKS;
  const int64 max_ticks_since_epoch = int64(DateTimeConstants::TIME_T_MAX) * DateTimeConstants::TICKS_PER_SECOND;

  if (local_ticks_since_epoch <= DateTimeConstants::TICKS_PER_DAY) {
    if (local_ticks_since_epoch >= -DateTimeConstants::TICKS_PER_DAY) {
      bool is_valid;
      const int64 utc_ticks_since_epoch = fun_mktime(&date, &time, daylight_status, abbreviation, &is_valid);
      if (is_valid && utc_ticks_since_epoch >= 0) {
        if (local_date) {
          *local_date = date;
        }

        if (local_time) {
          *local_time = time;
        }

        return utc_ticks_since_epoch + DateTimeConstants::EPOCH_TICKS;
      }
    } else {
      // If we don't call mktime then need to call tzset to get offset
      fun_tzset();
    }

    // Time is clearly before 1970-01-01 so just use standard offset to convert
    const int64 utc_ticks_since_epoch = local_ticks_since_epoch + fun_timezone() * DateTimeConstants::TICKS_PER_SECOND;
    if (local_date || local_time) {
      TicksToDateTime(local_ticks, local_date, local_time);
    }

    if (daylight_status) {
      *daylight_status = DaylightStatus::StandardTime;
    }

    if (abbreviation) {
      *abbreviation = fun_tzname(DaylightStatus::StandardTime);
    }
    return utc_ticks_since_epoch + DateTimeConstants::EPOCH_TICKS;

  } else if (local_ticks_since_epoch >= max_ticks_since_epoch - DateTimeConstants::TICKS_PER_DAY) {
    // Docs state any local_time after 2037-12-31 *will* have any DST applied
    // but this may fall outside the supported __time64_t range, so need to fake it.

    // First, if localMsecs is within +/- 1 day of maximum __time64_t try mktime in case it does
    // fall before maximum and can use proper DST conversion

    // 마지막 하루안일 경우에 처리.

    if (local_ticks_since_epoch <= max_ticks_since_epoch + DateTimeConstants::TICKS_PER_DAY) {
      bool is_valid;
      const int64 utc_ticks_since_epoch = fun_mktime(&date, &time, daylight_status, abbreviation, &is_valid);
      if (is_valid && utc_ticks_since_epoch <= max_ticks_since_epoch) {
        if (local_date) {
          *local_date = date;
        }

        if (local_time) {
          *local_time = time;
        }

        return utc_ticks_since_epoch + DateTimeConstants::EPOCH_TICKS;
      }
    }

    // Use existing method to fake the conversion, but this is deeply flawed as it may
    // apply the conversion from the wrong day number, e.g. if rule is last Sunday of month
    // TODO Use TimeZone when available to apply the future rule correctly

    int32 year, month, day;
    date.GetDate(&year, &month, &day);
    // 2037 is not a leap year, so make sure date isn't Feb 29
    if (month == 2 && day == 29) {
      --day;
    }

    Date fake_date(2037, month, day);
    int64 fake_diff = fake_date.DaysTo(date);
    int64 utc_ticks = fun_mktime(&fake_date, &time, daylight_status, abbreviation) + DateTimeConstants::EPOCH_TICKS;
    if (local_date) {
      *local_date = fake_date.AddDays(fake_diff);
    }

    if (local_time) {
      *local_time = time;
    }

    Date utc_date;
    Time utc_time;
    TicksToDateTime(utc_ticks, &utc_date, &utc_time);
    utc_date = utc_date.AddDays(fake_diff);
    utc_ticks = DateTimeToTicks(utc_date, utc_time);
    return utc_ticks;
  } else {
    // Clearly falls inside 1970-2037 suported range so can use mktime
    const int64 ticks_since_epoch = fun_mktime(&date, &time, daylight_status, abbreviation);
    if (local_date) {
      *local_date = date;
    }

    if (local_time) {
      *local_time = time;
    }

    return ticks_since_epoch + DateTimeConstants::EPOCH_TICKS;
  }
}

// Convert a timezone time expressed in zone msecs encoding into a UTC msecs
// DST transitions are disambiguated by hint.
static int64 ZoneToUtcTicks(int64 zone_ticks,
                            const TimeZone& zone,
                            DaylightStatus hint,
                            Date* local_date = nullptr,
                            Time* local_time = nullptr) {
  // Get the effective data from TimeZone
  const auto data = zone.impl_->DataForLocalTime((zone_ticks - DateTimeConstants::EPOCH_TICKS) / DateTimeConstants::TICKS_PER_MILLISECOND, int32(hint));

  // Docs state any local_time before 1970-01-01 will *not* have any DST applied
  // but all affected times afterwards will have DST applied.
  if (data.at_msecs_since_epoch >= 0) {
    TicksToDateTime(data.at_msecs_since_epoch * DateTimeConstants::TICKS_PER_MILLISECOND + (data.offset_from_utc * DateTimeConstants::TICKS_PER_SECOND) + DateTimeConstants::EPOCH_TICKS, local_date, local_time);
    return data.at_msecs_since_epoch * DateTimeConstants::TICKS_PER_MILLISECOND + DateTimeConstants::EPOCH_TICKS;
  } else {
    TicksToDateTime(zone_ticks, local_date, local_time);
    return zone_ticks - (data.standard_time_offset * DateTimeConstants::TICKS_PER_SECOND);
  }
}

// 설정된 시간이 유효한지 여부를 체크하고, OffsetFromUTC를 계산함.
// 단, Local/timezone 일 경우에만 호출됨.
static void RefreshDateTime(DateTime::Data& d) {
  const TimeSpec spec = GetSpec(d.status);

  uint8 status = d.status;
  const int64 ticks = d.ticks;
  int64 utc_ticks = 0;

  Date test_date;
  Time test_time;

  fun_check(spec == TimeSpec::Local || spec == TimeSpec::TimeZone);

  if (spec == TimeSpec::TimeZone) {
    if (d.timezone->IsValid()) {
      utc_ticks = ZoneToUtcTicks(ticks, *d.timezone, ExtractDaylightStatus(d.status), &test_date, &test_time);
      //실제로 참조되지 않고, 아래에서 다시 설정하므로 생략?
      //d.offset_from_utc = d.timezone->impl_->OffsetFromUTC((utc_ticks - DateTimeConstants::EPOCH_TICKS) / DateTimeConstants::TICKS_PER_MILLISECOND);
    } else {
      status &= ~ValidDateTime;
    }
  }

  // If not valid date and time then is invalid
  if (!(status & ValidDate) || !(status & ValidTime)) {
    status &= ~ValidDateTime;
    d.status = status;
    d.offset_from_utc = 0;
    return;
  }

  if (spec == TimeSpec::Local) {
    DaylightStatus daylight_status = ExtractDaylightStatus(d.status);
    utc_ticks = LocalToUtcTicks(ticks, &daylight_status, &test_date, &test_time);
  }

  // 내부에서 교정되었는지 여부 확인. (오류일 경우, 원래의 값과 다를것이므로...)
  // DST 범위내에 있지 않을 경우, 표현이 안될 수 있기 때문임.
  const int64 test_ticks = DateTimeToTicks(test_date, test_time);
  if (test_ticks == ticks) { // 계산된 시간과 예상한 시간이 같으므로, OK
    status |= ValidDateTime;
    d.offset_from_utc = (ticks - utc_ticks) / DateTimeConstants::TICKS_PER_SECOND;
  } else {
    status &= ~ValidDateTime;
    d.offset_from_utc = 0;
  }

  d.status = status;
}

static void CheckValidDateTime(DateTime::Data& d) {
  const TimeSpec spec = GetSpec(d.status);
  switch (spec) {
  case TimeSpec::OffsetFromUTC:
  case TimeSpec::UTC:
    // for these, a valid date and a valid time imply a valid DateTime
    if ((d.status & ValidDate) && (d.status & ValidTime)) {
      d.status |= ValidDateTime;
    } else {
      d.status &= ~ValidDateTime;
    }
    break;

  case TimeSpec::TimeZone:
  case TimeSpec::Local:
    // for these, we need to check whether the timezone is valid and whether
    // the time is valid in that timezone. Expensive, but no other option.
    RefreshDateTime(d);
    break;
  }
}

static void SetTimeSpec_helper(DateTime::Data& d, TimeSpec spec, int32 offset_seconds) {
  uint8 status = d.status;
  status &= ~(ValidDateTime | DaylightMask | TimeSpecMask);

  switch (spec) {
  case TimeSpec::OffsetFromUTC:
    if (offset_seconds == 0) {
      spec = TimeSpec::UTC;
    }
    break;

  // TimeZone일 경우에는 SetTimeSpec 함수로 하지 말고, SetTimeZone으로 해야함.
  // 에러 처리를 하는게 좋을까?
  // 원래 코드로 하면, 강제로 로컬로 변경되는데 이로인해서 발생하는 문제는 없을런지?
  case TimeSpec::TimeZone:
    // Use system time zone instead
    spec = TimeSpec::Local;
    //fall through
    fun_check(0); // 나중에 상황을 파악하기 위해서 브레이크 걸어둠.

  case TimeSpec::UTC:
  case TimeSpec::Local:
    offset_seconds = 0;
    break;
  }

  d.status = MergeSpec(status, spec);
  d.offset_from_utc = offset_seconds;
  *d.timezone = TimeZone();
}

static String ToOffsetString(DateFormatType format, int32 offset) {
  //return String::Format("{}{:02d}{}{:02d}",
  //  offset >= 0 ? '+' : '-',
  //  MathBase::Abs(offset) / 3600,
  //  format == DateFormatType::TextDate ? UTEXT("") : UTEXT(":"),
  //  (MathBase::Abs(offset) / 60) % 60);

  //TODO format_str 재조정해야함.
  return String::Format("%c%02d%s%02d",
              offset >= 0 ? '+' : '-',
              MathBase::Abs(offset) / 3600,
              format == DateFormatType::TextDate ? "" : ":",
              (MathBase::Abs(offset) / 60) % 60);
}

// Parse offset in [+-]hh[[:]mm] format
static int32 FromOffsetString(const String& string, bool* is_valid) {
  *is_valid = false;

  const int32 len = string.Len();
  if (len < 2 || len > 6) {
    return 0;
  }

  // sign will be +1 for a positive and -1 for a negative offset
  int32 sign;

  // First char must be + or -
  const UNICHAR sign_char = string[0];
  if (sign_char == '+') {
    sign = 1;
  } else if (sign_char == '-') {
    sign = -1;
  } else {
    return 0;
  }

  // Split the hour and minute parts
  const String time = string.Mid(1);
  int32 hh_len = time.IndexOf(':');
  int32 mm_index;
  if (hh_len == -1) {
    mm_index = hh_len = 2; // [+-]HHmm or [+-]hh format
  } else {
    mm_index = hh_len + 1;
  }

  const String hh = time.Left(hh_len);
  bool ok = false;
  const int32 hour = hh.ToInt32(&ok);
  if (!ok) {
    return 0;
  }

  const String mm = time.Mid(mm_index);
  const int32 minute = mm.IsEmpty() ? 0 : mm.ToInt32(&ok);
  if (!ok || minute < 0 || minute > 59) {
    return 0;
  }

  *is_valid = true;
  return sign * ((hour * 60) + minute) * 60;
}


static const int32 DAYS_PER_MONTH[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static const int32 DAYS_TO_MONTH365[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
static const int32 DAYS_TO_MONTH366[13] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

//
// Date
//

uint32 HashOf(const Date& date) {
  return HashOf(date.days_);
}

Archive& operator & (Archive& ar, Date& date) {
  return ar & date.days_;
}

static int32 DateToDays(int32 year, int32 month, int32 day) {
  if (Date::IsValid(year, month, day)) {
    const int32* days = Date::IsLeapYear(year) ? DAYS_TO_MONTH366 : DAYS_TO_MONTH365;
    if (day >= 1 && day <= days[month] - days[month - 1]) {
      const int32 y = year - 1;
      const int32 n = (y * 365) + (y / 4) - (y / 100) + (y / 400) + days[month - 1] + day - 1;
      return n;
    }
  }

  return Date::NullDaysValue;
}


const Date Date::Null;
const int32 Date::NullDaysValue = int32_MAX;
const Date Date::MinValue = Date(0);
const Date Date::MaxValue = Date(int32_MAX - 1);

void Date::Swap(Date& rhs) {
  fun::Swap(days_, rhs.days_);
}


//
// Date / Time formatting helper functions.
//

static const char SHORT_MONTH_NAMES[][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static int32 MonthNumberFromShortName(const String& short_name) {
  for (int32 i = 0; i < 12; ++i) {
    if (short_name == SHORT_MONTH_NAMES[i]) {
      return i + 1;
    }
  }
  return -1;
}

static int32 FromShortMonthName(const String& short_name) {
  int32 month = MonthNumberFromShortName(short_name);
  if (month != -1) {
    return month;
  }

  for (int32 i = 1; i <= 12; ++i) {
    if (short_name == Date::GetShortMonthName(i)) {
      return i;
    }
  }

  return -1;
}


#if !FUN_NO_DATESTRING
struct ParsedRfcDateTime {
  Date date;
  Time time;
  int32 utc_offset;
};

static ParsedRfcDateTime RfcDateImpl(const String& string) {
  // 1. "Wdy, DD Mon YYYY hh:mm:ss ±hhmm" (Wdy, being optional)
  // 2. "Wdy Mon DD hh:mm:ss YYYY"

  ParsedRfcDateTime result;

  //TODO
  //TODO
  //TODO
  //TODO
  //TODO

  fun_check(0);

  /*
  CRegularExpressionMatch Match;

  // Matches "Wdy, DD Mon YYYY hh:mm:ss ±hhmm" (Wdy, being optional)
  CRegularExpression Regex(StringLiteral("^(?:[A-Z][a-z]+,)?[ \\t]*(\\d{1,2})[ \\t]+([A-Z][a-z]+)[ \\t]+(\\d\\d\\d\\d)(?:[ \\t]+(\\d\\d):(\\d\\d)(?::(\\d\\d))?)?[ \\t]*(?:([+-])(\\d\\d)(\\d\\d))?"));
  if (string.IndexOf(Regex, 0, &Match) == 0) {
    const Array<String> Captures = Match.CapturedTexts();
    result.date = Date(Captures[3].ToInt32(), MonthNumberFromShortName(Captures[2]), Captures[1].ToInt32());
    if (!Captures[4].IsEmpty()) {
      result.time = Time(Captures[4].ToInt32(), Captures[5].ToInt32(), Captures[6].ToInt32());
    }
    const bool bPositiveOffset = Captures[7] == AsciiString("+");
    const int32 HourOffset = Captures[8].ToInt32();
    const int32 MinuteOffset = Captures[9].ToInt32();
    result.utc_offset = ((HourOffset * 60 + MinuteOffset) * (bPositiveOffset ? 60 : -60));
  } else {
    // Matches "Wdy Mon DD hh:mm:ss YYYY"
    CRegularExpression Regex(CStringLiteral("^[A-Z][a-z]+[ \\t]+([A-Z][a-z]+)[ \\t]+(\\d\\d)(?:[ \\t]+(\\d\\d):(\\d\\d):(\\d\\d))?[ \\t]+(\\d\\d\\d\\d)[ \\t]*(?:([+-])(\\d\\d)(\\d\\d))?"));
    if (string.IndexOf(Regex, 0, &Match) == 0) {
      const Array<String> Captures = Match.CapturedTexts();
      result.date = Date(Captures[6].ToInt32(), MonthNumberFromShortName(Captures[1]), Captures[2].ToInt32());
      if (!Captures[3].IsEmpty()) {
        result.time = Time(Captures[3].ToInt32(), Captures[4].ToInt32(), Captures[5].ToInt32());
      }
      const bool bPositiveOffset = Captures[7] == AsciiString("+");
      const int32 HourOffset = Captures[8].ToInt32();
      const int32 MinuteOffset = Captures[9].ToInt32();
      result.utc_offset = ((HourOffset * 60 + MinuteOffset) * (bPositiveOffset ? 60 : -60));
    }
  }
  */

  return result;
}
#endif // FUN_NO_DATESTRING

DayOfWeekType Date::DayOfWeek() const {
  if (!IsValid()) {
    return DayOfWeekType::Sunday;
  }

  return static_cast<DayOfWeekType>((days_ + 1) % 7);
}

int32 Date::DaysInYear() const {
  if (!IsValid()) {
    return 0;
  }

  return IsLeapYear(Year()) ? 366 : 355;
}

int32 Date::Year() const {
  return GetDatePart(DatePart::Year);
}

int32 Date::Month() const {
  return GetDatePart(DatePart::Month);
}

int32 Date::Day() const {
  return GetDatePart(DatePart::Day);
}

int32 Date::DaysInMonth() const {
  if (!IsValid()) {
    return 0;
  }

  return DaysInMonth(Year(), Month());
}

int32 Date::DayOfYear() const {
  return GetDatePart(DatePart::DayOfYear);
}

int32 Date::GetDatePart(DatePart part) const {
  if (!IsValid()) {
    return 0;
  }

  // Number of days in a non-leap year
  const int32 DaysPerYear = 365;
  // Number of days in 4 years
  const int32 DaysPer4Years = DaysPerYear * 4 + 1;        // 1461
  // Number of days in 100 years
  const int32 DaysPer100Years = DaysPer4Years * 25 - 1;   // 36524
  // Number of days in 400 years
  const int32 DaysPer400Years = DaysPer100Years * 4 + 1;  // 146097

  // N = number of days since 1/1/0001
  int32 N = days_;

  // Y400 = number of whole 400-year periods since 1/1/0001
  int32 Y400 = N / DaysPer400Years;
  // N = day number within 400-year period
  N -= Y400 * DaysPer400Years;
  // Y100 = number of whole 100-year periods within 400-year period
  int32 Y100 = N / DaysPer100Years;
  // Last 100-year period has an extra day, so decrement result if 4
  if (Y100 == 4) Y100 = 3;
  // N = day number within 100-year period
  N -= Y100 * DaysPer100Years;
  // Y4 = number of whole 4-year periods within 100-year period
  int32 Y4 = N / DaysPer4Years;
  // N = day number within 4-year period
  N -= Y4 * DaysPer4Years;
  // Y1 = number of whole years within 4-year period
  int32 Y1 = N / DaysPerYear;
  // Last year has an extra day, so decrement result if 4
  if (Y1 == 4) Y1 = 3;
  // If year was requested, compute and return it
  if (part == DatePart::Year) {
    return Y400 * 400 + Y100 * 100 + Y4 * 4 + Y1 + 1;
  }

  // N = day number within year
  N -= Y1 * DaysPerYear;
  // If day-of-year was requested, return it
  if (part == DatePart::DayOfYear) {
    return N + 1;
  }

  // Leap year calculation looks different from IsLeapYear since Y1, Y4,
  // and Y100 are relative to year 1, not year 0
  const bool is_leaf_year = Y1 == 3 && (Y4 != 24 || Y100 == 3);
  const int32* days = is_leaf_year ? DAYS_TO_MONTH366 : DAYS_TO_MONTH365;
  // All months have less than 32 days, so N >> 5 is a good conservative
  // estimate for the month
  int32 M = (N >> 5) + 1;
  // M = 1-based month number
  while (N >= days[M]) M++;
  // If month was requested, return it
  if (part == DatePart::Month) {
    return M;
  }

  // Return 1-based day-of-month
  return N - days[M - 1] + 1;
}

bool Date::SetDate(int32 year, int32 month, int32 day) {
  if (IsValid(year, month, day)) {
    days_ = DateToDays(year, month, day);
    return true;
  }
  else {
    days_ = NullDaysValue;
    return false;
  }
}

bool Date::GetDate(int32* year, int32* month, int32* day) const {
  const bool is_valid = IsValid();

  int32 i = 0, j = 0, k = 0, l = 0, n = 0;
  if (is_valid) {
    l = int32(ToJulianDay() + 0.5) + 68569;
    n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    j = 80 * l / 2447;
    k = l - 2447 * j / 80;
    l = j / 11;
    j = j + 2 - 12 * l;
    i = 100 * (n - 49) + i + l;
  }

  if (year) {
    *year = i;
  }

  if (month) {
    *month = j;
  }

  if (day) {
    *day = k;
  }

  return is_valid;
}

Date Date::AddDays(int32 days) const {
  return Date(*this).AddDaysInPlace(days);
}

Date Date::AddMonths(int32 months) const {
  return Date(*this).AddMonthsInPlace(months);
}

Date Date::AddYears(int32 years) const {
  return Date(*this).AddYearsInPlace(years);
}

Date& Date::AddDaysInPlace(int32 days) {
  *this = FromDays(days_ + days);
  return *this;
}

Date& Date::AddMonthsInPlace(int32 months) {
  if (!IsValid()) {
    return *this;
  }

  if (months < -120000 || months > 120000) {
    //todo error handling
    return *this;
  }

  int32 y, m, d;
  GetDate(&y, &m, &d);

  int32 i = m - 1 + months;
  if (i >= 0) {
    m = (i % 12) + 1;
    y = y + (i / 12);
  } else {
    m = 12 + (i + 1) % 12;
    y = y + (i - 11) / 12;
  }

  if (y < 1 || y > 9999) {
    //todo error handling
    return *this;
  }

  const int32 days = DaysInMonth(y, m);
  if (d > days) {
    d = days;
  }

  *this = Date(y, m, d);
  return *this;
}

Date& Date::AddYearsInPlace(int32 months) {
  return AddMonthsInPlace(months * 12);
}

int32 Date::DaysTo(const Date& to) const {
  return to.ToDays() - ToDays();
}

double Date::ToJulianDay() const {
  return double(1721425.5) + (double)ToDays();
}

double Date::ToModifiedJulianDay() const {
  return ToJulianDay() - 2400000.5;
}

Date Date::FromJulianDay(double julian_day) {
  return Date(int64(julian_day - 1721425.5));
}

bool Date::IsValid(int32 year, int32 month, int32 day) {
  // there is no year 0 in the gregorian calendar
  if (year == 0) {
    return false;
  }

  return  (day > 0 && month > 0 && month <= 12) &&
          (day >= 1 && day <= DaysInMonth(year, month));
}

bool Date::IsLeapYear(int32 year) {
  // No year 0 in gregorian calendar, so -1, -5, -9 etc are leap years
  if (year < 1) {
    ++year;
  }

  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int32 Date::DaysInMonth(int32 year, int32 month) {
  fun_check(month >= 1 && month <= 12);
  if (month == 2 && IsLeapYear(year)) {
    return 29;
  }

  return DAYS_PER_MONTH[month];
}

int32 Date::DaysInYear(int32 year) {
  return IsLeapYear(year) ? 366 : 365; //윤년에서는 1년이 366일.
}

Date Date::Today() {
  return DateTime::Now().GetDate();
}

Date Date::UtcToday() {
  return DateTime::UtcNow().GetDate();
}

String Date::GetShortMonthName(int32 month, MonthNameType type) {
  if (month >= 1 && month <= 12) {
    switch (type) {
    case MonthNameType::DateFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetMonthName(month, Locale::ShortFormat).c_str());
    case MonthNameType::StandaloneFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetStandaloneMonthName(month, Locale::ShortFormat).c_str());
    }
  }

  return String();
}

String Date::GetLongMonthName(int32 month, MonthNameType type) {
  if (month >= 1 && month <= 12) {
    switch (type) {
    case MonthNameType::DateFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetMonthName(month, Locale::LongFormat).c_str());
    case MonthNameType::StandaloneFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetStandaloneMonthName(month, Locale::LongFormat).c_str());
    }
  }

  return String();
}

String Date::GetShortDayName(DayOfWeekType weekday, MonthNameType type) {
  if (weekday >= DayOfWeekType::Sunday && weekday <= DayOfWeekType::Saturday) {
    switch (type) {
    case MonthNameType::DateFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetDayName(weekday, Locale::ShortFormat).c_str());
    case MonthNameType::StandaloneFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetStandaloneDayName(weekday, Locale::ShortFormat).c_str());
    }
  }

  return String();
}

String Date::GetLongDayName(DayOfWeekType weekday, MonthNameType type) {
  if (weekday >= DayOfWeekType::Sunday && weekday <= DayOfWeekType::Saturday) {
    switch (type) {
    case MonthNameType::DateFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetDayName(weekday, Locale::LongFormat).c_str());
    case MonthNameType::StandaloneFormat:
      return UNICHAR_TO_UTF8(Locale::System().GetStandaloneDayName(weekday, Locale::LongFormat).c_str());
    }
  }

  return String();
}


//TODO 이거 왜이리 안이쁘게 나오지? 더군다나 이게 기본이네?
static String ToStringTextDate(const Date& date) {
  return String::Format("{0} {1} {2} {3}",
                        *date.GetShortDayName(date.DayOfWeek()),
                        *date.GetShortMonthName(date.Month()),
                        date.Day(),
                        date.Year());
}

static String ToStringIsoDate(int64 days) {
  const Date date = Date::FromDays(days);

  //year 0가 유효한건가?
  if (date.Year() >= 0 && date.Year() <= 9999) {
    return String::Format("%04d-%02d-%02d", date.Year(), date.Month(), date.Day());
  } else {
    return String();
  }
}

String Date::ToString(DateFormatType format) const {
  if (!IsValid()) {
    return String(AsciiString("(null)"));
  }

  switch (format) {
    case DateFormatType::SystemLocaleShortDate:
      return UNICHAR_TO_UTF8(Locale::System().ToString(*this, Locale::ShortFormat).c_str());

    case DateFormatType::SystemLcaleLongDate:
      return UNICHAR_TO_UTF8(Locale::System().ToString(*this, Locale::LongFormat).c_str());

    case DateFormatType::DefaultLocaleShortDate:
      return UNICHAR_TO_UTF8(Locale().ToString(*this, Locale::ShortFormat).c_str());

    case DateFormatType::DefaultLocaleLongDate:
      return UNICHAR_TO_UTF8(Locale().ToString(*this, Locale::LongFormat).c_str());

    case DateFormatType::RFC2822Date:
      return UNICHAR_TO_UTF8(Locale::CLocale().ToString(*this, AsciiString("dd MMM yyyy")).c_str());

    default:
    case DateFormatType::TextDate:
      return ToStringTextDate(*this);

    case DateFormatType::ISODate:
    case DateFormatType::ISODateWithMs:
      return ToStringIsoDate(days_);
  }
}

String Date::ToString(const String& format) const {
  return UNICHAR_TO_UTF8(Locale::System().ToString(*this, UString::FromUtf8(format)).c_str());
}

Date Date::FromString(const String& string, DateFormatType format) {
  if (string.IsEmpty()) {
    return Date::Null;
  }

  switch (format) {
    case DateFormatType::SystemLocaleShortDate:
      return Locale::System().ToDate(UString::FromUtf8(string), Locale::ShortFormat);

    case DateFormatType::SystemLcaleLongDate:
      return Locale::System().ToDate(UString::FromUtf8(string), Locale::LongFormat);

    case DateFormatType::DefaultLocaleShortDate:
      return Locale().ToDate(UString::FromUtf8(string), Locale::ShortFormat);

    case DateFormatType::DefaultLocaleLongDate:
      return Locale().ToDate(UString::FromUtf8(string), Locale::LongFormat);

    case DateFormatType::RFC2822Date:
      return RfcDateImpl(string).date;

    default:
    case DateFormatType::TextDate: {
      const Array<String> parts = string.Split(" ");

      if (parts.Count() != 4) {
        return Date::Null;
      }

      const String month_name = parts[1];
      const int32 month = FromShortMonthName(month_name);
      if (month == -1) {
        // Month name matches neither english nor other localised name.
        return Date::Null;
      }

      bool ok = false;
      const int32 year = parts[3].ToInt32(&ok);
      if (!ok) {
        return Date::Null;
      }

      return Date(year, month, parts[2].ToInt32());
    }

    case DateFormatType::ISODate: {
      // Semi-strict parsing, must be long enough and have non-numeric separators
      if (string.Len() < 10 ||
          CharTraitsU::IsDigit(string[4]) ||
          CharTraitsU::IsDigit(string[7]) ||
          (string.Len() > 10 && CharTraitsU::IsDigit(string[10]))) {
        return Date::Null;
      }

      const int32 year = string.Mid(0, 4).ToInt32();
      if (year <= 0 || year > 9999) {
        return Date::Null;
      }
      return Date(year, string.Mid(5, 2).ToInt32(), string.Mid(8, 2).ToInt32());
    }
  } // end of switch

  return Date::Null;
}

Date Date::FromString(const String& string, const String& format) {
  Date date;
  DateTimeParser parse(VariantTypes::Date, DateTimeParser::CONTEXT_FromString);
  // parse.SetDefaultLocale(Locale::CLocale());
  if (parse.ParseFormat(format)) {
    parse.FromString(string, &date, 0);
  }
  return date;
}


//
// Time
//

const Time Time::Null;
const Time Time::MinValue = Time::FromTicksStartOfDay(0);
const Time Time::MaxValue = Time::FromTicksStartOfDay(DateTimeConstants::TICKS_PER_DAY - 1);

uint32 HashOf(const Time& time) {
  return HashOf(time.ticks_in_day_);
}

Archive& operator & (Archive& ar, Time& time) {
  return ar & time.ticks_in_day_;
}

void Time::Swap(Time& rhs) {
  fun::Swap(ticks_in_day_, rhs.ticks_in_day_);
}

Time::Time(int32 hour, int32 minute, int32 second, int32 millisecond, int32 microsecond) {
  SetTime(hour, minute, second, millisecond, microsecond);
}

bool Time::IsValid() const {
  return ticks_in_day_ >= 0 && ticks_in_day_ < DateTimeConstants::TICKS_PER_DAY;
}

int32 Time::Hour() const {
  return IsValid() ? int32(ToTicksStartOfDay() / DateTimeConstants::TICKS_PER_HOUR) : -1;
}

int32 Time::Minute() const {
  return IsValid() ? int32((ToTicksStartOfDay() / DateTimeConstants::TICKS_PER_MINUTE) % 60) : -1;
}

int32 Time::Second() const {
  return IsValid() ? int32((ToTicksStartOfDay() / DateTimeConstants::TICKS_PER_SECOND) % 60) : -1;
}

int32 Time::Millisecond() const {
  return IsValid() ? int32((ToTicksStartOfDay() / DateTimeConstants::TICKS_PER_MILLISECOND) % 1000) : -1;
}

//@todo Microsecond까지 다루어야할까??
int32 Time::Microsecond() const {
  return IsValid() ? int32((ToTicksStartOfDay() / DateTimeConstants::TICKS_PER_MICROSECOND) % 1000) : -1;
}

int32 Time::HourAMPM() const {
  const int32 hour = Hour();

  if (hour < 1) {
    return 12;
  } else if (hour > 12) {
    return hour - 12;
  } else {
    return hour;
  }
}

bool Time::IsAM() const {
  const int32 hour = Hour();
  return hour < 12;
}

bool Time::IsPM() const {
  const int32 hour = Hour();
  return hour >= 12;
}

bool Time::SetTime(int32 hour, int32 minute, int32 second, int32 millisecond, int32 microsecond) {
  if (!IsValid(hour, minute, second, millisecond, microsecond)) {
    ticks_in_day_ = InvalidTime;
    return false;
  } else {
    ticks_in_day_ =
      hour * DateTimeConstants::TICKS_PER_HOUR +
      minute * DateTimeConstants::TICKS_PER_MINUTE +
      second * DateTimeConstants::TICKS_PER_SECOND +
      millisecond * DateTimeConstants::TICKS_PER_MILLISECOND +
      microsecond * DateTimeConstants::TICKS_PER_MICROSECOND;
    return true;
  }
}

Time Time::AddHours(double hours) const {
  return Time(*this).AddHoursInPlace(hours);
}

Time Time::AddMinutes(double minutes) const {
  return Time(*this).AddMinutesInPlace(minutes);
}

Time Time::AddSeconds(double seconds) const {
  return Time(*this).AddSecondsInPlace(seconds);
}

Time Time::AddMilliseconds(double milliseconds) const {
  return Time(*this).AddMillisecondsInPlace(milliseconds);
}

Time Time::AddMicroseconds(double microseconds) const {
  return Time(*this).AddMicrosecondsInPlace(microseconds);
}

Time Time::AddTicks(int64 ticks) const {
  return Time(*this).AddTicksInPlace(ticks);
}

Time& Time::AddHoursInPlace(double hours) {
  return AddTicksInPlace(int64(hours * DateTimeConstants::TICKS_PER_HOUR));
}

Time& Time::AddMinutesInPlace(double minutes) {
  return AddTicksInPlace(int64(minutes * DateTimeConstants::TICKS_PER_MINUTE));
}

Time& Time::AddSecondsInPlace(double seconds) {
  return AddTicksInPlace(int64(seconds * DateTimeConstants::TICKS_PER_SECOND));
}

Time& Time::AddMillisecondsInPlace(double milliseconds) {
  return AddTicksInPlace(int64(milliseconds * DateTimeConstants::TICKS_PER_MILLISECOND));
}

Time& Time::AddMicrosecondsInPlace(double microseconds) {
  return AddTicksInPlace(int64(microseconds * DateTimeConstants::TICKS_PER_MICROSECOND));
}

Time& Time::AddTicksInPlace(int64 ticks_to_add) {
  if (IsValid()) {
    if (ticks_to_add < 0) {
      const int32 neg_days = (DateTimeConstants::TICKS_PER_DAY - ticks_to_add) / DateTimeConstants::TICKS_PER_DAY;
      ticks_in_day_ = (ticks_in_day_ + ticks_to_add + neg_days * DateTimeConstants::TICKS_PER_DAY) % DateTimeConstants::TICKS_PER_DAY;
    } else {
      ticks_in_day_ = (ticks_in_day_ + ticks_to_add) % DateTimeConstants::TICKS_PER_DAY;
    }
  }
  return *this;
}

double Time::HoursTo(const Time& to) const {
  return (double)TicksTo(to) / DateTimeConstants::TICKS_PER_HOUR;
}

double Time::MinutesTo(const Time& to) const {
  return (double)TicksTo(to) / DateTimeConstants::TICKS_PER_MINUTE;
}

double Time::SecondsTo(const Time& to) const {
  return (double)TicksTo(to) / DateTimeConstants::TICKS_PER_SECOND;
}

double Time::MillisecondsTo(const Time& to) const {
  return (double)TicksTo(to) / DateTimeConstants::TICKS_PER_MILLISECOND;
}

double Time::MicrosecondsTo(const Time& to) const {
  return (double)TicksTo(to) / DateTimeConstants::TICKS_PER_MICROSECOND;
}

int64 Time::TicksTo(const Time& to) const {
  return to.ToTicksStartOfDay() - ToTicksStartOfDay();
}

//todo MICROSECOND?
String Time::ToString(DateFormatType format) const {
  if (!IsValid()) {
    return String(AsciiString("(null)"));
  }

  switch (format) {
    case DateFormatType::SystemLocaleShortDate:
      return UNICHAR_TO_UTF8(Locale::System().ToString(*this, Locale::ShortFormat).c_str());

    case DateFormatType::SystemLcaleLongDate:
      return UNICHAR_TO_UTF8(Locale::System().ToString(*this, Locale::LongFormat).c_str());

    case DateFormatType::DefaultLocaleShortDate:
      return UNICHAR_TO_UTF8(Locale().ToString(*this, Locale::ShortFormat).c_str());

    case DateFormatType::DefaultLocaleLongDate:
      return UNICHAR_TO_UTF8(Locale().ToString(*this, Locale::LongFormat).c_str());

    case DateFormatType::ISODateWithMs:
      return String::Format("%02d:%02d:%02d.%03d", Hour(), Minute(), Second(), Millisecond());

    case DateFormatType::RFC2822Date:
    case DateFormatType::ISODate:
    case DateFormatType::TextDate:
    default:
      return String::Format("%02d:%02d:%02d", Hour(), Minute(), Second());
  }
}

//todo MICROSECOND?
String Time::ToString(const String& format) const {
  return UNICHAR_TO_UTF8(Locale::System().ToString(*this, UString::FromUtf8(format)).c_str());
}

//todo MICROSECOND?
static Time FromIsoTimeString(const String& string, DateFormatType format, bool* is_midnight24) {
  if (is_midnight24) {
    *is_midnight24 = false;
  }

  const int32 len = string.Len();
  if (len < 5) {
    return Time::Null;
  }

  bool ok = false;
  int32 hour = string.Mid(0, 2).ToInt32(&ok);
  if (!ok) {
    return Time::Null;
  }

  const int32 minute = string.Mid(3, 2).ToInt32(&ok);
  if (!ok) {
    return Time::Null;
  }

  int32 second = 0;
  int32 tick = 0;

  if (len == 5) {
    // hh:mm Format
    second = 0;
    tick = 0;
  } else if (string[5] == ',' || string[5] == '.') {
    if (format == DateFormatType::TextDate) {
      return Time::Null;
    }

    // ISODate hh:mm.ssssss format
    // We only want 5 digits worth of fraction of Minute. This follows the existing
    // behavior that determines how milliseconds are read; 4 millisecond digits are
    // read and then rounded to 3. If we read at most 5 digits for fraction of Minute,
    // the maximum amount of millisecond digits it will expand to once converted to
    // seconds is 4. E.g. 12:34,99999 will expand to 12:34:59.9994. The milliseconds
    // will then be rounded up AND clamped to 999.

    const String minute_fraction_str = string.Mid(6, 5);
    const int32 minute_fraction_int = minute_fraction_str.ToInt32(&ok);
    if (!ok) {
      return Time::Null;
    }

    const float minute_fraction = double(minute_fraction_int) / (std::pow(double(10), minute_fraction_str.Len()));

    const float second_with_ms = minute_fraction * 60;
    const float second_no_ms = std::floor(second_with_ms);
    const float second_fraction = second_with_ms - second_no_ms;
    second = second_no_ms;
    tick = MathBase::Min(int32(second_fraction * 1000.0 + 0.5), 999);
  } else {
    // hh:mm:ss or hh:mm:ss.zzz
    second = string.Mid(6, 2).ToInt32(&ok);
    if (!ok) {
      return Time::Null;
    }
    if (len > 8 && (string[8] == ',' || string[8] == '.')) {
      const String msec_str(string.Mid(9, 4));
      int32 msec_int = msec_str.IsEmpty() ? 0 : msec_str.ToInt32(&ok);
      if (!ok) {
        return Time::Null;
      }
      const double second_fraction(msec_int / (std::pow(double(10), msec_str.Len())));
      tick = MathBase::Min(int32(second_fraction * 1000.0 + 0.5), 999);
    }
  }

  const bool is_iso_date = format == DateFormatType::ISODate || format == DateFormatType::ISODateWithMs;
  if (is_iso_date && hour == 24 && minute == 0 && second == 0 && tick == 0) {
    if (is_midnight24) {
      *is_midnight24 = true;
    }
    hour = 0;
  }

  return Time(hour, minute, second, tick);
}

Time Time::FromString(const String& string, DateFormatType format) {
  if (string.IsEmpty()) {
    return Time::Null;
  }

  switch (format) {
    case DateFormatType::SystemLocaleShortDate:
      return Locale::System().ToTime(UString::FromUtf8(string), Locale::ShortFormat);

    case DateFormatType::SystemLcaleLongDate:
      return Locale::System().ToTime(UString::FromUtf8(string), Locale::LongFormat);

    case DateFormatType::DefaultLocaleShortDate:
      return Locale().ToTime(UString::FromUtf8(string), Locale::ShortFormat);

    case DateFormatType::DefaultLocaleLongDate:
      return Locale().ToTime(UString::FromUtf8(string), Locale::LongFormat);

    case DateFormatType::RFC2822Date:
      return RfcDateImpl(string).time;

    case DateFormatType::ISODate:
    case DateFormatType::ISODateWithMs:
    case DateFormatType::TextDate:
    default:
      return FromIsoTimeString(string, format, nullptr);
  }
}

Time Time::FromString(const String& string, const String& format) {
  Time time;
  DateTimeParser parse(VariantTypes::Time, DateTimeParser::CONTEXT_FromString);
  // parse.SetDefaultLocale(Locale::AnsiCulture()); ### Qt 6
  if (parse.ParseFormat(format)) {
    parse.FromString(string, 0, &time);
  }

  return time;
}

bool Time::IsValid(int32 hour, int32 minute, int32 second, int32 millisecond, int32 microsecond) {
  return uint32(hour) < 24 && uint32(minute) < 60 && uint32(second) < 60 && uint32(millisecond) < 1000 && uint32(microsecond) < 1000;
}

void Time::Start() {
  *this = CurrentTime();
}

int64 Time::Restart() {
  const Time now = CurrentTime();
  int64 elapsed_ticks = MillisecondsTo(now);
  if (elapsed_ticks < 0) {
    elapsed_ticks += DateTimeConstants::TICKS_PER_DAY;
  }
  *this = now;
  return elapsed_ticks;
}

int64 Time::Elapsed() const {
  int64 elapsed_ticks = MillisecondsTo(CurrentTime());
  if (elapsed_ticks < 0) {
    elapsed_ticks += DateTimeConstants::TICKS_PER_DAY;
  }

  return elapsed_ticks;
}

Time Time::CurrentTime() {
  return DateTime::Now().GetTime();
}

Time Time::UtcCurrentTime() {
  return DateTime::UtcNow().GetTime();
}


//
// DateTime
//

DateTime::Data::Data(TimeSpec spec)
  : ticks(0),
    status((uint8)spec << TimeSpecShift),
    offset_from_utc(0),
    timezone(new TimeZone()) {}

DateTime::Data::~Data() {
  delete timezone;
}

DateTime::Data::Data(const Data& rhs)
  : ticks(rhs.ticks),
    status(rhs.status),
    offset_from_utc(rhs.offset_from_utc),
    timezone(new TimeZone(*rhs.timezone)) {
}

DateTime::Data& DateTime::Data::operator = (const DateTime::Data& rhs) {
  ticks = rhs.ticks;
  status = rhs.status;
  offset_from_utc = rhs.offset_from_utc;
  *timezone = *rhs.timezone;
  return *this;
}


const DateTime DateTime::None;
const DateTime DateTime::Null;
const DateTime DateTime::MinValue = DateTime::FromUtcTicks(0);                       // 1-1-1 00:00:00.000
const DateTime DateTime::MaxValue = DateTime::FromUtcTicks(185542587100799999LL);    // 5879611-7-11 23:59:59.999

DateTime::DateTime() {
  fun_check(GetTimeSpec() == TimeSpec::Local);
}

DateTime::DateTime(const Date& date) {
  SetDateAndTime(data_, date, Time::Null);
  fun_check(GetTimeSpec() == TimeSpec::Local);
}

DateTime::DateTime(const Date& date, const Time& time, TimeSpec spec) {
  SetTimeSpec_helper(data_, spec, 0);
  SetDateAndTime(data_, date, time);
}

DateTime::DateTime(const Date& date, const Time& time, TimeSpec spec, int32 offset_seconds) {
  SetTimeSpec_helper(data_, spec, offset_seconds);
  SetDateAndTime(data_, date, time);
}

DateTime::DateTime(const Date& date, const Time& time, const TimeZone& timezone) {
  //SetTimeSpec_helper(data, TimeSpec::TimeZone, 0);
  data_.status = MergeSpec(data_.status, TimeSpec::TimeZone);
  *data_.timezone = timezone;
  SetDateAndTime(data_, date, time);
}

DateTime::DateTime(const Timestamp& ts) {
  //TODO
  fun_check(0);
}

DateTime::DateTime(const DateTime& rhs)
  : data_(rhs.data_) {}

DateTime::DateTime(DateTime&& rhs)
  : data_(MoveTemp(rhs.data_)) {}

DateTime& DateTime::operator = (const DateTime& rhs) {
  data_ = rhs.data_;
  return *this;
}

DateTime& DateTime::operator = (DateTime&& rhs) {
  data_ = MoveTemp(rhs.data_);
  return *this;
}

void DateTime::Swap(DateTime& rhs) {
  fun::Swap(data_.ticks, rhs.data_.ticks);
  fun::Swap(data_.status, rhs.data_.status);
  fun::Swap(data_.offset_from_utc, rhs.data_.offset_from_utc);
  fun::Swap(*data_.timezone, *rhs.data_.timezone);
}

bool DateTime::IsNull() const {
  return (data_.status & (ValidDate | ValidTime)) == 0;
}

bool DateTime::IsValid() const {
  return !!(data_.status & ValidDateTime);
}

int32 DateTime::Year() const {
  return GetDate().Year();
}

int32 DateTime::Month() const {
  return GetDate().Month();
}

int32 DateTime::Day() const {
  return GetDate().Day();
}

DayOfWeekType DateTime::DayOfWeek() const {
  return GetDate().DayOfWeek();
}

int32 DateTime::DayOfYear() const {
  return GetDate().DayOfYear();
}

int32 DateTime::DaysInMonth() const {
  return GetDate().DaysInMonth();
}

int32 DateTime::DaysInYear() const {
  return GetDate().DaysInYear();
}

int32 DateTime::Hour() const {
  return int32((data_.ticks / DateTimeConstants::TICKS_PER_HOUR) % 24);
}

int32 DateTime::Minute() const {
  return int32((data_.ticks / DateTimeConstants::TICKS_PER_MINUTE) % 60);
}

int32 DateTime::Second() const {
  return int32((data_.ticks / DateTimeConstants::TICKS_PER_SECOND) % 60);
}

int32 DateTime::Millisecond() const {
  return int32((data_.ticks / DateTimeConstants::TICKS_PER_MILLISECOND) % 1000);
}

int32 DateTime::Microsecond() const {
  return int32(((data_.ticks % DateTimeConstants::TICKS_PER_MILLISECOND) / 1000) % 1000);
}

int32 DateTime::HourAMPM() const {
  const int32 hour = Hour();

  if (hour < 1) {
    return 12;
  } else if (hour > 12) {
    return hour - 12;
  } else {
    return hour;
  }
}

bool DateTime::IsAM() const {
  const int32 hour = Hour();
  return hour < 12;
}

bool DateTime::IsPM() const {
  const int32 hour = Hour();
  return hour >= 12;
}

Date DateTime::GetDate() const {
  if (!(data_.status & ValidDate)) {
    return Date::Null;
  }

  Date date_part;
  TicksToDateTime(data_.ticks, &date_part, nullptr);
  return date_part;
}

Time DateTime::GetTime() const {
  if (!(data_.status & ValidTime)) {
    return Time::Null;
  }

  Time time_part;
  TicksToDateTime(data_.ticks, nullptr, &time_part);
  return time_part;
}

TimeSpec DateTime::GetTimeSpec() const {
  return GetSpec(data_.status);
}

int32 DateTime::GetOffsetFromUtc() const {
  return data_.offset_from_utc;

  //if (!IsValid()) {
  //  return 0;
  //}
  //
  //const auto spec = GetSpec(data_.status);
  //
  //if (spec == TimeSpec::OffsetFromUTC || spec == TimeSpec::TimeZone) { //TODO TimeZone일 경우에 OffsetFromUTC가 설정되는지??
  //  return data_.offset_from_utc;
  //} else if (spec == TimeSpec::Local) {
  //  // We didn't cache the value, so we need to calculate it now.
  //  return (data_.ticks - ToUtcTicks()) / DateTimeConstants::TICKS_PER_SECOND;
  //} else {
  //  fun_check(spec == TimeSpec::UTC);
  //  return 0;
  //}
}

TimeZone DateTime::GetTimeZone() const {
  const TimeSpec spec = GetSpec(data_.status);
  switch (spec) {
    case TimeSpec::UTC:
      return TimeZone::Utc();

    case TimeSpec::OffsetFromUTC:
      return TimeZone(Timespan::FromSeconds(data_.offset_from_utc));

    case TimeSpec::TimeZone:
      fun_check(data_.timezone->IsValid());
      return *data_.timezone;

    case TimeSpec::Local:
      return TimeZone::Local();
  }

  // unreachable to here.
  return TimeZone::Local();
}

String DateTime::GetTimeZoneAbbreviation() const {
  const TimeSpec spec = GetSpec(data_.status);
  switch (spec) {
    case TimeSpec::UTC:
      return AsciiString("UTC");

    case TimeSpec::OffsetFromUTC:
      return AsciiString("UTC") + ToOffsetString(DateFormatType::ISODate, data_.offset_from_utc);

    case TimeSpec::TimeZone:
      return data_.timezone->impl_->GetAbbreviation(ToUtcTicksSinceEpoch() / DateTimeConstants::TICKS_PER_MILLISECOND);

    case TimeSpec::Local: {
      String abbreviation;
      auto daylight_status = ExtractDaylightStatus(data_.status);
      LocalToUtcTicks(data_.ticks, &daylight_status, 0, 0, &abbreviation);
      return abbreviation;
    }
  }

  // unreachable to here.
  return String();
}

bool DateTime::IsDaylightTime() const {
  const TimeSpec spec = GetSpec(data_.status);
  switch (spec) {
    case TimeSpec::UTC:
    case TimeSpec::OffsetFromUTC:
      return false;

    case TimeSpec::TimeZone:
      return data_.timezone->impl_->IsDaylightTime(ToUtcTicksSinceEpoch() / DateTimeConstants::TICKS_PER_MILLISECOND);

    case TimeSpec::Local: {
      auto status = ExtractDaylightStatus(data_.status);
      if (status == DaylightStatus::UnknownDaylightTime) {
        LocalToUtcTicks(data_.ticks, &status);
      }
      return status == DaylightStatus::DaylightTime;
    }
  }

  return false;
}

int64 DateTime::ToUtcTicks() const {
  if (!IsValid()) {
    //fun_check(data_.ticks == 0);
    return 0;
  }

  const TimeSpec spec = GetSpec(data_.status);
  switch (spec) {
    case TimeSpec::UTC:
      return data_.ticks;

    case TimeSpec::OffsetFromUTC:
      return data_.ticks - (data_.offset_from_utc * DateTimeConstants::TICKS_PER_SECOND);

    case TimeSpec::Local: {
      auto status = ExtractDaylightStatus(data_.status);
      return LocalToUtcTicks(data_.ticks, &status);
    }

    case TimeSpec::TimeZone:
      return ZoneToUtcTicks(data_.ticks, *data_.timezone, ExtractDaylightStatus(data_.status));
  }

  // unrechable to here.
  fun_unexpected();
  return 0;
}

int64 DateTime::ToUtcTicksSinceEpoch() const {
  return ToUtcTicks() - DateTimeConstants::EPOCH_TICKS;
}

void DateTime::SetDateOnly(const Date& date) {
  SetDateAndTime(data_, date, GetTime());
}

void DateTime::SetTimeOnly(const Time& time) {
  SetDateAndTime(data_, GetDate(), time);
}

void DateTime::SetTimeSpec(TimeSpec spec) {
  SetTimeSpec_helper(data_, spec, 0);
  CheckValidDateTime(data_);
}

void DateTime::SetOffsetFromUtc(int32 offset_seconds) {
  SetTimeSpec_helper(data_, TimeSpec::OffsetFromUTC, offset_seconds);
  CheckValidDateTime(data_);
}

void DateTime::SetTimeZone(const TimeZone& timezone) {
  data_.status = MergeSpec(data_.status, TimeSpec::TimeZone);
  data_.offset_from_utc = 0;
  *data_.timezone = timezone;
  RefreshDateTime(data_);
}

void DateTime::SetUtcTicks(int64 utc_ticks) {
  const TimeSpec spec = GetSpec(data_.status);
  uint8 status = data_.status;

  status &= ~ValidityMask;

  switch (spec) {
    case TimeSpec::UTC:
      status |= (ValidDate | ValidTime | ValidDateTime);
      break;

    case TimeSpec::OffsetFromUTC:
      status |= (ValidDate | ValidTime | ValidDateTime);
      utc_ticks += (data_.offset_from_utc * DateTimeConstants::TICKS_PER_SECOND);
      break;

    case TimeSpec::TimeZone: {
      // Docs state any local_time before 1970-01-01 will *not* have any DST applied
      // but all affected times afterwards will have DST applied.
      const int64 utc_ticks_since_epoch = utc_ticks - DateTimeConstants::EPOCH_TICKS;
      if (utc_ticks_since_epoch >= 0) {
        status = MergeDaylightStatus(status, data_.timezone->impl_->IsDaylightTime(utc_ticks_since_epoch / DateTimeConstants::TICKS_PER_MILLISECOND) ? DaylightStatus::DaylightTime : DaylightStatus::StandardTime);
        data_.offset_from_utc = data_.timezone->impl_->GetOffsetFromUtc(utc_ticks_since_epoch / DateTimeConstants::TICKS_PER_MILLISECOND);
      } else {
        status = MergeDaylightStatus(status, DaylightStatus::StandardTime);
        data_.offset_from_utc = data_.timezone->impl_->GetStandardTimeOffset(utc_ticks_since_epoch / DateTimeConstants::TICKS_PER_MILLISECOND);
      }
      utc_ticks += (data_.offset_from_utc * DateTimeConstants::TICKS_PER_SECOND);
      status |= ValidDate | ValidTime | ValidDateTime; // 일단은 유효하다고 설정해두고, RefreshDateTime() 함수내에서 실제 유효한지 여부를 갱신.
      break;
    }

    case TimeSpec::Local: {
      Date date;
      Time time;
      DaylightStatus daylight;
      UtcTicksToLocalDateTime(utc_ticks, &date, &time, &daylight);

      SetDateAndTime(data_, date, time);

      utc_ticks = data_.ticks; // assign converted msecs.
      status = MergeDaylightStatus(data_.status, daylight);
      break;
    }
  }

  data_.ticks = utc_ticks;
  data_.status = status;

  if (spec == TimeSpec::Local || spec == TimeSpec::TimeZone) {
    RefreshDateTime(data_);
  }
}

void DateTime::SetUtcTicksSinceEpoch(int64 utc_ticks_since_epoch) {
  SetUtcTicks(utc_ticks_since_epoch + DateTimeConstants::EPOCH_TICKS);
}

//TODO fraction.
String DateTime::ToString(DateFormatType format) const {
  if (!IsValid()) {
    return AsciiString("(null)");
  }

  String buf;

  switch (format) {
    case DateFormatType::SystemLocaleShortDate:
      return UNICHAR_TO_UTF8(Locale::System().ToString(*this, Locale::ShortFormat).c_str());

    case DateFormatType::SystemLcaleLongDate:
      return UNICHAR_TO_UTF8(Locale::System().ToString(*this, Locale::LongFormat).c_str());

    case DateFormatType::DefaultLocaleShortDate:
      return UNICHAR_TO_UTF8(Locale().ToString(*this, Locale::ShortFormat).c_str());

    case DateFormatType::DefaultLocaleLongDate:
      return UNICHAR_TO_UTF8(Locale().ToString(*this, Locale::LongFormat).c_str());

    case DateFormatType::RFC2822Date: {
      buf = UNICHAR_TO_UTF8(Locale::CLocale().ToString(*this, AsciiString("dd MMM yyyy hh:mm:ss ")).c_str());
      buf += ToOffsetString(DateFormatType::TextDate, GetOffsetFromUtc());
      return buf;
    }

    default:
    case DateFormatType::TextDate: {
      const Date date_part = this->GetDate();
      const Time time_part = this->GetTime();

      buf = date_part.ToString(DateFormatType::TextDate);
      // Insert time between date's day and year:
      buf.Insert(buf.LastIndexOf(' '), ' ' + time_part.ToString(DateFormatType::TextDate));

      // Append zone/offset indicator, as appropriate:
      const TimeSpec spec = GetSpec(data_.status);
      switch (spec) {
        case TimeSpec::Local:
          break;

        case TimeSpec::TimeZone:
          buf += ' ' + data_.timezone->GetAbbreviation(*this);
          break;

        default:
          buf += AsciiString(" GMT");
          if (spec == TimeSpec::OffsetFromUTC) {
            buf += ToOffsetString(DateFormatType::TextDate, GetOffsetFromUtc());
          }
      }
      return buf;
    }

    case DateFormatType::ISODate:
    case DateFormatType::ISODateWithMs: {
      const Date date_part = this->GetDate();
      const Time time_part = this->GetTime();

      buf = date_part.ToString(DateFormatType::ISODate);
      if (buf.IsEmpty()) {
        return String(); // failed to convert
      }

      buf += 'T';
      buf += time_part.ToString(format);

      const TimeSpec spec = GetSpec(data_.status);
      switch (spec) {
      case TimeSpec::UTC:
        buf += 'Z';
        break;

      case TimeSpec::OffsetFromUTC:
      case TimeSpec::TimeZone:
        buf += ToOffsetString(DateFormatType::ISODate, GetOffsetFromUtc());
        break;

      default:
        break;
      }
      return buf;
    }
  }
}

String DateTime::ToString(const String& format) const {
  return UNICHAR_TO_UTF8(Locale::System().ToString(*this, UString::FromUtf8(format)).c_str());
  //return UNICHAR_TO_UTF8(Locale::CLocale().ToString(*this, UString::FromUtf8(format)).c_str());
}

DateTime DateTime::AddDays(int64 days) const {
  return DateTime(*this).AddDaysInPlace(days);
}

DateTime DateTime::AddMonths(int32 months) const {
  return DateTime(*this).AddMonthsInPlace(months);
}

DateTime DateTime::AddYears(int32 years) const {
  return DateTime(*this).AddYearsInPlace(years);
}

DateTime DateTime::AddHours(int64 hours) const {
  return DateTime(*this).AddHoursInPlace(hours);
}

DateTime DateTime::AddMinutes(int64 minutes) const {
  return DateTime(*this).AddMinutesInPlace(minutes);
}

DateTime DateTime::AddSeconds(int64 seconds) const {
  return DateTime(*this).AddSecondsInPlace(seconds);
}

DateTime DateTime::AddMilliseconds(int64 millisecond) const {
  return DateTime(*this).AddMillisecondsInPlace(millisecond);
}

DateTime DateTime::AddMicroseconds(int64 microseconds) const {
  return DateTime(*this).AddMicrosecondsInPlace(microseconds);
}

DateTime DateTime::AddTicks(int64 ticks) const {
  return DateTime(*this).AddTicksInPlace(ticks);
}

DateTime& DateTime::AddDaysInPlace(int64 days) {
  return AddMillisecondsInPlace(days * DateTimeConstants::TICKS_PER_DAY);
}

DateTime& DateTime::AddMonthsInPlace(int32 months) {
  if (months < -120000 || months > 120000) {
    return *this;
  }

  if (!IsValid()) {
    return *this;
  }

  const Date date_part = GetDate();
  int32 y = date_part.Year();
  int32 m = date_part.Month();
  int32 d = date_part.Day();
  int32 i = m - 1 + months;
  if (i >= 0) {
    m = (i % 12) + 1;
    y = y + (i / 12);
  } else {
    m = 12 + (i + 1) % 12;
    y = y + (i - 11) / 12;
  }

  if (y < 1 || y > 9999) {
    return *this;
  }

  const int32 days = Date::DaysInMonth(y, m);
  if (d > days) {
    d = days;
  }

  data_.ticks = DateToDays(y, m, d) * DateTimeConstants::TICKS_PER_DAY + (data_.ticks % DateTimeConstants::TICKS_PER_DAY);
  return *this;
}

DateTime& DateTime::AddYearsInPlace(int32 years) {
  return AddMonthsInPlace(years * 12);
}

DateTime& DateTime::AddHoursInPlace(int64 hours) {
  return AddTicksInPlace(hours * DateTimeConstants::TICKS_PER_HOUR);
}

DateTime& DateTime::AddMinutesInPlace(int64 minutes) {
  return AddTicksInPlace(minutes * DateTimeConstants::TICKS_PER_MINUTE);
}

DateTime& DateTime::AddSecondsInPlace(int64 seconds) {
  return AddTicksInPlace(seconds * DateTimeConstants::TICKS_PER_SECOND);
}

DateTime& DateTime::AddMillisecondsInPlace(int64 milliseconds) {
  return AddTicksInPlace(milliseconds * DateTimeConstants::TICKS_PER_MILLISECOND);
}

DateTime& DateTime::AddMicrosecondsInPlace(int64 microseconds) {
  return AddTicksInPlace(microseconds * DateTimeConstants::TICKS_PER_MICROSECOND);
}

DateTime& DateTime::AddTicksInPlace(int64 ticks_to_add) {
  if (IsValid()) {
    const TimeSpec spec = GetSpec(data_.status);
    if (spec == TimeSpec::Local || spec == TimeSpec::TimeZone) {
      // Convert to real UTC first in case crosses DST transition
      SetUtcTicks(ToUtcTicks() + ticks_to_add);
    } else {
      //data_.ticks += ticks_to_add;
      SetUtcTicks(data_.ticks + ticks_to_add);
    }
  }
  return *this;
}

DateTime DateTime::ToTimeSpec(TimeSpec spec) const {
  if (GetSpec(data_.status) == spec && (spec == TimeSpec::UTC || spec == TimeSpec::Local)) {
    return *this;
  }

  // If this datetime is not a valid datetime, it will be returned to
  // the copy according to spec.
  // (Even if the value is not valid, spec will be tailored to the request.)
  if (!IsValid()) {
    DateTime result(*this);
    result.SetTimeSpec(spec);
    return result;
  }

  return FromUtcTicks(ToUtcTicks(), spec, 0);
}

DateTime DateTime::ToLocalTime() const {
  return ToTimeSpec(TimeSpec::Local);
}

DateTime DateTime::ToUniversalTime() const {
  return ToTimeSpec(TimeSpec::UTC);
}

DateTime DateTime::ToOffsetFromUTC(int32 offset_seconds) const {
  if (GetSpec(data_.status) == TimeSpec::OffsetFromUTC && data_.offset_from_utc == offset_seconds) {
    // Since no conversion is needed, we can not accept that.
    return *this;
  }

  // If this datetime is not a valid datetime, it will be returned to
  // the copy according to spec.
  // (Even if the value is not valid, spec will be tailored to the request.)
  if (!IsValid()) {
    DateTime result(*this);
    result.SetOffsetFromUtc(offset_seconds);
    return result;
  }

  return FromUtcTicks(ToUtcTicks(), TimeSpec::OffsetFromUTC, offset_seconds);
}

DateTime DateTime::ToTimeZone(const TimeZone& to_time_zone) const {
  if (GetSpec(data_.status) == TimeSpec::TimeZone && *data_.timezone == to_time_zone) {
    // Since no conversion is needed, we can not accept that.
    return *this;
  }

  // If this datetime is not a valid datetime, it will be returned to
  // the copy according to spec.
  // (Even if the value is not valid, spec will be tailored to the request.)
  if (!IsValid()) {
    DateTime result(*this);
    result.SetTimeZone(to_time_zone);
    return result;
  }

  return FromUtcTicks(ToUtcTicks(), to_time_zone);
}

// If the value of this or to is not valid, it is calculated as 0.
double DateTime::DaysTo(const DateTime& to) const {
  return double(TicksTo(to)) / DateTimeConstants::TICKS_PER_DAY;
}

// If the value of this or to is not valid, it is calculated as 0.
double DateTime::HoursTo(const DateTime& to) const {
  return double(TicksTo(to)) / DateTimeConstants::TICKS_PER_HOUR;
}

// If the value of this or to is not valid, it is calculated as 0.
double DateTime::SecondsTo(const DateTime& to) const {
  return double(TicksTo(to)) / DateTimeConstants::TICKS_PER_SECOND;
}

// If the value of this or to is not valid, it is calculated as 0.
double DateTime::MillisecondTo(const DateTime& to) const {
  return double(TicksTo(to)) / DateTimeConstants::TICKS_PER_MILLISECOND;
}

// If the value of this or to is not valid, it is calculated as 0.
double DateTime::MicrosecondsTo(const DateTime& to) const {
  return double(TicksTo(to)) / DateTimeConstants::TICKS_PER_MICROSECOND;
}

// If the value of this or to is not valid, it is calculated as 0.
int64 DateTime::TicksTo(const DateTime& to) const {
  return to.ToUtcTicks() - ToUtcTicks();
}

int32 DateTime::Compare(const DateTime& other) const {
  if (GetSpec(data_.status) == TimeSpec::Local &&
      GetSpec(data_.status) == GetSpec(other.data_.status)) {
    return data_.ticks == other.data_.ticks ? 0 : (data_.ticks < other.data_.ticks ? -1 : +1);
  }

  const int64 this_utc_ticks = ToUtcTicks();
  const int64 other_utc_ticks = other.ToUtcTicks();
  return this_utc_ticks == other_utc_ticks ? 0 : (this_utc_ticks < other_utc_ticks ? -1 : +1);
}

bool operator == (const DateTime& lhs, const DateTime& rhs) {
  return lhs.Compare(rhs) == 0;
}

bool operator != (const DateTime& lhs, const DateTime& rhs) {
  return lhs.Compare(rhs) != 0;
}

bool operator < (const DateTime& lhs, const DateTime& rhs) {
  return lhs.Compare(rhs) < 0;
}

bool operator <= (const DateTime& lhs, const DateTime& rhs) {
  return lhs.Compare(rhs) <= 0;
}

bool operator > (const DateTime& lhs, const DateTime& rhs) {
  return lhs.Compare(rhs) > 0;
}

bool operator >= (const DateTime& lhs, const DateTime& rhs) {
  return lhs.Compare(rhs) >= 0;
}

DateTime DateTime::Now() {
  //TODO UTC를 가지고 로컬로 변환해서 반환하는 형태로 처리..
  int32 year, month, dayofweek, day;
  int32 hour, minute, second, millisecond;
  SystemTime::GetSystemTime(year, month, dayofweek, day, hour, minute, second, millisecond);
  return DateTime(Date(year, month, day), Time(hour, minute, second, millisecond), TimeSpec::Local);
}

DateTime DateTime::UtcNow() {
  //TODO FILETIME에서 가져와서 처리하는 걸로...
  int32 year, month, dayofweek, day;
  int32 hour, minute, second, millisecond;
  SystemTime::GetUtcTime(year, month, dayofweek, day, hour, minute, second, millisecond);
  return DateTime(Date(year, month, day), Time(hour, minute, second, millisecond), TimeSpec::UTC);
}

DateTime DateTime::FromString(const String& string, DateFormatType format) {
  if (string.IsEmpty()) {
    return DateTime::Null;
  }

  switch (format) {
    case DateFormatType::SystemLocaleShortDate:
      return Locale::System().ToDateTime(UString::FromUtf8(string), Locale::ShortFormat);

    case DateFormatType::SystemLcaleLongDate:
      return Locale::System().ToDateTime(UString::FromUtf8(string), Locale::LongFormat);

    case DateFormatType::DefaultLocaleShortDate:
      return Locale().ToDateTime(UString::FromUtf8(string), Locale::ShortFormat);

    case DateFormatType::DefaultLocaleLongDate:
      return Locale().ToDateTime(UString::FromUtf8(string), Locale::LongFormat);

    case DateFormatType::RFC2822Date: {
      const ParsedRfcDateTime rfc = RfcDateImpl(string);

      // If both date and time are not valid, DateTime::Null is returned.
      if (!rfc.date.IsValid() || !rfc.time.IsValid()) {
        return DateTime::Null;
      }

      DateTime dt(rfc.date, rfc.time, TimeSpec::UTC);
      dt.SetOffsetFromUtc(rfc.utc_offset);
      return dt;
    }

    case DateFormatType::ISODate:
    case DateFormatType::ISODateWithMs: {
      const int32 len = string.Len();
      if (len < 10) {
        return DateTime::Null;
      }

      String iso_string(string);
      TimeSpec spec = TimeSpec::Local;

      Date date = Date::FromString(string.Left(10), DateFormatType::ISODate);
      if (!date.IsValid()) {
        return DateTime::Null;
      }

      if (len == 10) {
        return DateTime(date); // date only
      }

      iso_string = iso_string.Right(iso_string.Len() - 11);
      int32 offset_seconds = 0;
      // Check end of string for time zone definition, either Z for UTC or [+-]hh:mm for offset
      if (iso_string.EndsWith('Z')) {
        spec = TimeSpec::UTC;
        iso_string = iso_string.Left(iso_string.Len() - 1);
      }
      else {
        int32 sign_index = iso_string.Len() - 1;
        bool found = false; {
          const char plus = '+';
          const char minus = '-';
          do {
            const char character(iso_string[sign_index]);
            found = character == plus || character == minus;
          } while (--sign_index >= 0 && !found);
          ++sign_index;
        }

        if (found) {
          bool ok;
          offset_seconds = FromOffsetString(iso_string.Mid(sign_index), &ok);
          if (!ok) {
            return DateTime::Null;
          }
          iso_string = iso_string.Left(sign_index);
          spec = TimeSpec::OffsetFromUTC;
        }
      }

      // Might be end of Day (24:00, including variants), which Time considers invalid.
      // ISO 8601 (section 4.2.3) says that 24:00 is equivalent to 00:00 the next day.
      bool is_midnight24 = false;
      const Time time = FromIsoTimeString(iso_string, format, &is_midnight24);
      if (!time.IsValid()) {
        return DateTime::Null;
      }
      if (is_midnight24) {
        date = date.AddDays(1);
      }
      return DateTime(date, time, spec, offset_seconds);
    }

    case DateFormatType::TextDate: {
      const Array<String> parts = string.Split(' ', 0, StringSplitOption::CullEmpty);

      if ((parts.Count() < 5) || (parts.Count() > 6)) {
        return DateTime::Null;
      }

      // Accept "Sun Dec 1 13:02:00 1974" and "Sun 1. Dec 13:02:00 1974"
      int32 month = 0;
      int32 day = 0;
      bool ok = false;

      // First try month then day
      month = FromShortMonthName(parts[1]);
      if (month) {
        day = parts[2].ToInt32();
      }

      // If failed try day then month
      if (!month || !day) {
        month = FromShortMonthName(parts[2]);
        if (month) {
          String day_str = parts[1];
          if (day_str.EndsWith('.')) {
            day_str = day_str.Left(day_str.Len() - 1);
            day = day_str.ToInt32();
          }
        }
      }

      // If both failed, give up
      if (!month || !day) {
        return DateTime::Null;
      }

      // Year can be before or after time, "Sun Dec 1 1974 13:02:00" or "Sun Dec 1 13:02:00 1974"
      // Guess which by looking for ':' in the time
      int32 year = 0;
      int32 year_part = 0;
      int32 time_part = 0;
      if (parts[3].Contains(':')) {
        year_part = 4;
        time_part = 3;
      } else if (parts[4].Contains(':')) {
        year_part = 3;
        time_part = 4;
      } else {
        return DateTime::Null;
      }

      year = parts[year_part].ToInt32(&ok);
      if (!ok) {
        return DateTime::Null;
      }

      Date date(year, month, day);
      if (!date.IsValid()) {
        return DateTime::Null;
      }

      const Array<String> time_parts = parts[time_part].Split(':');
      if (time_parts.Count() < 2 || time_parts.Count() > 3) {
        return DateTime::Null;
      }

      const int32 hour = time_parts[0].ToInt32(&ok);
      if (!ok) {
        return DateTime::Null;
      }

      const int32 minute = time_parts[1].ToInt32(&ok);
      if (!ok) {
        return DateTime::Null;
      }

      int32 second = 0;
      int32 millisecond = 0;
      if (time_parts.Count() > 2) {
        const Array<String> second_parts = time_parts[2].Split('.');
        if (second_parts.Count() > 2) {
          return DateTime::Null;
        }

        second = second_parts.First().ToInt32(&ok);
        if (!ok) {
          return DateTime::Null;
        }

        if (second_parts.Count() > 1) {
          millisecond = second_parts.Last().ToInt32(&ok);
          if (!ok) {
            return DateTime::Null;
          }
        }
      }

      const Time time(hour, minute, second, millisecond);
      if (!time.IsValid()) {
        return DateTime::Null;
      }

      if (parts.Count() == 5) {
        return DateTime(date, time, TimeSpec::Local);
      }

      String tz = parts[5];
      if (!tz.StartsWith(AsciiString("GMT"), CaseSensitivity::IgnoreCase)) {
        return DateTime::Null;
      }
      tz = tz.Mid(3);
      if (!tz.IsEmpty()) {
        const int32 offset_seconds = FromOffsetString(tz, &ok);
        if (!ok) {
          return DateTime::Null;
        }
        return DateTime(date, time, TimeSpec::OffsetFromUTC, offset_seconds);
      } else {
        return DateTime(date, time, TimeSpec::UTC);
      }
    }
  }

  return DateTime::Null;
}

DateTime DateTime::FromString(const String& string, const String& format) {
  Date date;
  Time time;
  DateTimeParser parse(VariantTypes::DateTime, DateTimeParser::CONTEXT_FromString);
  // dt.SetDefaultLocale(Locale::CLocale()); ### Qt 6
  if (parse.ParseFormat(format) && parse.FromString(string, &date, &time)) {
    return DateTime(date, time);
  }
  return DateTime::Null;
}

DateTime DateTime::FromUtcTicks(int64 utc_ticks) {
  return FromUtcTicks(utc_ticks, TimeSpec::UTC);
}

DateTime DateTime::FromUtcTicks(int64 utc_ticks, TimeSpec spec, int32 offset_seconds) {
  DateTime result;
  SetTimeSpec_helper(result.data_, spec, offset_seconds);
  result.SetUtcTicks(utc_ticks);
  return result;
}

DateTime DateTime::FromUtcTicks(int64 utc_ticks, const TimeZone& timezone) {
  DateTime result;
  result.SetTimeZone(timezone);
  result.SetUtcTicks(utc_ticks);
  return result;
}

DateTime DateTime::FromUtcTicksSinceEpoch(int64 utc_ticks_since_epoch) {
  return FromUtcTicks(utc_ticks_since_epoch + DateTimeConstants::EPOCH_TICKS);
}

DateTime DateTime::FromUtcTicksSinceEpoch(int64 utc_ticks_since_epoch, TimeSpec spec, int32 offset_from_utc) {
  return FromUtcTicks(utc_ticks_since_epoch + DateTimeConstants::EPOCH_TICKS, spec, offset_from_utc);
}

DateTime DateTime::FromUtcTicksSinceEpoch(int64 utc_ticks_since_epoch, const TimeZone& timezone) {
  return FromUtcTicks(utc_ticks_since_epoch + DateTimeConstants::EPOCH_TICKS, timezone);
}

Archive& operator & (Archive& ar, DateTime& dt) {
  //다시 정리해볼 필요가 있음.
  return ar & dt.data_.ticks & dt.data_.status & dt.data_.offset_from_utc & *(dt.data_.timezone);
}

uint32 HashOf(const DateTime& dt) {
  return HashOf(dt.ToUtcTicks());
}

namespace Lex {
  String ToString(const Date& value) { return value.ToString(); }
  String ToString(const Time& value) { return value.ToString(); }
  String ToString(const DateTime& value) { return value.ToString(); }
}

} // namespace fun
