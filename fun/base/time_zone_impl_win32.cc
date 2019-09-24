#include "fun/base/time_zone.h"
#include "fun/base/time_zone_impl.h"
#include "fun/base/date_time.h"
#include "fun/base/locale/locale_private.h"

#include <algorithm>

namespace fun {

#ifndef Q_OS_WINRT
#define FUN_USE_REGISTRY_TIMEZONE 1
#endif

/*
  Private

  Windows system implementation
*/

#define MAX_KEY_LENGTH 255
#define FILETIME_UNIX_EPOCH Q_UINT64_C(116444736000000000)

// MSDN home page for Time support
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724962%28v=vs.85%29.aspx

// For Windows XP and later refer to MSDN docs on TIME_ZONE_INFORMATION structure
// http://msdn.microsoft.com/en-gb/library/windows/desktop/ms725481%28v=vs.85%29.aspx

// Vista introduced support for historic data, see MSDN docs on DYNAMIC_TIME_ZONE_INFORMATION
// http://msdn.microsoft.com/en-gb/library/windows/desktop/ms724253%28v=vs.85%29.aspx
#ifdef FUN_USE_REGISTRY_TIMEZONE
static const char TZ_REG_PATH[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones";
static const char CURRENT_TZ_REG_PATH[] = "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation";
#endif

enum {
  MIN_YEAR = -292275056,
  MAX_YEAR = 292278994,
  MSECS_PER_DAY = 86400000,
  TIME_T_MAX = 2145916799,  // int maximum 2037-12-31T23:59:59 UTC
  JULIAN_DAY_FOR_EPOCH = 2440588 // result of julianDayFromDate(1970, 1, 1)
};

// Copied from MSDN, see above for link
typedef struct _REG_TZI_FORMAT {
  LONG Bias;
  LONG StandardBias;
  LONG DaylightBias;
  SYSTEMTIME StandardDate;
  SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT;

namespace {

// Fast and reliable conversion from msecs to date for all values
// Adapted from QDateTime MSecsToDate
Date MSecsToDate(int64 msecs) {
  int64 jd = JULIAN_DAY_FOR_EPOCH;

  if (MathBase::Abs(msecs) >= MSECS_PER_DAY) {
    jd += (msecs / MSECS_PER_DAY);
    msecs %= MSECS_PER_DAY;
  }

  if (msecs < 0) {
    int64 ds = MSECS_PER_DAY - msecs - 1;
    jd -= ds / MSECS_PER_DAY;
  }

  return Date::FromJulianDay((double)jd);
}

bool EqualSystemtime(const SYSTEMTIME& t1, const SYSTEMTIME& t2) {
  return (t1.wYear == t2.wYear
      && t1.wMonth == t2.wMonth
      && t1.wDay == t2.wDay
      && t1.wDayOfWeek == t2.wDayOfWeek
      && t1.wHour == t2.wHour
      && t1.wMinute == t2.wMinute
      && t1.wSecond == t2.wSecond
      && t1.wMilliseconds == t2.wMilliseconds);
}

bool EqualTzi(const TIME_ZONE_INFORMATION&tzi1, const TIME_ZONE_INFORMATION& tzi2) {
  return(tzi1.Bias == tzi2.Bias
       && tzi1.StandardBias == tzi2.StandardBias
       && EqualSystemtime(tzi1.StandardDate, tzi2.StandardDate)
       && wcscmp(tzi1.StandardName, tzi2.StandardName) == 0
       && tzi1.DaylightBias == tzi2.DaylightBias
       && EqualSystemtime(tzi1.DaylightDate, tzi2.DaylightDate)
       && wcscmp(tzi1.DaylightName, tzi2.DaylightName) == 0);
}

#ifdef FUN_USE_REGISTRY_TIMEZONE
bool OpenRegistryKey(const String& key_path, HKEY* key) {
  //return (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (const wchar_t*)key_path.utf16(), 0, KEY_READ, key)
  return (RegOpenKeyEx(HKEY_LOCAL_MACHINE, UTF8_TO_WCHAR(key_path.c_str()), 0, KEY_READ, key)
      == ERROR_SUCCESS);
}

String ReadRegistryString(const HKEY key, const wchar_t* value) {
  wchar_t buffer[MAX_PATH] = {0};
  DWORD size = sizeof(wchar_t) * MAX_PATH;
  RegQueryValueEx(key, (LPCWSTR)value, NULL, NULL, (LPBYTE)buffer, &size);
  //return String::fromWCharArray(buffer);
  return WCHAR_TO_UTF8(buffer);
}

int32 ReadRegistryValue(const HKEY key, const wchar_t* value) {
  DWORD buffer;
  DWORD size = sizeof(buffer);
  RegQueryValueEx(key, (LPCWSTR)value, NULL, NULL, (LPBYTE)&buffer, &size);
  return buffer;
}

WinTimeZoneImpl::WinTransitionRule
ReadRegistryRule(const HKEY key, const wchar_t* value, bool* ok) {
  *ok = false;
  WinTimeZoneImpl::WinTransitionRule rule;
  REG_TZI_FORMAT tzi;
  DWORD tzi_size = sizeof(tzi);
  if (RegQueryValueEx(key, (LPCWSTR)value, NULL, NULL, (BYTE*)&tzi, &tzi_size) == ERROR_SUCCESS) {
    rule.start_year = 0;
    rule.standard_time_bias = tzi.Bias + tzi.StandardBias;
    rule.daylight_time_bias = tzi.Bias + tzi.DaylightBias - rule.standard_time_bias;
    rule.standard_time_rule = tzi.StandardDate;
    rule.daylight_time_rule = tzi.DaylightDate;
    *ok = true;
  }
  return rule;
}

TIME_ZONE_INFORMATION GetRegistryTzi(const String& windows_id, bool* ok) {
  *ok = false;
  TIME_ZONE_INFORMATION tzi;
  REG_TZI_FORMAT reg_tzi;
  DWORD reg_tzi_size = sizeof(reg_tzi);
  HKEY key = NULL;
  const String tzi_key_path = String(TZ_REG_PATH) + '\\' + windows_id;

  if (OpenRegistryKey(tzi_key_path, &key)) {
    DWORD size = sizeof(tzi.DaylightName);
    RegQueryValueEx(key, L"Dlt", NULL, NULL, (LPBYTE)tzi.DaylightName, &size);

    size = sizeof(tzi.StandardName);
    RegQueryValueEx(key, L"Std", NULL, NULL, (LPBYTE)tzi.StandardName, &size);

    if (RegQueryValueEx(key, L"TZI", NULL, NULL, (BYTE*)&reg_tzi, &reg_tzi_size) == ERROR_SUCCESS) {
      tzi.Bias = reg_tzi.Bias;
      tzi.StandardBias = reg_tzi.StandardBias;
      tzi.DaylightBias = reg_tzi.DaylightBias;
      tzi.StandardDate = reg_tzi.StandardDate;
      tzi.DaylightDate = reg_tzi.DaylightDate;
      *ok = true;
    }

    RegCloseKey(key);
  }

  return tzi;
}

#else // FUN_USE_REGISTRY_TIMEZONE

struct WinDynamicTimeZone {
  String standard_name;
  String daylight_name;
  String time_zone_name;
  qint32 bias;
  bool daylight_time;
};

typedef QHash<String, WinDynamicTimeZone> WinRtTimeZoneHash;

Q_GLOBAL_STATIC(WinRtTimeZoneHash, g_time_zones)

void EnumerateTimeZones() {
  DYNAMIC_TIME_ZONE_INFORMATION dtz_info;
  quint32 index = 0;
  String prev_time_zone_key_name;
  while (SUCCEEDED(EnumDynamicTimeZoneInformation(index++, &dtz_info))) {
    WinDynamicTimeZone item;
    item.time_zone_name = String::fromWCharArray(dtz_info.TimeZoneKeyName);
    // As soon as key name repeats, break. Some systems continue to always
    // return the last item independent of index being out of range
    if (item.time_zone_name == prev_time_zone_key_name) {
      break;
    }
    item.standard_name = String::fromWCharArray(dtz_info.StandardName);
    item.daylight_name = String::fromWCharArray(dtz_info.DaylightName);
    item.daylight_time = !dtz_info.DynamicDaylightTimeDisabled;
    item.bias = dtz_info.Bias;
    g_time_zones->insert(item.time_zone_name.toUtf8(), item);
    prev_time_zone_key_name = item.time_zone_name;
  }
}

DYNAMIC_TIME_ZONE_INFORMATION DynamicInfoForId(const String& windows_id) {
  DYNAMIC_TIME_ZONE_INFORMATION dtz_info;
  quint32 index = 0;
  String prev_time_zone_key_name;
  while (SUCCEEDED(EnumDynamicTimeZoneInformation(index++, &dtz_info))) {
    const String time_zone_name = String::fromWCharArray(dtz_info.TimeZoneKeyName);
    if (time_zone_name == QLatin1String(windows_id)) {
      break;
    }
    if (time_zone_name == prev_time_zone_key_name) {
      break;
    }
    prev_time_zone_key_name = time_zone_name;
  }
  return dtz_info;
}

WinTimeZoneImpl::WinTransitionRule
ReadDynamicRule(DYNAMIC_TIME_ZONE_INFORMATION& dtzi, int32 year, bool* ok) {
  TIME_ZONE_INFORMATION tzi;
  WinTimeZoneImpl::WinTransitionRule rule;
  *ok = GetTimeZoneInformationForYear(year, &dtzi, &tzi);
  if (*ok) {
    rule.start_year = 0;
    rule.standard_time_bias = tzi.Bias + tzi.StandardBias;
    rule.daylight_time_bias = tzi.Bias + tzi.DaylightBias - rule.standard_time_bias;
    rule.standard_time_rule = tzi.StandardDate;
    rule.daylight_time_rule = tzi.DaylightDate;
  }
  return rule;
}

#endif // FUN_USE_REGISTRY_TIMEZONE

bool IsSameRule(const WinTimeZoneImpl::WinTransitionRule& last,
                const WinTimeZoneImpl::WinTransitionRule& rule) {
  // In particular, when this is true and either wYear is 0, so is the other;
  // so if one rule is recurrent and they're equal, so is the other.  If
  // either rule *isn't* recurrent, it has non-0 wYear which shall be
  // different from the other's.  Note that we don't compare .start_year, since
  // that will always be different.
  return EqualSystemtime(last.standard_time_rule, rule.standard_time_rule)
      && EqualSystemtime(last.daylight_time_rule, rule.daylight_time_rule)
      && last.standard_time_bias == rule.standard_time_bias
      && last.daylight_time_bias == rule.daylight_time_bias;
}

Array<String> AvailableWindowsIds() {
#ifdef FUN_USE_REGISTRY_TIMEZONE
  // TODO Consider caching results in a global static, very unlikely to change.
  Array<String> list;
  HKEY key = NULL;
  if (OpenRegistryKey(TZ_REG_PATH, &key)) {
    DWORD id_count = 0;
    if (RegQueryInfoKey(key, 0, 0, 0, &id_count, 0, 0, 0, 0, 0, 0, 0) == ERROR_SUCCESS
      && id_count > 0) {
      for (DWORD i = 0; i < id_count; ++i) {
        DWORD max_len = MAX_KEY_LENGTH;
        wchar_t buffer[MAX_KEY_LENGTH];
        if (RegEnumKeyEx(key, i, buffer, &max_len, 0, 0, 0, 0) == ERROR_SUCCESS) {
          list.Add(String(WCHAR_TO_UTF8(buffer)));
        }
      }
    }
    RegCloseKey(key);
  }
  return list;
#else // FUN_USE_REGISTRY_TIMEZONE
  if (g_time_zones->IsEmpty()) {
    EnumerateTimeZones();
  }
  return g_time_zones->keys();
#endif // FUN_USE_REGISTRY_TIMEZONE
}

String WindowsSystemZoneId() {
#ifdef FUN_USE_REGISTRY_TIMEZONE
  // On Vista and later is held in the value TimeZoneKeyName in key CURRENT_TZ_REG_PATH
  String id;
  HKEY key = NULL;
  String tzi_key_path = CURRENT_TZ_REG_PATH;
  if (OpenRegistryKey(tzi_key_path, &key)) {
    id = ReadRegistryString(key, L"TimeZoneKeyName");
    RegCloseKey(key);
    if (!id.IsEmpty()) {
      return id;
    }
  }

  // On XP we have to iterate over the zones until we find a match on
  // names/offsets with the current data
  TIME_ZONE_INFORMATION sys_tzi;
  GetTimeZoneInformation(&sys_tzi);
  bool ok = false;
  const auto win_ids = AvailableWindowsIds();
  for (const String& win_id : win_ids) {
    if (EqualTzi(GetRegistryTzi(win_id, &ok), sys_tzi)) {
      return win_id;
    }
  }
#else // FUN_USE_REGISTRY_TIMEZONE
  DYNAMIC_TIME_ZONE_INFORMATION dtzi;
  if (SUCCEEDED(GetDynamicTimeZoneInformation(&dtzi))) {
    return String::fromWCharArray(dtzi.TimeZoneKeyName).toLocal8Bit();
  }
#endif // FUN_USE_REGISTRY_TIMEZONE

  // If we can't determine the current ID use UTC
  return "UTC";
}

Date CalculateTransitionLocalDate(const SYSTEMTIME& rule, int32 year) {
  // If month is 0 then there is no date
  if (rule.wMonth == 0) {
    return Date();
  }

  // Interpret SYSTEMTIME according to the slightly quirky rules in:
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms725481(v=vs.85).aspx

  // If the year is set, the rule gives an absolute date:
  if (rule.wYear) {
    return Date(rule.wYear, rule.wMonth, rule.wDay);
  }

  // Otherwise, the rule date is annual and relative:
  const int32 day_of_week = rule.wDayOfWeek == 0 ? 7 : rule.wDayOfWeek;
  Date date(year, rule.wMonth, 1);
  // How many days before was last day_of_week before target month ?
  int32 adjust = day_of_week - (int32)date.DayOfWeek(); // -6 <= adjust < 7
  if (adjust >= 0) { // Ensure -7 <= adjust < 0:
    adjust -= 7;
  }
  // Normally, wDay is day-within-month; but here it is 1 for the first
  // of the given day_of_week in the month, through 4 for the fourth or ...
  adjust += (rule.wDay < 1 ? 1 : rule.wDay > 4 ? 5 : rule.wDay) * 7;
  date = date.AddDays(adjust);
  // ... 5 for the last; so back up by weeks to get within the month:
  if (date.Month() != rule.wMonth) {
    fun_check(rule.wDay > 4);
    // (Note that, with adjust < 0, date <= 28th of our target month
    // is guaranteed when wDay <= 4, or after our first -7 here.)
    date = date.AddDays(-7);
    fun_check(date.Month() == rule.wMonth);
  }
  return date;
}

// Converts a date/time value into msecs
inline int64 TimeToMSecs(const Date& date, const Time& time) {
  return (int64)((date.ToJulianDay() - JULIAN_DAY_FOR_EPOCH) * MSECS_PER_DAY) + time.ToMSecsStartOfDay();
}

int64 calculateTransitionForYear(const SYSTEMTIME& rule, int32 year, int32 bias) {
  // TODO Consider caching the calculated values - i.e. replace SYSTEMTIME in
  // WinTransitionRule; do this in init() once and store the results.
  const Date date = CalculateTransitionLocalDate(rule, year);
  const Time time = Time(rule.wHour, rule.wMinute, rule.wSecond);
  if (date.IsValid() && time.IsValid()) {
    return TimeToMSecs(date, time) + bias * 60000;
  }
  return TimeZoneImpl::InvalidMSecs();
}

struct TransitionTimePair {
  // Transition times after the epoch, in ms:
  int64 std, dst;
  // If either is InvalidMSecs(), which shall then be < the other, there is no
  // DST and the other describes a change in actual standard offset.

  TransitionTimePair(const WinTimeZoneImpl::WinTransitionRule& rule,
             int32 year, int32 old_year_offset)
    // The local time in Daylight Time of the switch to Standard Time
    : std(calculateTransitionForYear(rule.standard_time_rule, year,
                     rule.standard_time_bias + rule.daylight_time_bias)),
      // The local time in Standard Time of the switch to Daylight Time
      dst(calculateTransitionForYear(rule.daylight_time_rule, year, rule.standard_time_bias)) {
    // Check for potential "fake DST", used by MS's APIs because the
    // TIME_ZONE_INFORMATION spec either expresses no transitions in the
    // year, or expresses a transition of each kind, even if standard time
    // did change in a year with no DST.  We've seen year-start fake-DST
    // (whose offset matches prior standard offset, in which the previous
    // year ended); and conjecture that similar might be used at a year-end.
    // (This might be used for a southern-hemisphere zone, where the start of
    // the year usually is in DST, when applicable.)  Note that, here, wDay
    // identifies an instance of a given day-of-week in the month, with 5
    // meaning last.
    //
    // Either the alleged standard_time_rule or the alleged daylight_time_rule
    // may be faked; either way, the transition is actually a change to the
    // current standard offset; but the unfaked half of the rule contains the
    // useful bias data, so we have to go along with its lies.
    //
    // Example: Russia/Moscow
    // Format: -bias +( -stdBias, stdDate | -dstBias, dstDate ) notes
    // Last year of DST, 2010: 180 +( 0, 0-10-5 3:0 | 60, 0-3-5 2:0 ) normal DST
    // Zone change in 2011: 180 +( 0, 0-1-1 0:0 | 60 0-3-5 2:0 ) fake DST at transition
    // Fixed standard in 2012: 240 +( 0, 0-0-0 0:0 | 60, 0-0-0 0:0 ) standard time years
    // Zone change in 2014: 180 +( 0, 0-10-5 2:0 | 60, 0-1-1 0:0 ) fake DST at year-start
    // The last of these is missing on Win7 VMs (too old to know about it).

    if (rule.daylight_time_rule.wMonth == 1 && rule.daylight_time_rule.wDay == 1) {
      // Fake "DST transition" at start of year producing the same offset as
      // previous year ended in.
      if (rule.standard_time_bias + rule.daylight_time_bias == old_year_offset) {
        dst = TimeZoneImpl::InvalidMSecs();
      }
    } else if (rule.daylight_time_rule.wMonth == 12 && rule.daylight_time_rule.wDay > 3) {
      // Similar, conjectured, for end of year, not changing offset.
      if (rule.daylight_time_bias == 0) {
        dst = TimeZoneImpl::InvalidMSecs();
      }
    } if (rule.standard_time_rule.wMonth == 1 && rule.standard_time_rule.wDay == 1) {
      // Fake "transition out of DST" at start of year producing the same
      // offset as previous year ended in.
      if (rule.standard_time_bias == old_year_offset) {
        std = TimeZoneImpl::InvalidMSecs();
      }
    } else if (rule.standard_time_rule.wMonth == 12 && rule.standard_time_rule.wDay > 3) {
      // Similar, conjectured, for end of year, not changing offset.
      if (rule.daylight_time_bias == 0) {
        std = TimeZoneImpl::InvalidMSecs();
      }
    }
  }

  bool FakesDst() const {
    return std == TimeZoneImpl::InvalidMSecs() || dst == TimeZoneImpl::InvalidMSecs();
  }
};

int32 YearEndOffset(const WinTimeZoneImpl::WinTransitionRule& rule, int32 year) {
  int32 offset = rule.standard_time_bias;
  // Only needed to help another TransitionTimePair work out year + 1's start
  // offset; and the old_year_offset we use only affects an alleged transition
  // at the *start* of this year, so it doesn't matter if we guess wrong here:
  TransitionTimePair pair(rule, year, offset);
  if (pair.dst > pair.std) {
    offset += rule.daylight_time_bias;
  }
  return offset;
}

Locale::Country GetUserCountry() {
  const GEOID id = GetUserGeoID(GEOCLASS_NATION);
  wchar_t code[3];
  const int32 size = GetGeoInfo(id, GEO_ISO2, code, 3, 0);
  return (size == 3) ? LocaleImpl::CodeToCountry(UStringView(code, size)) : Locale::AnyCountry;
}

// Index of last rule in rules with .start_year <= year:
int32 RuleIndexForYear(const Array<WinTimeZoneImpl::WinTransitionRule>& rules, int32 year) {
  if (rules.Last().start_year <= year) {
    return rules.Count() - 1;
  }

  // We don't have a rule for before the first, but the first is the best we can offer:
  if (rules.First().start_year > year) {
    return 0;
  }

  // Otherwise, use binary chop:
  int32 lo = 0, hi = rules.Count();
  // invariant: rules[i].start_year <= year < rules[hi].start_year
  // subject to treating rules[rules.Count()] as "off the end of time"
  while (lo + 1 < hi) {
    const int32 mid = (lo + hi) / 2;
    // lo + 2 <= hi, so lo + 1 <= mid <= hi - 1, so lo < mid < hi
    // In particular, mid < rules.Count()
    const int32 mid_year = rules[mid].start_year;
    if (mid_year > year) {
      hi = mid;
    } else if (mid_year < year) {
      lo = mid;
    } else { // No two rules have the same start_year:
      return mid;
    }
  }
  return lo;
}

} // namespace


WinTimeZoneImpl::WinTimeZoneImpl() : TimeZoneImpl() {
  Init(String());
}

WinTimeZoneImpl::WinTimeZoneImpl(const String& iana_id) : TimeZoneImpl() {
  Init(iana_id);
}

WinTimeZoneImpl::WinTimeZoneImpl(const WinTimeZoneImpl& other)
  : TimeZoneImpl(other),
    windows_id_(other.windows_id_),
    display_name_(other.display_name_),
    standard_name_(other.standard_name_),
    daylight_name_(other.daylight_name_),
    trans_rules_(other.trans_rules_) {}

WinTimeZoneImpl::~WinTimeZoneImpl() {}

WinTimeZoneImpl* WinTimeZoneImpl::Clone() const {
  return new WinTimeZoneImpl(*this);
}

void WinTimeZoneImpl::Init(const String& iana_id) {
  if (iana_id.IsEmpty()) {
    windows_id_ = WindowsSystemZoneId();
    id_ = GetSystemTimeZoneId();
  } else {
    windows_id_ = IanaIdToWindowsId(iana_id);
    id_ = iana_id;
  }

  bool bad_month = false; // Only warn once per zone, if at all.
  if (!windows_id_.IsEmpty()) {
#ifdef FUN_USE_REGISTRY_TIMEZONE
    // Open the base TZI for the time zone
    HKEY base_key = NULL;
    const String base_key_path = String(TZ_REG_PATH) + '\\' + windows_id_;
    if (OpenRegistryKey(base_key_path, &base_key)) {
      //  Load the localized names
      display_name_ = ReadRegistryString(base_key, L"Display");
      standard_name_ = ReadRegistryString(base_key, L"Std");
      daylight_name_ = ReadRegistryString(base_key, L"Dlt");
      // On Vista and later the optional dynamic key holds historic data
      const String dynamic_key_path = base_key_path + "\\Dynamic DST";
      HKEY dynamic_key = NULL;
      if (OpenRegistryKey(dynamic_key_path, &dynamic_key)) {
        // Find out the start and end years stored, then iterate over them
        int32 start_year = ReadRegistryValue(dynamic_key, L"FirstEntry");
        int32 end_year = ReadRegistryValue(dynamic_key, L"LastEntry");
        for (int32 year = start_year; year <= end_year; ++year) {
          bool rule_ok;
          WinTransitionRule rule = ReadRegistryRule(dynamic_key,
                                 (LPCWSTR)UString::FromNumber(year).c_str(),
                                 &rule_ok);
          if (rule_ok
            // Don't repeat a recurrent rule:
            && (trans_rules_.IsEmpty()
              || !IsSameRule(trans_rules_.Last(), rule))) {
            if (!bad_month
              && (rule.standard_time_rule.wMonth == 0)
              != (rule.daylight_time_rule.wMonth == 0)) {
              bad_month = true;
              //qWarning("MS registry TZ API violated its wMonth constraint;"
              //     "this may cause mistakes for %s from %d",
              //     iana_id.constData(), year);
            }
            rule.start_year = trans_rules_.IsEmpty() ? MIN_YEAR : year;
            trans_rules_.Add(rule);
          }
        }
        RegCloseKey(dynamic_key);
      } else {
        // No dynamic data so use the base data
        bool rule_ok;
        WinTransitionRule rule = ReadRegistryRule(base_key, L"TZI", &rule_ok);
        rule.start_year = MIN_YEAR;
        if (rule_ok) {
          trans_rules_.Add(rule);
        }
      }
      RegCloseKey(base_key);
    }
#else // FUN_USE_REGISTRY_TIMEZONE
    if (g_time_zones->IsEmpty()) {
      EnumerateTimeZones();
    }

    WinRtTimeZoneHash::const_iterator it = g_time_zones->find(windows_id_);
    if (it != g_time_zones->constEnd()) {
      display_name_ = it->time_zone_name;
      standard_name_ = it->standard_name;
      daylight_name_ = it->daylight_name;
      DWORD first_year = 0;
      DWORD last_year = 0;
      DYNAMIC_TIME_ZONE_INFORMATION dtzi = DynamicInfoForId(windows_id_);
      if (GetDynamicTimeZoneInformationEffectiveYears(&dtzi, &first_year, &last_year)
        == ERROR_SUCCESS && first_year < last_year) {
        for (DWORD year = first_year; year <= last_year; ++year) {
          bool ok = false;
          WinTransitionRule rule = ReadDynamicRule(dtzi, year, &ok);
          if (ok
            // Don't repeat a recurrent rule
            && (trans_rules_.IsEmpty()
              || !IsSameRule(trans_rules_.last(), rule))) {
            if (!bad_month
              && (rule.standard_time_rule.wMonth == 0)
              != (rule.daylight_time_rule.wMonth == 0)) {
              bad_month = true;
              qWarning("MS dynamic TZ API violated its wMonth constraint;"
                   "this may cause mistakes for %s from %d",
                   iana_id.constData(), year);
            }
            rule.start_year = trans_rules_.IsEmpty() ? MIN_YEAR : year;
            trans_rules_.Add(rule);
          }
        }
      } else {
        // At least try to get the non-dynamic data:
        dtzi.DynamicDaylightTimeDisabled = false;
        bool ok = false;
        WinTransitionRule rule = ReadDynamicRule(dtzi, 1970, &ok);
        if (ok) {
          rule.start_year = MIN_YEAR;
          trans_rules_.Add(rule);
        }
      }
    }
#endif // FUN_USE_REGISTRY_TIMEZONE
  }

  // If there are no rules then we failed to find a windows_id or any tzi info
  if (trans_rules_.Count() == 0) {
    id_.Clear();
    windows_id_.Clear();
    display_name_.Clear();
  }
}

String WinTimeZoneImpl::GetComment() const {
  return display_name_;
}

String WinTimeZoneImpl::GetDisplayName(TimeZone::TimeType time_type,
                     TimeZone::NameType name_type,
                     const Locale& locale) const {
  // TODO Registry holds MUI keys, should be able to look up translations?
  FUN_UNUSED(locale);

  if (name_type == TimeZone::OffsetName) {
    const auto& rule =
      trans_rules_[RuleIndexForYear(trans_rules_, Date::Today().Year())];
    if (time_type == TimeZone::DaylightTime) {
      return IsoOffsetFormat((rule.standard_time_bias + rule.daylight_time_bias) * -60);
    } else {
      return IsoOffsetFormat((rule.standard_time_bias) * -60);
    }
  }

  switch (time_type) {
    case TimeZone::DaylightTime:
      return daylight_name_;
    case TimeZone::GenericTime:
      return display_name_;
    case TimeZone::StandardTime:
      return standard_name_;
  }
  return standard_name_;
}

String WinTimeZoneImpl::GetAbbreviation(int64 at_msecs_since_epoch) const {
  return GetData(at_msecs_since_epoch).abbreviation;
}

int32 WinTimeZoneImpl::GetOffsetFromUtc(int64 at_msecs_since_epoch) const {
  return GetData(at_msecs_since_epoch).offset_from_utc;
}

int32 WinTimeZoneImpl::GetStandardTimeOffset(int64 at_msecs_since_epoch) const {
  return GetData(at_msecs_since_epoch).standard_time_offset;
}

int32 WinTimeZoneImpl::GetDaylightTimeOffset(int64 at_msecs_since_epoch) const {
  return GetData(at_msecs_since_epoch).daylight_time_offset;
}

bool WinTimeZoneImpl::HasDaylightTime() const {
  return HasTransitions();
}

bool WinTimeZoneImpl::IsDaylightTime(int64 at_msecs_since_epoch) const {
  return (GetData(at_msecs_since_epoch).daylight_time_offset != 0);
}

TimeZoneImpl::Data
WinTimeZoneImpl::GetData(int64 for_msecs_since_epoch) const {
  int32 year = MSecsToDate(for_msecs_since_epoch).Year();
  for (int32 rule_index = RuleIndexForYear(trans_rules_, year);
     rule_index >= 0; --rule_index) {
    const auto& rule = trans_rules_[rule_index];
    // Does this rule's period include any transition at all ?
    if (rule.standard_time_rule.wMonth > 0 || rule.daylight_time_rule.wMonth > 0) {
      const int32 end_year = MathBase::Max(rule.start_year, year - 1);
      while (year >= end_year) {
        const int32 new_year_offset = (year <= rule.start_year && rule_index > 0)
          ? YearEndOffset(trans_rules_[rule_index - 1], year - 1)
          : YearEndOffset(rule, year - 1);
        const TransitionTimePair pair(rule, year, new_year_offset);
        bool is_dst = false;
        if (pair.std != InvalidMSecs() && pair.std <= for_msecs_since_epoch) {
          is_dst = pair.std < pair.dst && pair.dst <= for_msecs_since_epoch;
        } else if (pair.dst != InvalidMSecs() && pair.dst <= for_msecs_since_epoch) {
          is_dst = true;
        } else {
          --year; // Try an earlier year for this rule (once).
          continue;
        }
        return RuleToData(rule, for_msecs_since_epoch,
                  is_dst ? TimeZone::DaylightTime : TimeZone::StandardTime,
                  pair.FakesDst());
      }
      // Fell off start of rule, try previous rule.
    } else {
      // No transition, no DST, use the year's standard time.
      return RuleToData(rule, for_msecs_since_epoch, TimeZone::StandardTime);
    }

    if (year >= rule.start_year) {
      year = rule.start_year - 1; // Seek last transition in new rule.
    }
  }
  // We don't have relevant data :-(
  return InvalidData();
}

bool WinTimeZoneImpl::HasTransitions() const {
  for (const auto& rule : trans_rules_) {
    if (rule.standard_time_rule.wMonth > 0 && rule.daylight_time_rule.wMonth > 0) {
      return true;
    }
  }
  return false;
}

TimeZoneImpl::Data
WinTimeZoneImpl::NextTransition(int64 after_msecs_since_epoch) const {
  int32 year = MSecsToDate(after_msecs_since_epoch).Year();
  for (int32 rule_index = RuleIndexForYear(trans_rules_, year);
     rule_index < trans_rules_.Count(); ++rule_index) {
    const auto& rule = trans_rules_[rule_index];
    // Does this rule's period include any transition at all ?
    if (rule.standard_time_rule.wMonth > 0 || rule.daylight_time_rule.wMonth > 0) {
      if (year < rule.start_year) {
        year = rule.start_year; // Seek first transition in this rule.
      }
      const int32 end_year = rule_index + 1 < trans_rules_.Count()
        ? MathBase::Min(trans_rules_[rule_index + 1].start_year, year + 2) : (year + 2);
      int32 new_year_offset = (year <= rule.start_year && rule_index > 0)
        ? YearEndOffset(trans_rules_[rule_index - 1], year - 1)
        : YearEndOffset(rule, year - 1);
      while (year < end_year) {
        const TransitionTimePair pair(rule, year, new_year_offset);
        bool is_dst = false;
        fun_check(InvalidMSecs() <= after_msecs_since_epoch); // invalid is min int64
        if (pair.std > after_msecs_since_epoch) {
          is_dst = pair.std > pair.dst && pair.dst > after_msecs_since_epoch;
        } else if (pair.dst > after_msecs_since_epoch) {
          is_dst = true;
        } else {
          new_year_offset = rule.standard_time_bias;
          if (pair.dst > pair.std) {
            new_year_offset += rule.daylight_time_bias;
          }
          ++year; // Try a later year for this rule (once).
          continue;
        }

        if (is_dst) {
          return RuleToData(rule, pair.dst, TimeZone::DaylightTime, pair.FakesDst());
        }
        return RuleToData(rule, pair.std, TimeZone::StandardTime, pair.FakesDst());
      }
      // Fell off end of rule, try next rule.
    } // else: no transition during rule's period
  }
  // Apparently no transition after the given time:
  return InvalidData();
}

TimeZoneImpl::Data
WinTimeZoneImpl::PreviousTransition(int64 before_msecs_since_epoch) const {
  const int64 start_of_time = InvalidMSecs() + 1;
  if (before_msecs_since_epoch <= start_of_time) {
    return InvalidData();
  }

  int32 year = MSecsToDate(before_msecs_since_epoch).Year();
  for (int32 rule_index = RuleIndexForYear(trans_rules_, year);
     rule_index >= 0; --rule_index) {
    const WinTransitionRule& rule = trans_rules_[rule_index];
    // Does this rule's period include any transition at all ?
    if (rule.standard_time_rule.wMonth > 0 || rule.daylight_time_rule.wMonth > 0) {
      const int32 end_year = MathBase::Max(rule.start_year, year - 1);
      while (year >= end_year) {
        const int32 new_year_offset = (year <= rule.start_year && rule_index > 0)
          ? YearEndOffset(trans_rules_[rule_index - 1], year - 1)
          : YearEndOffset(rule, year - 1);
        const TransitionTimePair pair(rule, year, new_year_offset);
        bool is_dst = false;
        if (pair.std != InvalidMSecs() && pair.std < before_msecs_since_epoch) {
          is_dst = pair.std < pair.dst && pair.dst < before_msecs_since_epoch;
        } else if (pair.dst != InvalidMSecs() && pair.dst < before_msecs_since_epoch) {
          is_dst = true;
        } else {
          --year; // Try an earlier year for this rule (once).
          continue;
        }
        if (is_dst) {
          return RuleToData(rule, pair.dst, TimeZone::DaylightTime, pair.FakesDst());
        }
        return RuleToData(rule, pair.std, TimeZone::StandardTime, pair.FakesDst());
      }
      // Fell off start of rule, try previous rule.
    } else if (rule_index == 0) {
      // Treat a no-transition first rule as a transition at the start of
      // time, so that a scan through all rules *does* see it as the first
      // rule:
      return RuleToData(rule, start_of_time, TimeZone::StandardTime, false);
    } // else: no transition during rule's period
    if (year >= rule.start_year) {
      year = rule.start_year - 1; // Seek last transition in new rule
    }
  }
  // Apparently no transition before the given time:
  return InvalidData();
}

String WinTimeZoneImpl::GetSystemTimeZoneId() const {
  const Locale::Country country = GetUserCountry();
  const String windows_id = WindowsSystemZoneId();
  String iana_id;
  // If we have a real country, then try get a specific match for that country
  if (country != Locale::AnyCountry)
    iana_id = WindowsIdToDefaultIanaId(windows_id, country);
  // If we don't have a real country, or there wasn't a specific match, try the global default
  if (iana_id.IsEmpty()) {
    iana_id = WindowsIdToDefaultIanaId(windows_id);
    // If no global default then probably an unknown Windows ID so return UTC
    if (iana_id.IsEmpty()) {
      return "UTC";
    }
  }
  return iana_id;
}

Array<String> WinTimeZoneImpl::AvailableTimeZoneIds() const {
  Array<String> result;
  const auto win_ids = AvailableWindowsIds();
  for (const String& win_id : win_ids) {
    result += WindowsIdToIanaIds(win_id);
  }

  result.Sort();

  //TODO
  fun_check(0);
  //result.erase(std::unique(result.begin(), result.end()), result.end());

  return result;
}

TimeZoneImpl::Data
WinTimeZoneImpl::RuleToData(const WinTransitionRule& rule,
                             int64 at_msecs_since_epoch,
                             TimeZone::TimeType type,
                             bool fake_dst) const {
  Data trans = InvalidData();
  trans.at_msecs_since_epoch = at_msecs_since_epoch;
  trans.standard_time_offset = rule.standard_time_bias * -60;
  if (fake_dst) {
    trans.daylight_time_offset = 0;
    trans.abbreviation = standard_name_;
    // Rule may claim we're in DST when it's actually a standard time change:
    if (type == TimeZone::DaylightTime) {
      trans.standard_time_offset += rule.daylight_time_bias * -60;
    }
  } else if (type == TimeZone::DaylightTime) {
    trans.daylight_time_offset = rule.daylight_time_bias * -60;
    trans.abbreviation = daylight_name_;
  } else {
    trans.daylight_time_offset = 0;
    trans.abbreviation = standard_name_;
  }
  trans.offset_from_utc = trans.standard_time_offset + trans.daylight_time_offset;
  return trans;
}

} // namespace fun
