#pragma once

#include "fun/base/base.h"
#include "fun/base/locale/locale.h"
#include "fun/base/time_zone.h"

namespace fun {

class TimeZoneImpl {
 public:
  struct Data {
    String abbreviation;
    int64 at_msecs_since_epoch; // 1970-1-1 0:00:00
    int32 offset_from_utc;
    int32 standard_time_offset;
    int32 daylight_time_offset;
  };
  typedef Array<Data> DataList;

  TimeZoneImpl();
  TimeZoneImpl(const TimeZoneImpl& rhs);
  virtual ~TimeZoneImpl();

  virtual TimeZoneImpl* Clone() const;

  bool operator == (const TimeZoneImpl& rhs) const;
  bool operator != (const TimeZoneImpl& rhs) const;

  bool IsValid() const;

  String GetId() const;

  virtual Locale::Country GetCountry() const;
  virtual String GetComment() const;

  virtual String GetDisplayName(int64 at_msecs_since_epoch,
                                TimeZone::NameType name_type,
                                const Locale& locale) const;
  virtual String GetDisplayName(TimeZone::TimeType time_type,
                                TimeZone::NameType name_type,
                                const Locale& locale) const;
  virtual String GetAbbreviation(int64 at_msecs_since_epoch) const;

  virtual int32 GetOffsetFromUtc(int64 at_msecs_since_epoch) const;
  virtual int32 GetStandardTimeOffset(int64 at_msecs_since_epoch) const;
  virtual int32 GetDaylightTimeOffset(int64 at_msecs_since_epoch) const;

  virtual bool HasDaylightTime() const;
  virtual bool IsDaylightTime(int64 at_msecs_since_epoch) const;

  virtual Data GetData(int64 for_msecs_since_epoch) const;
  Data DataForLocalTime(int64 for_local_msecs, int32 hint) const;

  virtual bool HasTransitions() const;
  virtual Data NextTransition(int64 after_msecs_since_epoch) const;
  virtual Data PreviousTransition(int64 before_msec_since_epoch) const;
  DataList GetTransitions(int64 from_msecs_since_epoch,
                          int64 to_msecs_since_epoch) const;

  virtual String GetSystemTimeZoneId() const;

  virtual Array<String> AvailableTimeZoneIds() const;
  virtual Array<String> AvailableTimeZoneIds(Locale::Country country) const;
  virtual Array<String> AvailableTimeZoneIds(int32 utc_offset) const;

  static inline int64 MaxMSecs() { return std::numeric_limits<int64>::max(); }
  static inline int64 MinMSecs() { return std::numeric_limits<int64>::min() + 1; }
  static inline int64 InvalidMSecs() { return std::numeric_limits<int64>::min(); }
  static inline int64 InvalidSeconds() { return std::numeric_limits<int32>::min(); }
  static Data InvalidData();
  static TimeZone::OffsetData InvalidOffsetData();
  static TimeZone::OffsetData ToOffsetData(const Data& data);
  static bool IsValidId(const String& iana_id);
  static String IsoOffsetFormat(int32 offset_from_utc);

  static String IanaIdToWindowsId(const String& iana_id);
  static String WindowsIdToDefaultIanaId(const String& windows_id);
  static String WindowsIdToDefaultIanaId( const String& windows_id,
                                          Locale::Country country);
  static Array<String> WindowsIdToIanaIds(const String& windows_id);
  static Array<String> WindowsIdToIanaIds(const String& windows_id,
                                          Locale::Country country);

 protected:
  String id_;
};


class UtcTimeZoneImpl : public TimeZoneImpl {
 public:
  using Super = TimeZoneImpl;

  UtcTimeZoneImpl();
  UtcTimeZoneImpl(const String& iana_id);
  UtcTimeZoneImpl(int32 offset_seconds);
  UtcTimeZoneImpl(const String& iana_id,
                  int32 offset_seconds,
                  const String& name,
                  const String& abbreviation,
                  Locale::Country country,
                  const String& comment);
  UtcTimeZoneImpl(const UtcTimeZoneImpl& rhs);
  ~UtcTimeZoneImpl();

  //super에서는 TimeZoneImpl* 타입으로 처리하는데 말이지?
  UtcTimeZoneImpl* Clone() const override;

  Data GetData(int64 for_msecs_since_epoch) const override;

  Locale::Country GetCountry() const override;
  String GetComment() const override;

  String GetDisplayName(TimeZone::TimeType time_type,
                        TimeZone::NameType name_type,
                        const Locale& locale) const override;
  String GetAbbreviation(int64 at_msecs_since_epoch) const override;

  int32 GetStandardTimeOffset(int64 at_msecs_since_epoch) const override;
  int32 GetDaylightTimeOffset(int64 at_msecs_since_epoch) const override;

  String GetSystemTimeZoneId() const override;

  Array<String> AvailableTimeZoneIds() const override;
  Array<String> AvailableTimeZoneIds(Locale::Country country) const override;
  Array<String> AvailableTimeZoneIds(int32 utc_offset) const override;

