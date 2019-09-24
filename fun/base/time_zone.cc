#include "fun/base/time_zone.h"
#include "fun/base/time_zone_impl.h"

// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones

namespace fun {

static TimeZoneImpl* NewBackendTimeZone() {
#if FUN_PLATFORM_WINDOWS_FAMILY && FUN_ENABLE_WINDOWS_TIMEZONE
  return new WinTimeZoneImpl();
#else
  return new UtcTimeZoneImpl();
#endif
}

static TimeZoneImpl* NewBackendTimeZone(const String& iana_id) {
#if FUN_PLATFORM_WINDOWS_FAMILY && FUN_ENABLE_WINDOWS_TIMEZONE
  return new WinTimeZoneImpl(iana_id);
#else
  return new UtcTimeZoneImpl(iana_id);
#endif
}


class TimeZoneSingleton {
 public:
  SharedPtr<TimeZoneImpl> GetBackend()   {
    if (!backend_.IsValid()) {
      backend_ = SharedPtr<TimeZoneImpl>(NewBackendTimeZone());
    }
    return backend_;
  }

 private:
  SharedPtr<TimeZoneImpl> backend_;
};

//TODO 정리 (종료시점에서 소멸 순서로 인해서 문제가 발생할 수도 있음?)
//GLOBAL_STATIC_VAR(TimeZoneSingleton, g_global_timezone);
static TimeZoneSingleton g_global_timezone;


TimeZone::TimeZone() {}

TimeZone::TimeZone(const String& iana_id) {
  impl_ = SharedPtr<TimeZoneImpl>(new UtcTimeZoneImpl(iana_id));

  if (!impl_->IsValid()) {
    impl_ = SharedPtr<TimeZoneImpl>(NewBackendTimeZone(iana_id));
  }
}

TimeZone::TimeZone(const Timespan& offset) {
  const int64 offset_in_second = (int64)offset.TotalSeconds();
  if (offset_in_second >= -50400 && offset_in_second <= 50400) {
    impl_ = SharedPtr<TimeZoneImpl>(new UtcTimeZoneImpl((int32)offset_in_second));
  }
}

TimeZone::TimeZone( const String& iana_id,
                    const Timespan& offset,
                    const String& name,
                    const String& abbreviation,
                    Locale::Country country,
                    const String& comment) {
  if (!IsTimeZoneAvailable(iana_id)) {
    impl_ = SharedPtr<TimeZoneImpl>(new UtcTimeZoneImpl(iana_id, (int32)offset.TotalSeconds(), name, abbreviation, country, comment));
  }
}

TimeZone::TimeZone(const TimeZone& rhs) : impl_(rhs.impl_) {}

TimeZone::~TimeZone() {}

TimeZone& TimeZone::operator = (const TimeZone& rhs) {
  impl_ = rhs.impl_;
  return *this;
}

bool TimeZone::operator == (const TimeZone& rhs) const {
  if (impl_ && rhs.impl_) {
    return *impl_ == *rhs.impl_;
  } else {
    return impl_ == rhs.impl_;
  }
}

bool TimeZone::operator != (const TimeZone& rhs) const {
  if (impl_ && rhs.impl_) {
    return *impl_ != *rhs.impl_;
  } else {
    return impl_ != rhs.impl_;
  }
}

bool TimeZone::IsValid() const {
  return impl_.IsValid() && impl_->IsValid();
}

String TimeZone::GetId() const {
  return impl_.IsValid() ? impl_->GetId() : String();
}

Locale::Country TimeZone::GetCountry() const {
  return impl_.IsValid() ? impl_->GetCountry() : Locale::AnyCountry;
}

String TimeZone::GetComment() const {
  return impl_.IsValid() ? impl_->GetComment() : String();
}

String TimeZone::GetDisplayName( const DateTime& at_date_time,
                                  TimeZone::NameType name_type,
                                  const Locale& locale) const {
  if (IsValid()) {
    return impl_->GetDisplayName(at_date_time.ToUtcTicksSinceEpoch(), name_type, locale);
  } else {
    return String();
  }
}

String TimeZone::GetDisplayName( TimeZone::TimeType time_type,
                                  TimeZone::NameType name_type,
                                  const Locale& locale) const {
  if (IsValid()) {
    return impl_->GetDisplayName(time_type, name_type, locale);
  } else {
    return String();
  }
}

String TimeZone::GetAbbreviation(const DateTime& at_date_time) const {
  if (IsValid()) {
    return impl_->GetAbbreviation(at_date_time.ToUtcTicksSinceEpoch());
  } else {
    return String();
  }
}

Timespan TimeZone::GetOffsetFromUtc(const DateTime& at_date_time) const {
  return Timespan::FromSeconds(IsValid() ? impl_->GetOffsetFromUtc(at_date_time.ToUtcTicksSinceEpoch()) : 0);
}

Timespan TimeZone::GetStandardTimeOffset(const DateTime& at_date_time) const {
  return Timespan::FromSeconds(IsValid() ? impl_->GetStandardTimeOffset(at_date_time.ToUtcTicksSinceEpoch()) : 0);
}

Timespan TimeZone::GetDaylightTimeOffset(const DateTime& at_date_time) const {
  return Timespan::FromSeconds(IsValid() ? impl_->GetDaylightTimeOffset(at_date_time.ToUtcTicksSinceEpoch()) : 0);
}

bool TimeZone::HasDaylightTime() const {
  return IsValid() ? impl_->HasDaylightTime() : false;
}

bool TimeZone::IsDaylightTime(const DateTime& at_date_time) const {
  return IsValid() ? impl_->IsDaylightTime(at_date_time.ToUtcTicksSinceEpoch()) : false;
}

TimeZone::OffsetData TimeZone::GetOffsetData(const DateTime& for_date_time) const {
  if (HasTransitions()) {
    return TimeZoneImpl::ToOffsetData(impl_->GetData(for_date_time.ToUtcTicksSinceEpoch()));
  } else {
    return TimeZoneImpl::InvalidOffsetData();
  }
}

bool TimeZone::HasTransitions() const {
  return IsValid() ? impl_->HasTransitions() : false;
}

TimeZone::OffsetData TimeZone::NextTransition(const DateTime& after_date_time) const {
  if (HasTransitions()) {
    return TimeZoneImpl::ToOffsetData(impl_->NextTransition(after_date_time.ToUtcTicksSinceEpoch()));
  } else {
    return TimeZoneImpl::InvalidOffsetData();
  }
}

TimeZone::OffsetData TimeZone::PreviousTransition(const DateTime& before_date_time) const {
  if (HasTransitions()) {
    return TimeZoneImpl::ToOffsetData(impl_->PreviousTransition(before_date_time.ToUtcTicksSinceEpoch()));
  } else {
    return TimeZoneImpl::InvalidOffsetData();
  }
}

TimeZone::OffsetDataList TimeZone::GetTransitions(const DateTime& from_date_time, const DateTime& to_date_time) const {
  OffsetDataList result;
  if (HasTransitions()) {
    const auto list = impl_->GetTransitions(from_date_time.ToUtcTicksSinceEpoch(), to_date_time.ToUtcTicksSinceEpoch());
    result.Reserve(list.Count());
    for (const auto& data : list) {
      result.Add(TimeZoneImpl::ToOffsetData(data));
    }
  }
  return result;
}

String TimeZone::GetSystemTimeZoneId() {
  return g_global_timezone.GetBackend()->GetSystemTimeZoneId();
}

TimeZone TimeZone::GetSystemTimeZone() {
  return TimeZone(GetSystemTimeZoneId());
}

TimeZone TimeZone::Utc() {
  return TimeZone("UTC");
}

bool TimeZone::IsTimeZoneAvailable(const String& iana_id) {
  if (!TimeZoneImpl::IsValidId(iana_id)) {
    return false;
  }

  return AvailableTimeZoneIds().Contains(iana_id);
}

namespace {

static Array<String> SetUnion(const Array<String>& list1, const Array<String>& list2) {
  Array<String> result;
  for (const auto& value : list1) {
    result.AddUnique(value);
  }

  for (const auto& value : list2) {
    result.AddUnique(value);
  }
  return result;
}

} // namespace

Array<String> TimeZone::AvailableTimeZoneIds() {
  return SetUnion(UtcTimeZoneImpl().AvailableTimeZoneIds(), g_global_timezone.GetBackend()->AvailableTimeZoneIds());
}

Array<String> TimeZone::AvailableTimeZoneIds(Locale::Country country) {
  return SetUnion(UtcTimeZoneImpl().AvailableTimeZoneIds(country), g_global_timezone.GetBackend()->AvailableTimeZoneIds(country));
}

Array<String> TimeZone::AvailableTimeZoneIds(int32 offset_seconds) {
  return SetUnion(UtcTimeZoneImpl().AvailableTimeZoneIds(offset_seconds), g_global_timezone.GetBackend()->AvailableTimeZoneIds(offset_seconds));
}

String TimeZone::IanaIdToWindowsId(const String& iana_id) {
  return TimeZoneImpl::IanaIdToWindowsId(iana_id);
}

String TimeZone::WindowsIdToDefaultIanaId(const String& windows_id) {
  return TimeZoneImpl::WindowsIdToDefaultIanaId(windows_id);
}

String TimeZone::WindowsIdToDefaultIanaId(const String& windows_id, Locale::Country country) {
  return TimeZoneImpl::WindowsIdToDefaultIanaId(windows_id, country);
}

Array<String> TimeZone::WindowsIdToIanaIds(const String& windows_id) {
  return TimeZoneImpl::WindowsIdToIanaIds(windows_id);
}

Array<String> TimeZone::WindowsIdToIanaIds(const String& windows_id, Locale::Country country) {
  return TimeZoneImpl::WindowsIdToIanaIds(windows_id, country);
}

String TimeZone::LocalId() {
  return GetSystemTimeZoneId();
}

TimeZone TimeZone::Local() {
  return GetSystemTimeZone();
}

uint32 HashOf(const TimeZone& timezone) {
  return HashOf(timezone.GetId());
}

Archive& operator & (Archive& ar, TimeZone& timezone) {
  if (ar.IsLoading()) {
    String iana_id;
    ar & iana_id;
    timezone = TimeZone(iana_id);
  } else {
    String iana_id = timezone.GetId();
    ar & iana_id;
  }
  return ar;
}

} // namespace fun
