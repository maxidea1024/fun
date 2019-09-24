#include "fun/base/time_zone_impl.h"
#include "fun/base/time_zone_impl_data.h"

#ifdef _MSC_VER
#pragma warning(disable : 4244)  // TODO 일단은 막았지만, 코드를 정리하는 쪽으로
                                 // 생각해보자..
#endif

namespace fun {

static const int32 WINDOWS_DATA_TABLE_COUNT = countof(_WINDOWS_DATA_TABLE) - 1;
static const int32 ZONE_DATA_TABLE_COUNT = countof(_ZONE_DATA_TABLE) - 1;
static const int32 UTC_DATA_TABLE_COUNT = countof(_UTC_DATA_TABLE) - 1;

static const ZoneData* _ZoneData(uint16 index) {
  fun_check(index < ZONE_DATA_TABLE_COUNT);
  return &_ZONE_DATA_TABLE[index];
}

static const WindowsData* _WindowsData(uint16 index) {
  fun_check(index < WINDOWS_DATA_TABLE_COUNT);
  return &_WINDOWS_DATA_TABLE[index];
}

static const UtcData* _UtcData(uint16 index) {
  fun_check(index < UTC_DATA_TABLE_COUNT);
  return &_UTC_DATA_TABLE[index];
}

// Return the Windows ID literal for a given WindowsData
static String _WindowsId(const WindowsData* windows_data) {
  return String(_WINDOWS_ID_DATA + windows_data->windows_id_index);
}

// Return the IANA ID literal for a given WindowsData
static String _IanaId(const WindowsData* windows_data) {
  return String(_IANA_ID_DATA + windows_data->iana_id_index);
}

// Return the IANA ID literal for a given ZoneData
static String _IanaId(const ZoneData* zone_data) {
  return String(_IANA_ID_DATA + zone_data->iana_id_index);
}

static String _UtcId(const UtcData* utc_data) {
  return String(_IANA_ID_DATA + utc_data->iana_id_index);
}

static uint16 _ToWindowsIdKey(const String& windows_id) {
  for (uint16 i = 0; i < WINDOWS_DATA_TABLE_COUNT; ++i) {
    const WindowsData* data = _WindowsData(i);
    if (_WindowsId(data) == windows_id) {
      return data->windows_id_key;
    }
  }
  return 0;
}

static String _ToWindowsIdLiteral(uint16 windows_id_key) {
  for (uint16 i = 0; i < WINDOWS_DATA_TABLE_COUNT; ++i) {
    const WindowsData* data = _WindowsData(i);
    if (data->windows_id_key == windows_id_key) {
      return _WindowsId(data);
    }
  }
  return String();
}

//
// TimeZoneImpl
//

TimeZoneImpl::TimeZoneImpl() {}

TimeZoneImpl::TimeZoneImpl(const TimeZoneImpl& rhs) : id_(rhs.id_) {}

TimeZoneImpl::~TimeZoneImpl() {}

TimeZoneImpl* TimeZoneImpl::Clone() const { return new TimeZoneImpl(*this); }

bool TimeZoneImpl::operator==(const TimeZoneImpl& rhs) const {
  return id_ == rhs.id_;
}

bool TimeZoneImpl::operator!=(const TimeZoneImpl& rhs) const {
  return id_ != rhs.id_;
}

bool TimeZoneImpl::IsValid() const { return id_.Len() != 0; }

String TimeZoneImpl::GetId() const { return id_; }

Locale::Country TimeZoneImpl::GetCountry() const {
  for (int32 i = 0; i < ZONE_DATA_TABLE_COUNT; ++i) {
    const ZoneData* data = _ZoneData(i);
    if (_IanaId(data).Split(" ").Contains(id_)) {
      return (Locale::Country)data->country;
    }
  }
  return Locale::AnyCountry;
}

String TimeZoneImpl::GetComment() const { return String(); }

String TimeZoneImpl::GetDisplayName(int64 at_msecs_since_epoch,
                                    TimeZone::NameType name_type,
                                    const Locale& locale) const {
  if (name_type == TimeZone::OffsetName) {
    return IsoOffsetFormat(GetOffsetFromUtc(at_msecs_since_epoch));
  }

  if (IsDaylightTime(at_msecs_since_epoch)) {
    return GetDisplayName(TimeZone::DaylightTime, name_type, locale);
  } else {
    return GetDisplayName(TimeZone::StandardTime, name_type, locale);
  }
}

String TimeZoneImpl::GetDisplayName(TimeZone::TimeType time_type,
                                    TimeZone::NameType name_type,
                                    const Locale& locale) const {
  return String();
}

String TimeZoneImpl::GetAbbreviation(int64 at_msecs_since_epoch) const {
  return String();
}

int32 TimeZoneImpl::GetOffsetFromUtc(int64 at_msecs_since_epoch) const {
  return GetStandardTimeOffset(at_msecs_since_epoch) +
         GetDaylightTimeOffset(at_msecs_since_epoch);
}

int32 TimeZoneImpl::GetStandardTimeOffset(int64 at_msecs_since_epoch) const {
  return InvalidSeconds();
}

int32 TimeZoneImpl::GetDaylightTimeOffset(int64 at_msecs_since_epoch) const {
  return InvalidSeconds();
}

bool TimeZoneImpl::HasDaylightTime() const { return false; }

bool TimeZoneImpl::IsDaylightTime(int64 at_msecs_since_epoch) const {
  return false;
}

TimeZoneImpl::Data TimeZoneImpl::GetData(int64 for_msecs_since_epoch) const {
  return InvalidData();
}

TimeZoneImpl::Data TimeZoneImpl::DataForLocalTime(int64 for_local_msecs,
                                                  int32 hint) const {
  if (!HasDaylightTime()) {  // No DST means same offset for all local msecs
    return GetData(for_local_msecs -
                   GetStandardTimeOffset(for_local_msecs) * 1000);
  }

  const int64 SIXTEEN_HOURS_IN_MSECS(16 * 3600 * 1000);

  if (HasTransitions()) {
    Data tran = PreviousTransition(for_local_msecs - SIXTEEN_HOURS_IN_MSECS);
    fun_check(for_local_msecs < 0 ||  // Pre-epoch TZ info may be unavailable
              for_local_msecs >=
                  tran.at_msecs_since_epoch + tran.offset_from_utc * 1000);
    Data next_tran = NextTransition(tran.at_msecs_since_epoch);
    while (next_tran.at_msecs_since_epoch != InvalidMSecs() &&
           for_local_msecs > next_tran.at_msecs_since_epoch +
                                 next_tran.offset_from_utc * 1000) {
      Data new_tran = NextTransition(next_tran.at_msecs_since_epoch);
      if (new_tran.at_msecs_since_epoch == InvalidMSecs() ||
          (new_tran.at_msecs_since_epoch + new_tran.offset_from_utc * 1000) >
              (for_local_msecs + SIXTEEN_HOURS_IN_MSECS)) {
        // Definitely not a relevant tansition: too far in the future.
        break;
      }
      tran = next_tran;
      next_tran = new_tran;
    }

    if (tran.at_msecs_since_epoch != InvalidMSecs()) {
      fun_check(for_local_msecs < 0 ||
                for_local_msecs >
                    tran.at_msecs_since_epoch + tran.offset_from_utc * 1000);
      const int64 next_start = next_tran.at_msecs_since_epoch;
      // Work out the UTC values it might make sense to return:
      next_tran.at_msecs_since_epoch =
          for_local_msecs - next_tran.offset_from_utc * 1000;
      tran.at_msecs_since_epoch = for_local_msecs - tran.offset_from_utc * 1000;

      const bool next_is_dst = tran.offset_from_utc < next_tran.offset_from_utc;
      // If that agrees with hint > 0, our first guess is to use next_tran; else
      // tran.
      const bool next_first =
          next_is_dst == (hint > 0) && next_start != InvalidMSecs();
      for (int32 i = 0; i < 2; i++) {
        if (next_first ? i == 0 : i) {
          fun_check(next_start != InvalidMSecs());
          if (next_start <= next_tran.at_msecs_since_epoch) {
            return next_tran;
          }
        } else {
          if (next_start == InvalidMSecs() ||
              next_start > tran.at_msecs_since_epoch) {
            return tran;
          }
        }
      }

      const int32 dst_step = next_tran.offset_from_utc - tran.offset_from_utc;
      fun_check(dst_step > 0);  // How else could we get here ?
      if (next_first) {         // hint thought we needed next_tran, so use tran
        tran.at_msecs_since_epoch -= dst_step;
        return tran;
      }
      next_tran.at_msecs_since_epoch += dst_step;
      return next_tran;
    }
    // System has transitions but not for this zone.
    // Try falling back to GetOffsetFromUtc
  }

  // Bracket and refine to discover offset.
  int64 utc_epoch_msecs;

  const int32 early =
      GetOffsetFromUtc(for_local_msecs - SIXTEEN_HOURS_IN_MSECS);
  const int32 late = GetOffsetFromUtc(for_local_msecs + SIXTEEN_HOURS_IN_MSECS);
  if (early == late) {  // > 99% of the time
    utc_epoch_msecs = for_local_msecs - early * 1000;
  } else {
    // Close to a DST transition: early > late is near a fall-back,
    // early < late is near a spring-forward.
    const int32 offset_in_dst = MathBase::Max(early, late);
    const int32 offset_in_std = MathBase::Min(early, late);
    // Candidate values for utc_epoch_msecs (if for_local_msecs is valid):
    const int64 for_dst = for_local_msecs - offset_in_dst * 1000;
    const int64 for_std = for_local_msecs - offset_in_std * 1000;
    // Best guess at the answer:
    const int64 hinted = hint > 0 ? for_dst : for_std;
    if (GetOffsetFromUtc(hinted) ==
        (hint > 0 ? offset_in_dst : offset_in_std)) {
      utc_epoch_msecs = hinted;
    } else if (hint <= 0 && GetOffsetFromUtc(for_dst) == offset_in_dst) {
      utc_epoch_msecs = for_dst;
    } else if (hint > 0 && GetOffsetFromUtc(for_std) == offset_in_std) {
      utc_epoch_msecs = for_std;
    } else {
      // Invalid for_local_msecs: in spring-forward gap.
      const int32 dst_step = GetDaylightTimeOffset(
          early < late ? for_local_msecs + SIXTEEN_HOURS_IN_MSECS
                       : for_local_msecs - SIXTEEN_HOURS_IN_MSECS);
      fun_check(dst_step);  // There can't be a transition without it !
      utc_epoch_msecs = (hint > 0) ? for_std - dst_step : for_dst + dst_step;
    }
  }

  return GetData(utc_epoch_msecs);
}

bool TimeZoneImpl::HasTransitions() const { return false; }

TimeZoneImpl::Data TimeZoneImpl::NextTransition(
    int64 after_msecs_since_epoch) const {
  return InvalidData();
}

TimeZoneImpl::Data TimeZoneImpl::PreviousTransition(
    int64 before_msecs_since_epoch) const {
  return InvalidData();
}

TimeZoneImpl::DataList TimeZoneImpl::GetTransitions(
    int64 from_msecs_since_epoch, int64 to_msecs_since_epoch) const {
  DataList list;
  if (to_msecs_since_epoch >= from_msecs_since_epoch) {
    // from_msecs_since_epoch is inclusive but NextTransitionTime() is exclusive
    // so go back 1 msec
    Data next = NextTransition(from_msecs_since_epoch - 1);
    while (next.at_msecs_since_epoch != InvalidMSecs() &&
           next.at_msecs_since_epoch <= to_msecs_since_epoch) {
      list.Add(next);
      next = NextTransition(next.at_msecs_since_epoch);
    }
  }
  return list;
}

String TimeZoneImpl::GetSystemTimeZoneId() const { return String(); }

Array<String> TimeZoneImpl::AvailableTimeZoneIds() const {
  return Array<String>();
}

Array<String> TimeZoneImpl::AvailableTimeZoneIds(
    Locale::Country country) const {
  fun_check(!"TODO");

  // Default fall-back mode, use the zoneTable to find Region of know Zones
  Array<String> regions;

  // First get all Zones in the Zones table belonging to the Region
  for (int32 i = 0; i < ZONE_DATA_TABLE_COUNT; ++i) {
    if (_ZoneData(i)->country == country) {
      regions += _IanaId(_ZoneData(i)).Split(" ");
    }
  }

  // TODO
  // std::sort(regions.begin(), regions.end());
  // regions.erase(std::unique(regions.begin(), regions.end()), regions.end());

  // Then select just those that are available
  const Array<String> All = AvailableTimeZoneIds();
  Array<String> result;
  // TODO
  // result.reserve(MathBase::Min(All.size(), regions.size()));
  // std::set_intersection(all.begin(), all.end(), regions.cbegin(),
  // regions.cend(), std::back_inserter(result));
  return result;
}

Array<String> TimeZoneImpl::AvailableTimeZoneIds(int32 offset_from_utc) const {
  fun_check(!"TODO");

  // Default fall-back mode, use the zoneTable to find Offset of know Zones
  Array<String> offsets;

  // First get all Zones in the table using the Offset
  for (int32 i = 0; i < WINDOWS_DATA_TABLE_COUNT; ++i) {
    const WindowsData* win_data = _WindowsData(i);

    if (win_data->offset_from_utc == offset_from_utc) {
      for (int32 j = 0; j < ZONE_DATA_TABLE_COUNT; ++j) {
        const ZoneData* data = _ZoneData(j);
        if (data->windows_id_key == win_data->windows_id_key) {
          offsets += _IanaId(data).Split(" ");
        }
      }
    }
  }

  // TODO
  // std::sort(offsets.begin(), offsets.end());
  // offsets.erase(std::unique(offsets.begin(), offsets.end()), offsets.end());

  // Then select just those that are available
  const Array<String> all = AvailableTimeZoneIds();
  Array<String> result;
  // TODO
  // result.reserve(qMin(all.size(), offsets.size()));
  // std::set_intersection(all.begin(), all.end(), offsets.cbegin(),
  // offsets.cend(), std::back_inserter(result));
  return result;
}

TimeZoneImpl::Data TimeZoneImpl::InvalidData() {
  Data data;
  data.at_msecs_since_epoch = InvalidMSecs();
  data.offset_from_utc = InvalidSeconds();
  data.standard_time_offset = InvalidSeconds();
  data.daylight_time_offset = InvalidSeconds();
  return data;
}

TimeZone::OffsetData TimeZoneImpl::InvalidOffsetData() {
  TimeZone::OffsetData offset_data;
  offset_data.at_utc = DateTime();
  offset_data.offset_from_utc = InvalidSeconds();
  offset_data.standard_time_offset = InvalidSeconds();
  offset_data.daylight_time_offset = InvalidSeconds();
  return offset_data;
}

TimeZone::OffsetData TimeZoneImpl::ToOffsetData(const Data& data) {
  TimeZone::OffsetData offset_data = InvalidOffsetData();
  if (data.at_msecs_since_epoch != InvalidMSecs()) {
    offset_data.at_utc = DateTime::FromUtcTicksSinceEpoch(
        data.at_msecs_since_epoch * DateTimeConstants::TICKS_PER_MILLISECOND,
        TimeSpec::UTC);
    offset_data.offset_from_utc = data.offset_from_utc;
    offset_data.standard_time_offset = data.standard_time_offset;
    offset_data.daylight_time_offset = data.daylight_time_offset;
    offset_data.abbreviation = data.abbreviation;
  }
  return offset_data;
}

bool TimeZoneImpl::IsValidId(const String& iana_id) {
  const int MIN_SECTION_LENGTH = 1;
  const int MAX_SECTION_LENGTH = 14;

  int section_len = 0;
  const char* it = *iana_id;
  const char* end = *iana_id + iana_id.Len();
  for (; it != end; ++it, ++section_len) {
    const char ch = *it;
    if (ch == '/') {
      if (section_len < MIN_SECTION_LENGTH ||
          section_len > MAX_SECTION_LENGTH) {
        return false;  // violates (4)
      }
      section_len = -1;
    } else if (ch == '-') {
      if (section_len == 0) {
        return false;  // violates (4)
      }
    } else if (!(ch >= 'a' && ch <= 'z') && !(ch >= 'A' && ch <= 'Z') &&
               !(ch == '_') &&
               !(ch == '.')
               // Should ideally check these only happen as an offset:
               && !(ch >= '0' && ch <= '9') && !(ch == '+') && !(ch == ':')) {
      return false;  // violates (2)
    }
  }

  if (section_len < MIN_SECTION_LENGTH || section_len > MAX_SECTION_LENGTH) {
    return false;  // violates (4)
  }

  return true;
}

String TimeZoneImpl::IsoOffsetFormat(int32 offset_from_utc) {
  const int32 mins = offset_from_utc / 60;
  // TODO
  return String::Format("%c%02d:%02d", mins >= 0 ? '+' : '-',
                        MathBase::Abs(mins) / 60, MathBase::Abs(mins) % 60);
}

String TimeZoneImpl::IanaIdToWindowsId(const String& iana_id) {
  for (int32 i = 0; i < ZONE_DATA_TABLE_COUNT; ++i) {
    const ZoneData* data = _ZoneData(i);

    if (_IanaId(data).Split(" ").Contains(iana_id)) {
      return _ToWindowsIdLiteral(data->windows_id_key);
    }
  }
  return String();
}

String TimeZoneImpl::WindowsIdToDefaultIanaId(const String& windows_id) {
  const uint16 windows_id_key = _ToWindowsIdKey(windows_id);
  for (int32 i = 0; i < WINDOWS_DATA_TABLE_COUNT; ++i) {
    const WindowsData* data = _WindowsData(i);

    if (data->windows_id_key == windows_id_key) {
      return _IanaId(data);
    }
  }
  return String();
}

String TimeZoneImpl::WindowsIdToDefaultIanaId(const String& windows_id,
                                              Locale::Country country) {
  const Array<String> list = WindowsIdToIanaIds(windows_id, country);
  return list.Count() > 0 ? list[0] : String();
}

Array<String> TimeZoneImpl::WindowsIdToIanaIds(const String& windows_id) {
  const uint16 windows_id_key = _ToWindowsIdKey(windows_id);
  Array<String> result;

  for (int32 i = 0; i < ZONE_DATA_TABLE_COUNT; ++i) {
    const ZoneData* data = _ZoneData(i);

    if (data->windows_id_key == windows_id_key) {
      result += _IanaId(data).Split(" ");
    }
  }

  // Return the full list in alpha order
  result.Sort();
  return result;
}

Array<String> TimeZoneImpl::WindowsIdToIanaIds(const String& windows_id,
                                               Locale::Country country) {
  const uint16 windows_id_key = _ToWindowsIdKey(windows_id);
  for (int32 i = 0; i < ZONE_DATA_TABLE_COUNT; ++i) {
    const ZoneData* data = _ZoneData(i);

    // Return the region matches in preference order
    if (data->windows_id_key == windows_id_key &&
        data->country == (uint16)country) {
      return _IanaId(data).Split(" ");
    }
  }

  return Array<String>();
}

//
// Default UTC time zone.
//

UtcTimeZoneImpl::UtcTimeZoneImpl() {
  Init("UTC", 0, "UTC", "UTC", Locale::AnyCountry, "UTC");
}

UtcTimeZoneImpl::UtcTimeZoneImpl(const String& iana_id) {
  for (int32 i = 0; i < UTC_DATA_TABLE_COUNT; ++i) {
    const auto* data = _UtcData(i);
    const auto& id = _UtcId(data);

    if (id == iana_id) {
      Init(id, data->offset_from_utc, id, id, Locale::AnyCountry, id);
    }
  }
}

UtcTimeZoneImpl::UtcTimeZoneImpl(int32 offset_seconds) {
  String id;
  if (offset_seconds != 0) {
    id = IsoOffsetFormat(offset_seconds);
  } else {
    id = "UTC";
  }

  Init(id, offset_seconds, id, id, Locale::AnyCountry, id);
}

UtcTimeZoneImpl::UtcTimeZoneImpl(const String& iana_id, int32 offset_seconds,
                                 const String& name, const String& abbreviation,
                                 Locale::Country country,
                                 const String& comment) {
  Init(iana_id, offset_seconds, name, abbreviation, country, comment);
}

UtcTimeZoneImpl::UtcTimeZoneImpl(const UtcTimeZoneImpl& rhs)
    : Super(rhs),
      name_(rhs.name_),
      abbreviation_(rhs.abbreviation_),
      comment_(rhs.comment_),
      country_(rhs.country_),
      offset_from_utc_(rhs.offset_from_utc_) {}

UtcTimeZoneImpl::~UtcTimeZoneImpl() {}

// super에서는 TimeZoneImpl* 타입으로 처리하는데 말이지?
UtcTimeZoneImpl* UtcTimeZoneImpl::Clone() const {
  return new UtcTimeZoneImpl(*this);
}

TimeZoneImpl::Data UtcTimeZoneImpl::GetData(int64 for_msecs_since_epoch) const {
  Data result;
  result.abbreviation = abbreviation_;
  result.at_msecs_since_epoch = for_msecs_since_epoch;
  result.standard_time_offset = result.offset_from_utc = offset_from_utc_;
  result.daylight_time_offset = 0;
  return result;
}

Locale::Country UtcTimeZoneImpl::GetCountry() const { return country_; }

String UtcTimeZoneImpl::GetComment() const { return comment_; }

String UtcTimeZoneImpl::GetDisplayName(TimeZone::TimeType time_type,
                                       TimeZone::NameType name_type,
                                       const Locale& locale) const {
  if (name_type == TimeZone::ShortName) {
    return abbreviation_;
  } else if (name_type == TimeZone::OffsetName) {
    return IsoOffsetFormat(offset_from_utc_);
  }
  return name_;
}

String UtcTimeZoneImpl::GetAbbreviation(int64 at_msecs_since_epoch) const {
  return abbreviation_;
}

int32 UtcTimeZoneImpl::GetStandardTimeOffset(int64 at_msecs_since_epoch) const {
  (void)at_msecs_since_epoch;  // UNUSED
  return offset_from_utc_;
}

int32 UtcTimeZoneImpl::GetDaylightTimeOffset(int64 at_msecs_since_epoch) const {
  (void)at_msecs_since_epoch;  // UNUSED
  return 0;
}

String UtcTimeZoneImpl::GetSystemTimeZoneId() const { return "UTC"; }

Array<String> UtcTimeZoneImpl::AvailableTimeZoneIds() const {
  Array<String> result;
  result.Reserve(UTC_DATA_TABLE_COUNT);
  for (int32 i = 0; i < UTC_DATA_TABLE_COUNT; ++i) {
    result.Add(_UtcId(_UtcData(i)));
  }
  result.Sort();
  return result;
}

Array<String> UtcTimeZoneImpl::AvailableTimeZoneIds(
    Locale::Country country) const {
  if (country == Locale::AnyCountry) {
    return AvailableTimeZoneIds();
  } else {
    return Array<String>();
  }
}

Array<String> UtcTimeZoneImpl::AvailableTimeZoneIds(
    int32 offset_seconds) const {
  Array<String> result;
  result.Reserve(UTC_DATA_TABLE_COUNT);
  for (int32 i = 0; i < UTC_DATA_TABLE_COUNT; ++i) {
    const auto* data = _UtcData(i);
    if (data->offset_from_utc == offset_seconds) {
      result.Add(_UtcId(data));
    }
  }
  result.Sort();
  return result;
}

void UtcTimeZoneImpl::Init(const String& zone_id) { id_ = zone_id; }

void UtcTimeZoneImpl::Init(const String& zone_id, int32 offset_seconds,
                           const String& name, const String& abbreviation,
                           Locale::Country country, const String& comment) {
  id_ = zone_id;
  offset_from_utc_ = offset_seconds;
  name_ = name;
  abbreviation_ = abbreviation;
  country_ = country;
  comment_ = comment;
}

}  // namespace fun