 private:
  void Init(const String& zone_id);
  void Init(const String& zone_id,
            int32 offset_seconds,
            const String& name,
            const String& abbreviation,
            Locale::Country country,
            const String& comment);

  String name_;
  String abbreviation_;
  String comment_;
  Locale::Country country_;
  int32 offset_from_utc_;
};


#if FUN_WITH_ICU

class IcuTimeZonePrivate : public TimeZoneImpl {
 public:
  using Super = TimeZoneImpl;

  IcuTimeZonePrivate();
  IcuTimeZonePrivate(const String& iana_id);
  IcuTimeZonePrivate(const IcuTimeZonePrivate& rhs);
  ~IcuTimeZonePrivate();

  //super에서는 TimeZoneImpl* 타입으로 처리하는데 말이지?
  IcuTimeZonePrivate* Clone() const override;

  String GetDisplayName(TimeZone::TimeType time_type,
                        TimeZone::NameType name_type,
                        const Locale& locale) const override;
  String GetAbbreviation(int64 at_msecs_since_epoch) const override;

  int32 GetOffsetFromUtc(int64 at_msecs_since_epoch) const override;
  int32 GetStandardTimeOffset(int64 at_msecs_since_epoch) const override;
  int32 GetDaylightTimeOffset(int64 at_msecs_since_epoch) const override;

  bool HasDaylightTime() const override;
  bool IsDaylightTime(int64 at_msecs_since_epoch) const override;

  Data GetData(int64 for_msecs_since_epoch) const override;

  bool HasTransitions() const override;
  Data NextTransition(int64 after_msecs_since_epoch) const override;
  Data PreviousTransition(int64 before_msec_since_epoch) const override;

  String GetSystemTimeZoneId() const override;

  Array<String> AvailableTimeZoneIds() const override;
  Array<String> AvailableTimeZoneIds(Locale::Country country) const override;
  Array<String> AvailableTimeZoneIds(int32 utc_offset) const override;

 private:
  void Init(const String& zone_id);

  UCalendar* ucal_;
};

#endif //FUN_WITH_ICU


#if FUN_PLATFORM_WINDOWS_FAMILY

class WinTimeZoneImpl : public TimeZoneImpl
{
 public:
  using Super = TimeZoneImpl;

  struct WinTransitionRule {
    int32 start_year;
    int32 standard_time_bias;
    int32 daylight_time_bias;
    SYSTEMTIME standard_time_rule;
    SYSTEMTIME daylight_time_rule;

    WinTransitionRule()
      : start_year(0),
        standard_time_bias(0),
        daylight_time_bias(0) {
      UnsafeMemory::Memzero(&standard_time_rule, sizeof(standard_time_rule));
      UnsafeMemory::Memzero(&daylight_time_rule, sizeof(daylight_time_rule));
    }
  };

  WinTimeZoneImpl();
  WinTimeZoneImpl(const String& iana_id);
  WinTimeZoneImpl(const WinTimeZoneImpl& rhs);
  ~WinTimeZoneImpl();

  //super에서는 TimeZoneImpl* 타입으로 처리하는데 말이지?
  WinTimeZoneImpl* Clone() const override;

  String GetComment() const override;

  String GetDisplayName(TimeZone::TimeType time_type,
                        TimeZone::NameType name_type,
                        const Locale& locale) const override;
  String GetAbbreviation(int64 at_msecs_since_epoch) const override;

  int32 GetOffsetFromUtc(int64 at_msecs_since_epoch) const override;
  int32 GetStandardTimeOffset(int64 at_msecs_since_epoch) const override;
  int32 GetDaylightTimeOffset(int64 at_msecs_since_epoch) const override;

  bool HasDaylightTime() const override;
  bool IsDaylightTime(int64 at_msecs_since_epoch) const override;

  Data GetData(int64 for_msecs_since_epoch) const override;

  bool HasTransitions() const override;
  Data NextTransition(int64 after_msecs_since_epoch) const override;
  Data PreviousTransition(int64 before_msec_since_epoch) const override;

  String GetSystemTimeZoneId() const override;

  Array<String> AvailableTimeZoneIds() const override;

 private:
  void Init(const String& iana_id);

  WinTransitionRule RuleForYear(int32 year) const;
  TimeZoneImpl::Data RuleToData(const WinTransitionRule& rule,
                                int64 at_msecs_since_epoch,
                                TimeZone::TimeType type,
                                bool fake_dst = false) const;

  String windows_id_;
  String display_name_;
  String standard_name_;
  String daylight_name_;
  Array<WinTransitionRule> trans_rules_;
};

#endif // FUN_PLATFORM_WINDOWS_FAMILY


struct TzTransitionTime {
  int64 at_msecs_since_epoch;
  uint8 rule_index;
};

struct TzTransitionRule {
  int32 std_offset;
  int32 dst_offset;
  uint8 abbreviation_index;

  bool operator == (const TzTransitionRule& other) const {
    return  std_offset == other.std_offset &&
            dst_offset == other.dst_offset &&
            abbreviation_index == other.abbreviation_index;
  }

