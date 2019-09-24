#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

#include <unicode/ucal.h>

#include <qdebug.h>
#include <qlist.h>

#include <algorithm>

namespace fun {

// ICU utilities

// Convert TimeType and NameType into ICU UCalendarDisplayNameType
static UCalendarDisplayNameType ucalDisplayNameType(TimeZone::TimeType time_type, TimeZone::NameType name_type) {
  // TODO ICU C UCalendarDisplayNameType does not support full set of C++ TimeZone::EDisplayType
  switch (name_type) {
    case TimeZone::ShortName :
    case TimeZone::OffsetName :
      if (time_type == TimeZone::DaylightTime) {
        return UCAL_SHORT_DST;
      }
      // Includes GenericTime
      return UCAL_SHORT_STANDARD;
    case TimeZone::DefaultName :
    case TimeZone::LongName :
      if (time_type == TimeZone::DaylightTime) {
        return UCAL_DST;
      }
      // Includes GenericTime
      return UCAL_STANDARD;
  }
  return UCAL_STANDARD;
}

// Qt wrapper around ucal_getDefaultTimeZone()
static String ucalDefaultTimeZoneId() {
  int32_t size = 30;
  String result(size, Qt::Uninitialized);
  UErrorCode status = U_ZERO_ERROR;

  // size = ucal_getDefaultTimeZone(result, resultLength, status)
  size = ucal_getDefaultTimeZone(reinterpret_cast<UChar *>(result.data()), size, &status);

  // If overflow, then resize and retry
  if (status == U_BUFFER_OVERFLOW_ERROR) {
    result.resize(size);
    status = U_ZERO_ERROR;
    size = ucal_getDefaultTimeZone(reinterpret_cast<UChar *>(result.data()), size, &status);
  }

  // If successful on first or second go, resize and return
  if (U_SUCCESS(status)) {
    result.resize(size);
    return std::move(result).toUtf8();
  }

  return String();
}

// Qt wrapper around ucal_getTimeZoneDisplayName()
static String ucalTimeZoneDisplayName(UCalendar* ucal, TimeZone::TimeType time_type,
                     TimeZone::NameType name_type,
                     const String& locale_code) {
  int32_t size = 50;
  String result(size, Qt::Uninitialized);
  UErrorCode status = U_ZERO_ERROR;

  // size = ucal_getTimeZoneDisplayName(cal, type, locale, result, resultLength, status)
  size = ucal_getTimeZoneDisplayName(ucal,
                     ucalDisplayNameType(time_type, name_type),
                     locale_code.toUtf8(),
                     reinterpret_cast<UChar*>(result.data()),
                     size,
                     &status);

  // If overflow, then resize and retry
  if (status == U_BUFFER_OVERFLOW_ERROR) {
    result.resize(size);
    status = U_ZERO_ERROR;
    size = ucal_getTimeZoneDisplayName(ucal,
                       ucalDisplayNameType(time_type, name_type),
                       locale_code.toUtf8(),
                       reinterpret_cast<UChar*>(result.data()),
                       size,
                       &status);
  }

  // If successful on first or second go, resize and return
  if (U_SUCCESS(status)) {
    result.resize(size);
    return result;
  }

  return String();
}

// Qt wrapper around ucal_get() for offsets
static bool ucalOffsetsAtTime(UCalendar* ucal_, int64 at_msecs_since_epoch,
                int *utc_offset, int *dst_offset) {
  *utc_offset = 0;
  *dst_offset = 0;

  // Clone the ucal so we don't change the shared object
  UErrorCode status = U_ZERO_ERROR;
  UCalendar* ucal = ucal_clone(ucal_, &status);
  if (!U_SUCCESS(status))
    return false;

  // Set the date to find the offset for
  status = U_ZERO_ERROR;
  ucal_setMillis(ucal, at_msecs_since_epoch, &status);

  int32_t utc = 0;
  if (U_SUCCESS(status)) {
    status = U_ZERO_ERROR;
    // Returns msecs
    utc = ucal_get(ucal, UCAL_ZONE_OFFSET, &status) / 1000;
  }

  int32_t dst = 0;
  if (U_SUCCESS(status)) {
    status = U_ZERO_ERROR;
    // Returns msecs
    dst = ucal_get(ucal, UCAL_DST_OFFSET, &status) / 1000;
  }

  ucal_close(ucal);
  if (U_SUCCESS(status)) {
    *utc_offset = utc;
    *dst_offset = dst;
    return true;
  }
  return false;
}

// ICU Draft api in v50, should be stable in ICU v51. Available in C++ api from ICU v3.8
#if U_ICU_VERSION_MAJOR_NUM == 50
// Qt wrapper around qt_ucal_getTimeZoneTransitionDate & ucal_get
static TimeZoneImpl::Data ucalTimeZoneTransition(UCalendar* ucal_,
                           UTimeZoneTransitionType type,
                           int64 at_msecs_since_epoch) {
  TimeZoneImpl::Data tran = TimeZoneImpl::InvalidData();

  // Clone the ucal so we don't change the shared object
  UErrorCode status = U_ZERO_ERROR;
  UCalendar* ucal = ucal_clone(ucal_, &status);
  if (!U_SUCCESS(status)) {
    return tran;
  }

  // Set the date to find the transition for
  status = U_ZERO_ERROR;
  ucal_setMillis(ucal, at_msecs_since_epoch, &status);

  // Find the transition time
  UDate trans_msecs = 0;
  status = U_ZERO_ERROR;
  bool ok = ucal_getTimeZoneTransitionDate(ucal, type, &trans_msecs, &status);

  // Set the transition time to find the offsets for
  if (U_SUCCESS(status) && ok) {
    status = U_ZERO_ERROR;
    ucal_setMillis(ucal, trans_msecs, &status);
  }

  int32_t utc = 0;
  if (U_SUCCESS(status) && ok) {
    status = U_ZERO_ERROR;
    utc = ucal_get(ucal, UCAL_ZONE_OFFSET, &status) / 1000;
  }

  int32_t dst = 0;
  if (U_SUCCESS(status) && ok) {
    status = U_ZERO_ERROR;
    dst = ucal_get(ucal, UCAL_DST_OFFSET, &status) / 1000;
  }

  ucal_close(ucal);
  if (!U_SUCCESS(status) || !ok) {
    return tran;
  }
  tran.at_msecs_since_epoch = trans_msecs;
  tran.offset_from_utc = utc + dst;
  tran.offset_from_utc = utc;
  tran.daylight_time_offset = dst;
  // TODO No ICU API, use short name instead
  if (dst == 0)
    tran.abbreviation = ucalTimeZoneDisplayName(ucal_, TimeZone::StandardTime,
                          TimeZone::ShortName, Locale().GetName());
  else
    tran.abbreviation = ucalTimeZoneDisplayName(ucal_, TimeZone::DaylightTime,
                          TimeZone::ShortName, Locale().GetName());
  return tran;
}
#endif // U_ICU_VERSION_SHORT

// Convert a uenum to a Array<String>
static Array<String> uenumToIdList(UEnumeration* uenum) {
  Array<String> list;
  int32_t size = 0;
  UErrorCode status = U_ZERO_ERROR;
  // TODO Perhaps use uenum_unext instead?
  String result = uenum_next(uenum, &size, &status);
  while (U_SUCCESS(status) && !result.IsEmpty()) {
    list << result;
    status = U_ZERO_ERROR;
    result = uenum_next(uenum, &size, &status);
  }
  std::sort(list.begin(), list.end());
  list.erase(std::unique(list.begin(), list.end()), list.end());
  return list;
}

// Qt wrapper around ucal_getDSTSavings()
static int ucalDaylightOffset(const String& id) {
  UErrorCode status = U_ZERO_ERROR;
  const int32_t dstMSecs = ucal_getDSTSavings(reinterpret_cast<const UChar *>(id.data()), &status);
  if (U_SUCCESS(status)) {
    return (dstMSecs / 1000);
  } else {
    return 0;
  }
}

// Create the system default time zone
IcuTimeZoneImpl::IcuTimeZoneImpl() : ucal_(nullptr) {
  // TODO No ICU C API to obtain sysem tz, assume default hasn't been changed
  Init(ucalDefaultTimeZoneId());
}

// Create a named time zone
IcuTimeZoneImpl::IcuTimeZoneImpl(const String& iana_id) : ucal_(0) {
  // Need to check validity here as ICu will create a GMT tz if name is invalid
  if (AvailableTimeZoneIds().Contains(iana_id)) {
    Init(iana_id);
  }
}

IcuTimeZoneImpl::IcuTimeZoneImpl(const IcuTimeZoneImpl &other)
  : TimeZoneImpl(other), ucal_(0) {
  // Clone the ucal so we don't close the shared object
  UErrorCode status = U_ZERO_ERROR;
  ucal_ = ucal_clone(other.ucal_, &status);
  if (!U_SUCCESS(status)) {
    id_.clear();
    ucal_ = 0;
  }
}

IcuTimeZoneImpl::~IcuTimeZoneImpl() {
  ucal_close(ucal_);
}

IcuTimeZoneImpl *IcuTimeZoneImpl::Clone() const {
  return new IcuTimeZoneImpl(*this);
}

void IcuTimeZoneImpl::Init(const String& iana_id) {
  id_ = iana_id;

  const String id = String::fromUtf8(id_);
  UErrorCode status = U_ZERO_ERROR;
  //TODO Use UCAL_GREGORIAN for now to match Locale, change to UCAL_DEFAULT once full ICU support
  ucal_ = ucal_open(reinterpret_cast<const UChar *>(id.data()), id.size(),
             Locale().name().toUtf8(), UCAL_GREGORIAN, &status);

  if (!U_SUCCESS(status)) {
    id_.Clear();
    ucal_ = 0;
  }
}

String IcuTimeZoneImpl::GetDisplayName(TimeZone::TimeType time_type,
                     TimeZone::NameType name_type,
                     const Locale &locale) const {
  // Return standard offset format name as ICU C api doesn't support it yet
  if (name_type == TimeZone::OffsetName) {
    const Data now_data = data(QDateTime::currentMSecsSinceEpoch());
    // We can't use transitions reliably to find out right dst offset
    // Instead use dst offset api to try get it if needed
    if (time_type == TimeZone::DaylightTime) {
      return IsoOffsetFormat(now_data.offset_from_utc + ucalDaylightOffset(id_));
    } else {
      return IsoOffsetFormat(now_data.offset_from_utc);
    }
  }
  return ucalTimeZoneDisplayName(ucal_, time_type, name_type, locale.GetName());
}

String IcuTimeZoneImpl::GetAbbreviation(int64 at_msecs_since_epoch) const {
  // TODO No ICU API, use short name instead
  if (IsDaylightTime(at_msecs_since_epoch)) {
    return GetDisplayName(TimeZone::DaylightTime, TimeZone::ShortName, String());
  } else {
    return GetDisplayName(TimeZone::StandardTime, TimeZone::ShortName, String());
  }
}

int32 IcuTimeZoneImpl::GetOffsetFromUtc(int64 at_msecs_since_epoch) const {
  int32 std_offset = 0;
  int32 dst_offset = 0;
  ucalOffsetsAtTime(ucal_, at_msecs_since_epoch, &std_offset, & dst_offset);
  return std_offset + dst_offset;
}

int32 IcuTimeZoneImpl::GetStandardTimeOffset(int64 at_msecs_since_epoch) const {
  int32 std_offset = 0;
  int32 dst_offset = 0;
  ucalOffsetsAtTime(ucal_, at_msecs_since_epoch, &std_offset, & dst_offset);
  return std_offset;
}

int32 IcuTimeZoneImpl::GetDaylightTimeOffset(int64 at_msecs_since_epoch) const {
  int32 std_offset = 0;
  int32 dst_offset = 0;
  ucalOffsetsAtTime(ucal_, at_msecs_since_epoch, &std_offset, & dst_offset);
  return dst_offset;
}

bool IcuTimeZoneImpl::HasDaylightTime() const {
  // TODO No direct ICU C api, work-around below not reliable?  Find a better way?
  return ucalDaylightOffset(id_) != 0;
}

bool IcuTimeZoneImpl::IsDaylightTime(int64 at_msecs_since_epoch) const {
  // Clone the ucal so we don't change the shared object
  UErrorCode status = U_ZERO_ERROR;
  UCalendar* ucal = ucal_clone(ucal_, &status);
  if (!U_SUCCESS(status)) {
    return false;
  }

  // Set the date to find the offset for
  status = U_ZERO_ERROR;
  ucal_setMillis(ucal, at_msecs_since_epoch, &status);

  bool result = false;
  if (U_SUCCESS(status)) {
    status = U_ZERO_ERROR;
    result = ucal_inDaylightTime(ucal, &status);
  }

  ucal_close(ucal);
  return result;
}

TimeZoneImpl::Data IcuTimeZoneImpl::GetData(int64 for_msecs_since_epoch) const {
  // Available in ICU C++ api, and draft C api in v50
  // TODO When v51 released see if api is stable
  TimeZoneImpl::Data data = InvalidData();
#if U_ICU_VERSION_MAJOR_NUM == 50
  data = ucalTimeZoneTransition(ucal_, UCAL_TZ_TRANSITION_PREVIOUS_INCLUSIVE,
                  for_msecs_since_epoch);
#else
  ucalOffsetsAtTime(ucal_, for_msecs_since_epoch, &data.offset_from_utc,
            &data.daylight_time_offset);
  data.offset_from_utc = data.offset_from_utc + data.daylight_time_offset;
  data.abbreviation = GetAbbreviation(for_msecs_since_epoch);
#endif // U_ICU_VERSION_MAJOR_NUM == 50
  data.at_msecs_since_epoch = for_msecs_since_epoch;
  return data;
}

bool IcuTimeZoneImpl::HasTransitions() const {
  // Available in ICU C++ api, and draft C api in v50
  // TODO When v51 released see if api is stable
#if U_ICU_VERSION_MAJOR_NUM == 50
  return true;
#else
  return false;
#endif // U_ICU_VERSION_MAJOR_NUM == 50
}

TimeZoneImpl::Data IcuTimeZoneImpl::NextTransition(int64 after_msecs_since_epoch) const {
  // Available in ICU C++ api, and draft C api in v50
  // TODO When v51 released see if api is stable
#if U_ICU_VERSION_MAJOR_NUM == 50
  return ucalTimeZoneTransition(ucal_, UCAL_TZ_TRANSITION_NEXT, after_msecs_since_epoch);
#else
  FUN_UNUSED(after_msecs_since_epoch)
  return InvalidData();
#endif // U_ICU_VERSION_MAJOR_NUM == 50
}

TimeZoneImpl::Data IcuTimeZoneImpl::PreviousTransition(int64 before_msecs_since_epoch) const {
  // Available in ICU C++ api, and draft C api in v50
  // TODO When v51 released see if api is stable
#if U_ICU_VERSION_MAJOR_NUM == 50
  return ucalTimeZoneTransition(ucal_, UCAL_TZ_TRANSITION_PREVIOUS, before_msecs_since_epoch);
#else
  FUN_UNUSED(before_msecs_since_epoch)
  return InvalidData();
#endif // U_ICU_VERSION_MAJOR_NUM == 50
}

String IcuTimeZoneImpl::GetSystemTimeZoneId() const {
  // No ICU C API to obtain sysem tz
  // TODO Assume default hasn't been changed and is the latests system
  return ucalDefaultTimeZoneId();
}

Array<String> IcuTimeZoneImpl::AvailableTimeZoneIds() const {
  UErrorCode status = U_ZERO_ERROR;
  UEnumeration* uenum = ucal_openTimeZones(&status);
  Array<String> result;
  if (U_SUCCESS(status)) {
    result = uenumToIdList(uenum);
  }
  uenum_close(uenum);
  return result;
}

Array<String> IcuTimeZoneImpl::AvailableTimeZoneIds(Locale::Country country) const {
  const QLatin1String regionCode = QLocalePrivate::countryToCode(country);
  const String regionCodeUtf8 = String(regionCode).toUtf8();
  UErrorCode status = U_ZERO_ERROR;
  UEnumeration* uenum = ucal_openCountryTimeZones(regionCodeUtf8.data(), &status);
  Array<String> result;
  if (U_SUCCESS(status)) {
    result = uenumToIdList(uenum);
  }
  uenum_close(uenum);
  return result;
}

Array<String> IcuTimeZoneImpl::AvailableTimeZoneIds(int offset_from_utc) const {
// TODO Available directly in C++ api but not C api, from 4.8 onwards new filter method works
#if U_ICU_VERSION_MAJOR_NUM >= 49 || (U_ICU_VERSION_MAJOR_NUM == 4 && U_ICU_VERSION_MINOR_NUM == 8)
  UErrorCode status = U_ZERO_ERROR;
  UEnumeration* uenum = ucal_openTimeZoneIDEnumeration(UCAL_ZONE_TYPE_ANY, 0, &offset_from_utc, &status);
  Array<String> result;
  if (U_SUCCESS(status)) {
    result = uenumToIdList(uenum);
  }
  uenum_close(uenum);
  return result;
#else
  return TimeZoneImpl::AvailableTimeZoneIds(offset_from_utc);
#endif
}

} // namespace fun
