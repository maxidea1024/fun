#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

#include "private/qcore_mac_p.h"
#include "qstringlist.h"

#include <Foundation/NSTimeZone.h>

#include <qdebug.h>

#include <algorithm>

namespace fun {

MacTimeZoneImpl::MacTimeZoneImpl() : nstz_(nullptr) {
  Init(GetSystemTimeZoneId());
}

MacTimeZoneImpl::MacTimeZoneImpl(const String& iana_id) : nstz_(nullptr) {
  Init(iana_id);
}

MacTimeZoneImpl::MacTimeZoneImpl(const MacTimeZoneImpl& other)
  : TimeZoneImpl(other), nstz_(nullptr) {
  nstz_ = [other.nstz_ copy];
}

MacTimeZoneImpl::~MacTimeZoneImpl() {
  [nstz_ release];
}

MacTimeZoneImpl* MacTimeZoneImpl::Clone() const {
  return new MacTimeZoneImpl(*this);
}

void MacTimeZoneImpl::Init(const String& iana_id) {
  if (AvailableTimeZoneIds().contains(iana_id)) {
    nstz_ = [[NSTimeZone timeZoneWithName:String::FromUtf8(iana_id).ToNSString()] retain];
    if (nstz_) {
      id_ = iana_id;
    }
  }
}

String MacTimeZoneImpl::comment() const {
  return String::FromNSString([nstz_ description]);
}

String MacTimeZoneImpl::GetDisplayName(
        TimeZone::TimeType time_type,
        TimeZone::NameType name_type,
        const Locale& locale) const {
  // TODO Mac doesn't support OffsetName yet so use standard offset name
  if (name_type == TimeZone::OffsetName) {
    const Data now_data = data(QDateTime::currentMSecsSinceEpoch());
    // TODO Cheat for now, assume if has dst the offset if 1 hour
    if (time_type == TimeZone::DaylightTime && HasDaylightTime()) {
      return IsoOffsetFormat(now_data.offset_from_utc + 3600);
    } else {
      return IsoOffsetFormat(now_data.offset_from_utc);
    }
  }

  NSTimeZoneNameStyle style = NSTimeZoneNameStyleStandard;

  switch (name_type) {
    case TimeZone::ShortName:
      if (time_type == TimeZone::DaylightTime) {
        style = NSTimeZoneNameStyleShortDaylightSaving;
      }
      else if (time_type == TimeZone::GenericTime) {
        style = NSTimeZoneNameStyleShortGeneric;
      }
      else {
        style = NSTimeZoneNameStyleShortStandard;
      }
      break;
    case TimeZone::DefaultName:
    case TimeZone::LongName :
      if (time_type == TimeZone::DaylightTime) {
        style = NSTimeZoneNameStyleDaylightSaving;
      }
      else if (time_type == TimeZone::GenericTime) {
        style = NSTimeZoneNameStyleGeneric;
      }
      else {
        style = NSTimeZoneNameStyleStandard;
      }
      break;
    case TimeZone::OffsetName:
      // Unreachable
      break;
  }

  NSString* mac_locale_code = locale.GetName().ToNSString();
  NSLocale* mac_locale = [[NSLocale alloc] initWithLocaleIdentifier:mac_locale_code];
  const String result = String::FromNSString([nstz_ localizedName:style locale:mac_locale]);
  [mac_locale release];
  return result;
}

String MacTimeZoneImpl::GetAbbreviation(int64 at_msecs_since_epoch) const
{
  const NSTimeInterval seconds = at_msecs_since_epoch / 1000.0;
  return String::FromNSString([nstz_ abbreviationForDate:[NSDate dateWithTimeIntervalSince1970:seconds]]);
}

int MacTimeZoneImpl::GetOffsetFromUtc(int64 at_msecs_since_epoch) const
{
  const NSTimeInterval seconds = at_msecs_since_epoch / 1000.0;
  return [nstz_ secondsFromGMTForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

int MacTimeZoneImpl::GetStandardTimeOffset(int64 at_msecs_since_epoch) const
{
  return GetOffsetFromUtc(at_msecs_since_epoch) - GetDaylightTimeOffset(at_msecs_since_epoch);
}

int MacTimeZoneImpl::GetDaylightTimeOffset(int64 at_msecs_since_epoch) const
{
  const NSTimeInterval seconds = at_msecs_since_epoch / 1000.0;
  return [nstz_ daylightSavingTimeOffsetForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

bool MacTimeZoneImpl::HasDaylightTime() const
{
  // TODO No Mac API, assume if has transitions
  return HasTransitions();
}

bool MacTimeZoneImpl::IsDaylightTime(int64 at_msecs_since_epoch) const
{
  const NSTimeInterval seconds = at_msecs_since_epoch / 1000.0;
  return [nstz_ isDaylightSavingTimeForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

TimeZoneImpl::Data
MacTimeZoneImpl::GetData(int64 for_msecs_since_epoch) const
{
  const NSTimeInterval seconds = for_msecs_since_epoch / 1000.0;
  NSDate* date = [NSDate dateWithTimeIntervalSince1970:seconds];
  Data data;
  data.at_msecs_since_epoch = for_msecs_since_epoch;
  data.offset_from_utc = [nstz_ secondsFromGMTForDate:date];
  data.daylight_time_offset = [nstz_ daylightSavingTimeOffsetForDate:date];
  data.offset_from_utc = data.offset_from_utc - data.daylight_time_offset;
  data.abbreviation = String::FromNSString([nstz_ abbreviationForDate:date]);
  return data;
}

bool MacTimeZoneImpl::HasTransitions() const
{
  // TODO No direct Mac API, so return if has next after 1970, i.e. since start of tz
  // TODO Not sure what is returned in event of no transitions, assume will be before requested date
  NSDate* epoch = [NSDate dateWithTimeIntervalSince1970:0];
  const NSDate* date = [nstz_ nextDaylightSavingTimeTransitionAfterDate:epoch];
  const bool result = ([date timeIntervalSince1970] > [epoch timeIntervalSince1970]);
  return result;
}

TimeZoneImpl::Data
MacTimeZoneImpl::NextTransition(int64 after_msecs_since_epoch) const
{
  TimeZoneImpl::Data trans;
  const NSTimeInterval seconds = after_msecs_since_epoch / 1000.0;
  NSDate* next_date = [NSDate dateWithTimeIntervalSince1970:seconds];
  next_date = [nstz_ nextDaylightSavingTimeTransitionAfterDate:next_date];
  const NSTimeInterval next_secs = [next_date timeIntervalSince1970];
  if (next_date == nil || next_secs <= seconds) {
    [next_date release];
    return InvalidData();
  }
  trans.at_msecs_since_epoch = next_secs * 1000;
  trans.offset_from_utc = [nstz_ secondsFromGMTForDate:next_date];
  trans.daylight_time_offset = [nstz_ daylightSavingTimeOffsetForDate:next_date];
  trans.offset_from_utc = trans.offset_from_utc - trans.daylight_time_offset;
  trans.abbreviation = String::FromNSString([nstz_ abbreviationForDate:next_date]);
  return trans;
}

TimeZoneImpl::Data MacTimeZoneImpl::PreviousTransition(int64 before_msecs_since_epoch) const
{
  // The native API only lets us search forward, so we need to find an early-enough start:
  const NSTimeInterval lower_bound = std::numeric_limits<NSTimeInterval>::lowest();
  const int64 end_secs = before_msecs_since_epoch / 1000;
  const int year = 366 * 24 * 3600; // a (long) year, in seconds
  NSTimeInterval prev_secs = end_secs; // sentinel for later check
  NSTimeInterval next_secs = prev_secs - year;
  NSTimeInterval trans_secs = lower_bound; // time at a transition; may be > end_secs

  NSDate* next_date = [NSDate dateWithTimeIntervalSince1970:next_secs];
  next_date = [nstz_ nextDaylightSavingTimeTransitionAfterDate:next_date];
  if (next_date != nil
    && (trans_secs = [next_date timeIntervalSince1970]) < end_secs) {
    // There's a transition within the last year before end_secs:
    next_secs = trans_secs;
  }
  else {
    // Need to start our search earlier:
    next_date = [NSDate dateWithTimeIntervalSince1970:lower_bound];
    next_date = [nstz_ nextDaylightSavingTimeTransitionAfterDate:next_date];
    if (next_date) {
      NSTimeInterval late_secs = next_secs;
      next_secs = [next_date timeIntervalSince1970];
      fun_check(next_secs <= end_secs - year || next_secs == trans_secs);

      // We're looking at the first ever transition for our zone, at
      // next_secs (and our zone *does* have at least one transition).  If
      // it's later than end_secs - year, then we must have found it on the
      // initial check and therefore set trans_secs to the same transition
      // time (which, we can infer here, is >= end_secs).  In this case, we
      // won't enter the binary-chop loop, below.
      //
      // In the loop, next_secs < late_secs < end_secs: we have a transition
      // at next_secs and there is no transition between late_secs and
      // end_secs.  The loop narrows the interval between next_secs and
      // late_secs by looking for a transition after their mid-point; if it
      // finds one < end_secs, next_secs moves to this transition; otherwise,
      // late_secs moves to the mid-point.  This soon enough narrows the gap
      // to within a year, after which walking forward one transition at a
      // time (the "Wind through" loop, below) is good enough.

      // Binary chop to within a year of last transition before end_secs:
      while (next_secs + year < late_secs) {
        // Careful about overflow, not fussy about rounding errors:
        NSTimeInterval middle = next_secs / 2 + late_secs / 2;
        NSDate* split = [NSDate dateWithTimeIntervalSince1970:middle];
        split = [nstz_ nextDaylightSavingTimeTransitionAfterDate:split];
        if (split != nil
          && (trans_secs = [split timeIntervalSince1970]) < end_secs) {
          next_date = split;
          next_secs = trans_secs;
        }
        else {
          late_secs = middle;
        }
      }
      fun_check(next_date != nil);
      // ... and next_secs < end_secs unless first transition ever was >= end_secs.
    } // else: we have no data - prev_secs is still end_secs, next_date is still nil
  }
  // Either next_date is nil or next_secs is at its transition.

  // Wind through remaining transitions (spanning at most a year), one at a time:
  while (next_date != nil && next_secs < end_secs) {
    prev_secs = next_secs;
    next_date = [nstz_ nextDaylightSavingTimeTransitionAfterDate:next_date];
    next_secs = [next_date timeIntervalSince1970];
    if (next_secs <= prev_secs) { // presumably no later data available
      break;
    }
  }
  if (prev_secs < end_secs) { // i.e. we did make it into that while loop
    return GetData(int64(prev_secs * 1e3));
  }

  // No transition data; or first transition later than requested time.
  return InvalidData();
}

String MacTimeZoneImpl::GetSystemTimeZoneId() const
{
  // Reset the cached system tz then return the name
  [NSTimeZone resetSystemTimeZone];
  return String::FromNSString([[NSTimeZone systemTimeZone] name]).toUtf8();
}

Array<String> MacTimeZoneImpl::AvailableTimeZoneIds() const
{
  NSEnumerator *enumerator = [[NSTimeZone knownTimeZoneNames] objectEnumerator];
  String tz_id = String::FromNSString([enumerator nextObject]).toUtf8();

  Array<String> list;
  while (!tz_id.IsEmpty()) {
    list << tz_id;
    tz_id = String::FromNSString([enumerator nextObject]).toUtf8();
  }

  std::sort(list.begin(), list.end());
  list.erase(std::unique(list.begin(), list.end()), list.end());

  return list;
}

NSTimeZone* MacTimeZoneImpl::GetNsTimeZone() const
{
  return nstz_;
}

} // namespace fun