  bool operator != (const TzTransitionRule& other) const {
    return !(*this == other);
  }
};


#if FUN_PLATFORM_UNIX_FAMILY

class UnixTimeZoneImpl : public TimeZoneImpl {
 public:
  UnixTimeZoneImpl();
  UnixTimeZoneImpl(const String& iana_id);
  UnixTimeZoneImpl(const UnixTimeZoneImpl& other);
  ~UnixTimeZoneImpl();

  UnixTimeZoneImpl* Clone() const override;

  String GetDisplayName(TimeZone::TimeType time_type,
                        TimeZone::NameType name_type,
                        const Locale& locale) const override;
  String GetAbbreviation(int64 at_msecs_since_epoch) const override;

  int32 GetOffsetFromUtc(int64 at_msecs_since_epoch) const override;
  int32 GetStandardTimeOffset(int64 at_msecs_since_epoch) const override;
  int32 GetDaylightTimeOffset(int64 at_msecs_since_epoch) const override;

  bool HasDaylightTime() const override;
  bool IsDaylightTime(int64 at_msecs_since_epoch) const override;

  Data GetData(int64 for_msecs_since_epoch) const override;

  bool HasTransitions() const override;
  Data NextTransition(int64 after_msecs_since_epoch) const override;
  Data PreviousTransition(int64 before_msec_since_epoch) const override;

  String GetSystemTimeZoneId() const override;

  Array<String> AvailableTimeZoneIds() const override;

 private:
  void Init(const String& iana_id);

  Data DataForTzTransition(TzTransitionTime trans) const;
  Array<TzTransitionTime> trans_times_;
  Array<TzTransitionTime> trans_rules_;
  Array<String> abbreviations_;
#if FUN_WITH_ICU
  mutable SharedPtr<TimeZoneImpl> icu_;
#endif
  String posix_rule_;
};

#endif // FUN_PLATFORM_UNIX_FAMILY


#if FUN_PLATFORM == FUN_PLATFORM_MAC

class MacTimeZoneImpl : public TimeZoneImpl {
 public:
  MacTimeZoneImpl();
  MacTimeZoneImpl(const String& iana_id);
  MacTimeZoneImpl(const MacTimeZoneImpl& other);
  ~MacTimeZoneImpl();

  MacTimeZoneImpl* Clone() const override;

  String GetDisplayName(TimeZone::TimeType time_type,
                        TimeZone::NameType name_type,
                        const Locale& locale) const override;
  String GetAbbreviation(int64 at_msecs_since_epoch) const override;

  int32 GetOffsetFromUtc(int64 at_msecs_since_epoch) const override;
  int32 GetStandardTimeOffset(int64 at_msecs_since_epoch) const override;
  int32 GetDaylightTimeOffset(int64 at_msecs_since_epoch) const override;

  bool HasDaylightTime() const override;
  bool IsDaylightTime(int64 at_msecs_since_epoch) const override;

  Data GetData(int64 for_msecs_since_epoch) const override;

  bool HasTransitions() const override;
  Data NextTransition(int64 after_msecs_since_epoch) const override;
  Data PreviousTransition(int64 before_msec_since_epoch) const override;

  String GetSystemTimeZoneId() const override;

  Array<String> AvailableTimeZoneIds() const override;

  NSTimeZone* GetNsTimeZone() const;

 private:
  void Init(const String& zone_id);

  NSTimeZone* ns_time_zone_;
};

#endif // FUN_PLATFORM_MAC


#if FUN_PLATFORM == FUN_PLATFORM_ANDROID

class AndroidTimeZoneImpl : public TimeZoneImpl {
 public:
  AndroidTimeZoneImpl();
  AndroidTimeZoneImpl(const String& iana_id);
  AndroidTimeZoneImpl(const AndroidTimeZoneImpl& other);
  ~AndroidTimeZoneImpl();

  AndroidTimeZoneImpl* Clone() const override;

  String GetDisplayName(TimeZone::TimeType time_type,
                        TimeZone::NameType name_type,
                        const Locale& locale) const override;
  String GetAbbreviation(int64 at_msecs_since_epoch) const override;

  int32 GetOffsetFromUtc(int64 at_msecs_since_epoch) const override;
  int32 GetStandardTimeOffset(int64 at_msecs_since_epoch) const override;
  int32 GetDaylightTimeOffset(int64 at_msecs_since_epoch) const override;

  bool HasDaylightTime() const override;
  bool IsDaylightTime(int64 at_msecs_since_epoch) const override;

  Data GetData(int64 for_msecs_since_epoch) const override;

  bool HasTransitions() const override;
  Data NextTransition(int64 after_msecs_since_epoch) const override;
  Data PreviousTransition(int64 before_msec_since_epoch) const override;

  String GetSystemTimeZoneId() const override;

  Array<String> AvailableTimeZoneIds() const override;

 private:
  void Init(const String& zone_id);

  JNIObjectImpl android_time_zone_;
};

#endif // FUN_PLATFORM_ANDROID

} // namespace fun
