#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/date_time.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/locale/locale.h"
#include "fun/base/string/string.h"

namespace fun {

class FUN_BASE_API TimeZone {
 public:
  enum TimeType { StandardTime = 0, DaylightTime = 1, GenericTime = 2 };

  enum NameType {
    DefaultName = 0,
    LongName = 1,
    ShortName = 2,
    OffsetName = 3
  };

  struct OffsetData {
    String abbreviation;
    DateTime at_utc;
    int32 offset_from_utc;
    int32 standard_time_offset;
    int32 daylight_time_offset;
  };
  typedef Array<OffsetData> OffsetDataList;

  TimeZone();
  TimeZone(const String& iana_id);
  TimeZone(const Timespan& offset);
  TimeZone(const String& iana_id, const Timespan& offset, const String& name,
           const String& abbreviation, Locale::Country country,
           const String& comment);
  TimeZone(const TimeZone& rhs);
  ~TimeZone();

  TimeZone& operator=(const TimeZone& rhs);

  bool operator==(const TimeZone& rhs) const;
  bool operator!=(const TimeZone& rhs) const;

  bool IsValid() const;

  String GetId() const;
  Locale::Country GetCountry() const;
  String GetComment() const;

  String GetDisplayName(const DateTime& at_date_time,
                        TimeZone::NameType name_type = TimeZone::DefaultName,
                        const Locale& locale = Locale()) const;
  String GetDisplayName(TimeZone::TimeType time_type,
                        TimeZone::NameType name_type = TimeZone::DefaultName,
                        const Locale& locale = Locale()) const;
  String GetAbbreviation(const DateTime& at_date_time) const;

  Timespan GetOffsetFromUtc(const DateTime& at_date_time) const;
  Timespan GetStandardTimeOffset(const DateTime& at_date_time) const;
  Timespan GetDaylightTimeOffset(const DateTime& at_date_time) const;

  bool HasDaylightTime() const;
  bool IsDaylightTime(const DateTime& at_date_time) const;

  OffsetData GetOffsetData(const DateTime& for_date_time) const;

  bool HasTransitions() const;
  OffsetData NextTransition(const DateTime& after_date_time) const;
  OffsetData PreviousTransition(const DateTime& before_date_time) const;
  OffsetDataList GetTransitions(const DateTime& from_date_time,
                                const DateTime& to_date_time) const;

  static String GetSystemTimeZoneId();  // local id
  static TimeZone GetSystemTimeZone();  // local

  static String LocalId();
  static TimeZone Local();
  static TimeZone Utc();

  static bool IsTimeZoneAvailable(const String& iana_id);

  static Array<String> AvailableTimeZoneIds();
  static Array<String> AvailableTimeZoneIds(Locale::Country country);
  static Array<String> AvailableTimeZoneIds(int32 offset_seconds);

  static String IanaIdToWindowsId(const String& iana_id);
  static String WindowsIdToDefaultIanaId(const String& windows_id);
  static String WindowsIdToDefaultIanaId(const String& windows_id,
                                         Locale::Country country);
  static Array<String> WindowsIdToIanaIds(const String& windows_id);
  static Array<String> WindowsIdToIanaIds(const String& windows_id,
                                          Locale::Country country);

  FUN_BASE_API friend uint32 HashOf(const TimeZone& timezone);
  FUN_BASE_API friend Archive& operator&(Archive& ar, TimeZone& timezone);

  //임시로 public으로 한것임. 내부 코드가 정리되면 다시 private으로 해야함.
 public:
  friend class TimeZoneImpl;
  SharedPtr<TimeZoneImpl> impl_;
};

}  // namespace fun
