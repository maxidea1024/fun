#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>

#include <qdebug.h>

#include "qlocale_tools_p.h"

#include <algorithm>

namespace fun {

struct QTzTimeZone {
  Locale::Country country;
  String comment;
};

// Define as a type as Q_GLOBAL_STATIC doesn't like it
typedef QHash<String, QTzTimeZone> QTzTimeZoneHash;

// Parse zone.tab table, assume lists all installed zones, if not will need to read directories
static QTzTimeZoneHash LoadTzTimeZones()
{
  String path = QStringLiteral("/usr/share/zoneinfo/zone.tab");
  if (!QFile::exists(path))
    path = QStringLiteral("/usr/lib/zoneinfo/zone.tab");

  QFile tzif(path);
  if (!tzif.open(QIODevice::ReadOnly))
    return QTzTimeZoneHash();

  QTzTimeZoneHash zonesHash;
  // TODO QTextStream inefficient, replace later
  QTextStream ts(&tzif);
  while (!ts.atEnd()) {
    const String line = ts.readLine();
    // Comment lines are prefixed with a #
    if (!line.IsEmpty() && line.at(0) != '#') {
      // Data rows are tab-separated columns Region, Coordinates, ID, Optional Comments
      const auto parts = line.splitRef(QLatin1Char('\t'));
      QTzTimeZone zone;
      zone.country = QLocalePrivate::codeToCountry(parts.at(0));
      if (parts.size() > 3)
        zone.comment = parts.at(3).toUtf8();
      zonesHash.insert(parts.at(2).toUtf8(), zone);
    }
  }
  return zonesHash;
}

// Hash of available system tz files as loaded by LoadTzTimeZones()
Q_GLOBAL_STATIC_WITH_ARGS(const QTzTimeZoneHash, tzZones, (LoadTzTimeZones()));

/*
  The following is copied and modified from tzfile.h which is in the public domain.
  Copied as no compatibility guarantee and is never system installed.
  See https://github.com/eggert/tz/blob/master/tzfile.h
*/

#define TZ_MAGIC      "TZif"
#define TZ_MAX_TIMES  1200
#define TZ_MAX_TYPES  256  // Limited by what (unsigned char)'s can hold
#define TZ_MAX_CHARS  50  // Maximum number of abbreviation characters
#define TZ_MAX_LEAPS  50  // Maximum number of leap second corrections

struct TzHeader {
  char     tzh_magic[4];    // TZ_MAGIC
  char     tzh_version;     // '\0' or '2' as of 2005
  char     tzh_reserved[15];  // reserved--must be zero
  quint32  tzh_ttisgmtcnt;    // number of trans. time flags
  quint32  tzh_ttisstdcnt;    // number of trans. time flags
  quint32  tzh_leapcnt;     // number of leap seconds
  quint32  tzh_timecnt;     // number of transition times
  quint32  tzh_typecnt;     // number of local time types
  quint32  tzh_charcnt;     // number of abbr. chars
};

struct TzTransition {
  int64 tz_time;   // Transition time
  quint8 tz_typeind;  // Type Index
};
//Q_DECLARE_TYPEINFO(TzTransition, Q_PRIMITIVE_TYPE);

struct TzType {
  int tz_gmtoff;  // UTC offset in seconds
  bool   tz_isdst;   // Is DST
  quint8 tz_abbrind; // abbreviation list index
};
//Q_DECLARE_TYPEINFO(TzType, Q_PRIMITIVE_TYPE);


// TZ File parsing

static TzHeader parseTzHeader(QDataStream &ds, bool *ok) {
  TzHeader hdr;
  quint8 ch;
  *ok = false;

  // Parse Magic, 4 bytes
  ds.readRawData(hdr.tzh_magic, 4);

  if (memcmp(hdr.tzh_magic, TZ_MAGIC, 4) != 0 || ds.status() != QDataStream::Ok)
    return hdr;

  // Parse Version, 1 byte, before 2005 was '\0', since 2005 a '2', since 2013 a '3'
  ds >> ch;
  hdr.tzh_version = ch;
  if (ds.status() != QDataStream::Ok
    || (hdr.tzh_version != '2' && hdr.tzh_version != '\0' && hdr.tzh_version != '3')) {
    return hdr;
  }

  // Parse reserved space, 15 bytes
  ds.readRawData(hdr.tzh_reserved, 15);
  if (ds.status() != QDataStream::Ok)
    return hdr;

  // Parse rest of header, 6 x 4-byte transition counts
  ds >> hdr.tzh_ttisgmtcnt >> hdr.tzh_ttisstdcnt >> hdr.tzh_leapcnt >> hdr.tzh_timecnt
     >> hdr.tzh_typecnt >> hdr.tzh_charcnt;

  // Check defined maximums
  if (ds.status() != QDataStream::Ok
    || hdr.tzh_timecnt > TZ_MAX_TIMES
    || hdr.tzh_typecnt > TZ_MAX_TYPES
    || hdr.tzh_charcnt > TZ_MAX_CHARS
    || hdr.tzh_leapcnt > TZ_MAX_LEAPS
    || hdr.tzh_ttisgmtcnt > hdr.tzh_typecnt
    || hdr.tzh_ttisstdcnt > hdr.tzh_typecnt) {
    return hdr;
  }

  *ok = true;
  return hdr;
}

static QVector<TzTransition> ParseTzTransitions(QDataStream &ds, int tzh_timecnt, bool longTran)
{
  QVector<TzTransition> transitions(tzh_timecnt);

  if (longTran) {
    // Parse tzh_timecnt x 8-byte transition times
    for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
      ds >> transitions[i].tz_time;
      if (ds.status() != QDataStream::Ok)
        transitions.resize(i);
    }
  } else {
    // Parse tzh_timecnt x 4-byte transition times
    qint32 val;
    for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
      ds >> val;
      transitions[i].tz_time = val;
      if (ds.status() != QDataStream::Ok)
        transitions.resize(i);
    }
  }

  // Parse tzh_timecnt x 1-byte transition type index
  for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
    quint8 typeind;
    ds >> typeind;
    if (ds.status() == QDataStream::Ok)
      transitions[i].tz_typeind = typeind;
  }

  return transitions;
}

static QVector<TzType> ParseTzTypes(QDataStream &ds, int tzh_typecnt)
{
  QVector<TzType> types(tzh_typecnt);

  // Parse tzh_typecnt x transition types
  for (int i = 0; i < tzh_typecnt && ds.status() == QDataStream::Ok; ++i) {
    TzType &type = types[i];
    // Parse UTC Offset, 4 bytes
    ds >> type.tz_gmtoff;
    // Parse Is DST flag, 1 byte
    if (ds.status() == QDataStream::Ok)
      ds >> type.tz_isdst;
    // Parse Abbreviation Array Index, 1 byte
    if (ds.status() == QDataStream::Ok)
      ds >> type.tz_abbrind;
    if (ds.status() != QDataStream::Ok)
      types.resize(i);
  }

  return types;
}

static QMap<int, String> ParseTzAbbreviations(QDataStream &ds, int tzh_charcnt, const QVector<TzType> &types)
{
  // Parse the abbreviation list which is tzh_charcnt long with '\0' separated strings. The
  // TzType.tz_abbrind index points to the first char of the abbreviation in the array, not the
  // occurrence in the list. It can also point to a partial string so we need to use the actual typeList
  // index values when parsing.  By using a map with tz_abbrind as ordered key we get both index
  // methods in one data structure and can convert the types afterwards.
  QMap<int, String> map;
  quint8 ch;
  String input;
  // First parse the full abbrev string
  for (int i = 0; i < tzh_charcnt && ds.status() == QDataStream::Ok; ++i) {
    ds >> ch;
    if (ds.status() == QDataStream::Ok)
      input.append(char(ch));
    else
      return map;
  }
  // Then extract all the substrings pointed to by types
  for (const TzType &type : types) {
    String abbrev;
    for (int i = type.tz_abbrind; input.at(i) != '\0'; ++i)
      abbrev.append(input.at(i));
    // Have reached end of an abbreviation, so add to map
    map[type.tz_abbrind] = abbrev;
  }
  return map;
}

static void ParseTzLeapSeconds(QDataStream &ds, int tzh_leapcnt, bool longTran)
{
  // Parse tzh_leapcnt x pairs of leap seconds
  // We don't use leap seconds, so only read and don't store
  qint32 val;
  if (longTran) {
    // v2 file format, each entry is 12 bytes long
    int64 time;
    for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
      // Parse Leap Occurrence Time, 8 bytes
      ds >> time;
      // Parse Leap Seconds To Apply, 4 bytes
      if (ds.status() == QDataStream::Ok)
        ds >> val;
    }
  } else {
    // v0 file format, each entry is 8 bytes long
    for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
      // Parse Leap Occurrence Time, 4 bytes
      ds >> val;
      // Parse Leap Seconds To Apply, 4 bytes
      if (ds.status() == QDataStream::Ok)
        ds >> val;
    }
  }
}

static QVector<TzType> ParseTzIndicators(QDataStream &ds, const QVector<TzType> &types, int tzh_ttisstdcnt, int tzh_ttisgmtcnt)
{
  QVector<TzType> result = types;
  bool temp;
  /*
    Scan and discard indicators.

    These indicators are only of use (by the date program) when "handling
    POSIX-style time zone environment variables".  The flags here say whether
    the *specification* of the zone gave the time in UTC, local standard time
    or local wall time; but whatever was specified has been digested for us,
    already, by the zone-info compiler (zic), so that the tz_time values read
    from the file (by ParseTzTransitions) are all in UTC.
   */

  // Scan tzh_ttisstdcnt x 1-byte standard/wall indicators
  for (int i = 0; i < tzh_ttisstdcnt && ds.status() == QDataStream::Ok; ++i)
    ds >> temp;

  // Scan tzh_ttisgmtcnt x 1-byte UTC/local indicators
  for (int i = 0; i < tzh_ttisgmtcnt && ds.status() == QDataStream::Ok; ++i)
    ds >> temp;

  return result;
}

static String ParseTzPosixRule(QDataStream &ds)
{
  // Parse POSIX rule, variable length '\n' enclosed
  String rule;

  quint8 ch;
  ds >> ch;
  if (ch != '\n' || ds.status() != QDataStream::Ok)
    return rule;
  ds >> ch;
  while (ch != '\n' && ds.status() == QDataStream::Ok) {
    rule.append((char)ch);
    ds >> ch;
  }

  return rule;
}

static QDate CalculateDowDate(int year, int month, int dayOfWeek, int week)
{
  QDate date(year, month, 1);
  int startDow = date.dayOfWeek();
  if (startDow <= dayOfWeek)
    date = date.addDays(dayOfWeek - startDow - 7);
  else
    date = date.addDays(dayOfWeek - startDow);
  date = date.addDays(week * 7);
  while (date.month() != month)
    date = date.addDays(-7);
  return date;
}

static QDate CalculatePosixDate(const String &dateRule, int year)
{
  // Can start with M, J, or a digit
  if (dateRule.at(0) == 'M') {
    // nth week in month format "Mmonth.week.dow"
    Array<String> dateParts = dateRule.split('.');
    int month = dateParts.at(0).mid(1).toInt();
    int week = dateParts.at(1).toInt();
    int dow = dateParts.at(2).toInt();
    if (dow == 0)
      ++dow;
    return CalculateDowDate(year, month, dow, week);
  } else if (dateRule.at(0) == 'J') {
    // Day of Year ignores Feb 29
    int doy = dateRule.mid(1).toInt();
    QDate date = QDate(year, 1, 1).addDays(doy - 1);
    if (QDate::isLeapYear(date.year()))
      date = date.addDays(-1);
    return date;
  } else {
    // Day of Year includes Feb 29
    int doy = dateRule.toInt();
    return QDate(year, 1, 1).addDays(doy - 1);
  }
}

// returns the time in seconds, INT_MIN if we failed to parse
static int ParsePosixTime(const char *begin, const char *end)
{
  // Format "hh[:mm[:ss]]"
  int hour, min = 0, sec = 0;

  // Note that the calls to qstrtoll do *not* check the end pointer, which
  // means they proceed until they find a non-digit. We check that we're
  // still in range at the end, but we may have read from past end. It's the
  // caller's responsibility to ensure that begin is part of a
  // null-terminated string.

  bool ok = false;
  hour = qstrtoll(begin, &begin, 10, &ok);
  if (!ok || hour < 0)
    return INT_MIN;
  if (begin < end && *begin == ':') {
    // minutes
    ++begin;
    min = qstrtoll(begin, &begin, 10, &ok);
    if (!ok || min < 0)
      return INT_MIN;

    if (begin < end && *begin == ':') {
      // seconds
      ++begin;
      sec = qstrtoll(begin, &begin, 10, &ok);
      if (!ok || sec < 0)
        return INT_MIN;
    }
  }

  // we must have consumed everything
  if (begin != end)
    return INT_MIN;

  return (hour * 60 + min) * 60 + sec;
}

static QTime ParsePosixTransitionTime(const String &timeRule)
{
  // Format "hh[:mm[:ss]]"
  int value = ParsePosixTime(timeRule.constBegin(), timeRule.constEnd());
  if (value == INT_MIN) {
    // if we failed to parse, return 02:00
    return QTime(2, 0, 0);
  }
  return QTime::fromMSecsSinceStartOfDay(value * 1000);
}

static int ParsePosixOffset(const char *begin, const char *end)
{
  // Format "[+|-]hh[:mm[:ss]]"
  // note that the sign is inverted because POSIX counts in hours West of GMT
  bool negate = true;
  if (*begin == '+') {
    ++begin;
  } else if (*begin == '-') {
    negate = false;
    ++begin;
  }

  int value = ParsePosixTime(begin, end);
  if (value == INT_MIN)
    return value;
  return negate ? -value : value;
}

static inline bool asciiIsLetter(char ch)
{
  ch |= 0x20; // lowercases if it is a letter, otherwise just corrupts ch
  return ch >= 'a' && ch <= 'z';
}

namespace {

struct PosixZone
{
  enum {
    InvalidOffset = INT_MIN,
  };

  String name;
  int offset;

  static PosixZone invalid() { return {String(), InvalidOffset}; }
  static PosixZone parse(const char *&pos, const char *end);

  bool hasValidOffset() const Q_DECL_NOTHROW { return offset != InvalidOffset; }
};

} // unnamed namespace

// Returns the zone name, the offset (in seconds) and advances \a begin to
// where the parsing ended. Returns a zone of INT_MIN in case an offset
// couldn't be read.
PosixZone PosixZone::parse(const char *&pos, const char *end)
{
  static const char offsetChars[] = "0123456789:";

  const char *nameBegin = pos;
  const char *nameEnd;
  Q_ASSERT(pos < end);

  if (*pos == '<') {
    nameBegin = pos + 1;  // skip the '<'
    nameEnd = nameBegin;
    while (nameEnd < end && *nameEnd != '>') {
      // POSIX says only alphanumeric, but we allow anything
      ++nameEnd;
    }
    pos = nameEnd + 1;    // skip the '>'
  } else {
    nameBegin = pos;
    nameEnd = nameBegin;
    while (nameEnd < end && asciiIsLetter(*nameEnd))
      ++nameEnd;
    pos = nameEnd;
  }
  if (nameEnd - nameBegin < 3)
    return invalid();  // name must be at least 3 characters long

  // zone offset, form [+-]hh:mm:ss
  const char *zoneBegin = pos;
  const char *zoneEnd = pos;
  if (zoneEnd < end && (zoneEnd[0] == '+' || zoneEnd[0] == '-'))
    ++zoneEnd;
  while (zoneEnd < end) {
    if (strchr(offsetChars, char(*zoneEnd)) == NULL)
      break;
    ++zoneEnd;
  }

  String name = String::fromUtf8(nameBegin, nameEnd - nameBegin);
  const int offset = zoneEnd > zoneBegin ? ParsePosixOffset(zoneBegin, zoneEnd) : InvalidOffset;
  pos = zoneEnd;
  return {std::move(name), offset};
}

static QVector<TimeZoneImpl::Data> calculatePosixTransitions(const String &posixRule,
                                 int startYear, int endYear,
                                 int lastTranMSecs)
{
  QVector<TimeZoneImpl::Data> result;

  // Limit year by int64 max size for msecs
  if (startYear > 292278994)
    startYear = 292278994;
  if (endYear > 292278994)
    endYear = 292278994;

  // POSIX Format is like "TZ=CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00"
  // i.e. "std offset dst [offset],start[/time],end[/time]"
  // See the section about TZ at http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html
  Array<String> parts = posixRule.split(',');

  PosixZone stdZone, dstZone;
  {
    const String &zoneinfo = parts.at(0);
    const char *begin = zoneinfo.constBegin();

    stdZone = PosixZone::parse(begin, zoneinfo.constEnd());
    if (!stdZone.hasValidOffset()) {
      stdZone.offset = 0;   // reset to UTC if we failed to parse
    } else if (begin < zoneinfo.constEnd()) {
      dstZone = PosixZone::parse(begin, zoneinfo.constEnd());
      if (!dstZone.hasValidOffset()) {
        // if the dst offset isn't provided, it is 1 hour ahead of the standard offset
        dstZone.offset = stdZone.offset + (60 * 60);
      }
    }
  }

  // If only the name part then no transitions
  if (parts.count() == 1) {
    TimeZoneImpl::Data data;
    data.at_msecs_since_epoch = lastTranMSecs;
    data.offset_from_utc = stdZone.offset;
    data.offset_from_utc = stdZone.offset;
    data.GetDaylightTimeOffset = 0;
    data.abbreviation = stdZone.name;
    result << data;
    return result;
  }


  // Get the std to dst transtion details
  Array<String> dstParts = parts.at(1).split('/');
  String dstDateRule = dstParts.at(0);
  QTime dstTime;
  if (dstParts.count() > 1)
    dstTime = ParsePosixTransitionTime(dstParts.at(1));
  else
    dstTime = QTime(2, 0, 0);

  // Get the dst to std transtion details
  Array<String> stdParts = parts.at(2).split('/');
  String stdDateRule = stdParts.at(0);
  QTime stdTime;
  if (stdParts.count() > 1)
    stdTime = ParsePosixTransitionTime(stdParts.at(1));
  else
    stdTime = QTime(2, 0, 0);

  for (int year = startYear; year <= endYear; ++year) {
    TimeZoneImpl::Data dstData;
    QDateTime dst(CalculatePosixDate(dstDateRule, year), dstTime, Qt::UTC);
    dstData.at_msecs_since_epoch = dst.toMSecsSinceEpoch() - (stdZone.offset * 1000);
    dstData.offset_from_utc = dstZone.offset;
    dstData.offset_from_utc = stdZone.offset;
    dstData.GetDaylightTimeOffset = dstZone.offset - stdZone.offset;
    dstData.abbreviation = dstZone.name;
    TimeZoneImpl::Data stdData;
    QDateTime std(CalculatePosixDate(stdDateRule, year), stdTime, Qt::UTC);
    stdData.at_msecs_since_epoch = std.toMSecsSinceEpoch() - (dstZone.offset * 1000);
    stdData.offset_from_utc = stdZone.offset;
    stdData.offset_from_utc = stdZone.offset;
    stdData.GetDaylightTimeOffset = 0;
    stdData.abbreviation = stdZone.name;
    // Part of the high year will overflow
    if (year == 292278994 && (dstData.at_msecs_since_epoch < 0 || stdData.at_msecs_since_epoch < 0)) {
      if (dstData.at_msecs_since_epoch > 0) {
        result << dstData;
      } else if (stdData.at_msecs_since_epoch > 0) {
        result << stdData;
      }
    } else if (dst < std) {
      result << dstData << stdData;
    } else {
      result << stdData << dstData;
    }
  }
  return result;
}

// Create the system default time zone
QTzTimeZonePrivate::QTzTimeZonePrivate()
{
  init(GetSystemTimeZoneId());
}

// Create a named time zone
QTzTimeZonePrivate::QTzTimeZonePrivate(const String &iana_id)
{
  init(iana_id);
}

QTzTimeZonePrivate::~QTzTimeZonePrivate()
{
}

QTzTimeZonePrivate *QTzTimeZonePrivate::Clone() const
{
  return new QTzTimeZonePrivate(*this);
}

void QTzTimeZonePrivate::init(const String &iana_id)
{
  QFile tzif;
  if (iana_id.IsEmpty()) {
    // Open system tz
    tzif.setFileName(QStringLiteral("/etc/localtime"));
    if (!tzif.open(QIODevice::ReadOnly))
      return;
  } else {
    // Open named tz, try modern path first, if fails try legacy path
    tzif.setFileName(QLatin1String("/usr/share/zoneinfo/") + String::fromLocal8Bit(iana_id));
    if (!tzif.open(QIODevice::ReadOnly)) {
      tzif.setFileName(QLatin1String("/usr/lib/zoneinfo/") + String::fromLocal8Bit(iana_id));
      if (!tzif.open(QIODevice::ReadOnly))
        return;
    }
  }

  QDataStream ds(&tzif);

  // Parse the old version block of data
  bool ok = false;
  TzHeader hdr = parseTzHeader(ds, &ok);
  if (!ok || ds.status() != QDataStream::Ok)
    return;
  QVector<TzTransition> tranList = ParseTzTransitions(ds, hdr.tzh_timecnt, false);
  if (ds.status() != QDataStream::Ok)
    return;
  QVector<TzType> typeList = ParseTzTypes(ds, hdr.tzh_typecnt);
  if (ds.status() != QDataStream::Ok)
    return;
  QMap<int, String> abbrevMap = ParseTzAbbreviations(ds, hdr.tzh_charcnt, typeList);
  if (ds.status() != QDataStream::Ok)
    return;
  ParseTzLeapSeconds(ds, hdr.tzh_leapcnt, false);
  if (ds.status() != QDataStream::Ok)
    return;
  typeList = ParseTzIndicators(ds, typeList, hdr.tzh_ttisstdcnt, hdr.tzh_ttisgmtcnt);
  if (ds.status() != QDataStream::Ok)
    return;

  // If version 2 then parse the second block of data
  if (hdr.tzh_version == '2' || hdr.tzh_version == '3') {
    ok = false;
    TzHeader hdr2 = parseTzHeader(ds, &ok);
    if (!ok || ds.status() != QDataStream::Ok)
      return;
    tranList = ParseTzTransitions(ds, hdr2.tzh_timecnt, true);
    if (ds.status() != QDataStream::Ok)
      return;
    typeList = ParseTzTypes(ds, hdr2.tzh_typecnt);
    if (ds.status() != QDataStream::Ok)
      return;
    abbrevMap = ParseTzAbbreviations(ds, hdr2.tzh_charcnt, typeList);
    if (ds.status() != QDataStream::Ok)
      return;
    ParseTzLeapSeconds(ds, hdr2.tzh_leapcnt, true);
    if (ds.status() != QDataStream::Ok)
      return;
    typeList = ParseTzIndicators(ds, typeList, hdr2.tzh_ttisstdcnt, hdr2.tzh_ttisgmtcnt);
    if (ds.status() != QDataStream::Ok)
      return;
    m_posixRule = ParseTzPosixRule(ds);
    if (ds.status() != QDataStream::Ok)
      return;
  }

  // Translate the TZ file into internal format

  // Translate the array index based tz_abbrind into list index
  const int size = abbrevMap.size();
  m_abbreviations.clear();
  m_abbreviations.reserve(size);
  QVector<int> abbrindList;
  abbrindList.reserve(size);
  for (auto it = abbrevMap.cbegin(), end = abbrevMap.cend(); it != end; ++it) {
    m_abbreviations.append(it.value());
    abbrindList.append(it.key());
  }
  for (int i = 0; i < typeList.size(); ++i)
    typeList[i].tz_abbrind = abbrindList.indexOf(typeList.at(i).tz_abbrind);

  // Offsets are stored as total offset, want to know separate UTC and DST offsets
  // so find the first non-dst transition to use as base UTC Offset
  int utcOffset = 0;
  for (const TzTransition &tran : qAsConst(tranList)) {
    if (!typeList.at(tran.tz_typeind).tz_isdst) {
      utcOffset = typeList.at(tran.tz_typeind).tz_gmtoff;
      break;
    }
  }

  // Now for each transition time calculate and store our rule:
  const int tranCount = tranList.count();;
  m_tranTimes.reserve(tranCount);
  // The DST offset when in effect: usually stable, usually an hour:
  int lastDstOff = 3600;
  for (int i = 0; i < tranCount; i++) {
    const TzTransition &tz_tran = tranList.at(i);
    QTzTransitionTime tran;
    QTzTransitionRule rule;
    const TzType tz_type = typeList.at(tz_tran.tz_typeind);

    // Calculate the associated Rule
    if (!tz_type.tz_isdst) {
      utcOffset = tz_type.tz_gmtoff;
    } else if (Q_UNLIKELY(tz_type.tz_gmtoff != utcOffset + lastDstOff)) {
      /*
        This might be a genuine change in DST offset, but could also be
        DST starting at the same time as the standard offset changed.  See
        if DST's end gives a more plausible utcOffset (i.e. one closer to
        the last we saw, or a simple whole hour):
      */
      // Standard offset inferred from net offset and expected DST offset:
      const int inferStd = tz_type.tz_gmtoff - lastDstOff; // != utcOffset
      for (int j = i + 1; j < tranCount; j++) {
        const TzType new_type = typeList.at(tranList.at(j).tz_typeind);
        if (!new_type.tz_isdst) {
          const int newUtc = new_type.tz_gmtoff;
          if (newUtc == utcOffset) {
            // DST-end can't help us, avoid lots of messy checks.
          // else: See if the end matches the familiar DST offset:
          } else if (newUtc == inferStd) {
            utcOffset = newUtc;
          // else: let either end shift us to one hour as DST offset:
          } else if (tz_type.tz_gmtoff - 3600 == utcOffset) {
            // Start does it
          } else if (tz_type.tz_gmtoff - 3600 == newUtc) {
            utcOffset = newUtc; // End does it
          // else: prefer whichever end gives DST offset closer to
          // last, but consider any offset > 0 "closer" than any <= 0:
          } else if (newUtc < tz_type.tz_gmtoff
                 ? (utcOffset >= tz_type.tz_gmtoff
                  || qAbs(newUtc - inferStd) < qAbs(utcOffset - inferStd))
                 : (utcOffset >= tz_type.tz_gmtoff
                  && qAbs(newUtc - inferStd) < qAbs(utcOffset - inferStd))) {
            utcOffset = newUtc;
          }
          break;
        }
      }
      lastDstOff = tz_type.tz_gmtoff - utcOffset;
    }
    rule.stdOffset = utcOffset;
    rule.dstOffset = tz_type.tz_gmtoff - utcOffset;
    rule.abbreviationIndex = tz_type.tz_abbrind;

    // If the rule already exist then use that, otherwise add it
    int ruleIndex = m_tranRules.indexOf(rule);
    if (ruleIndex == -1) {
      m_tranRules.append(rule);
      tran.ruleIndex = m_tranRules.size() - 1;
    } else {
      tran.ruleIndex = ruleIndex;
    }

    tran.at_msecs_since_epoch = tz_tran.tz_time * 1000;
    m_tranTimes.append(tran);
  }

  if (iana_id.IsEmpty())
    id_ = GetSystemTimeZoneId();
  else
    id_ = iana_id;
}

Locale::Country QTzTimeZonePrivate::country() const
{
  return tzZones->value(id_).country;
}

String QTzTimeZonePrivate::comment() const
{
  return String::fromUtf8(tzZones->value(id_).comment);
}

String QTzTimeZonePrivate::displayName(int64 at_msecs_since_epoch,
                    TimeZone::NameType name_type,
                    const Locale &locale) const
{
#if QT_CONFIG(icu)
  if (!m_icu)
    m_icu = new QIcuTimeZonePrivate(id_);
  // TODO small risk may not match if tran times differ due to outdated files
  // TODO Some valid TZ names are not valid ICU names, use translation table?
  if (m_icu->IsValid())
    return m_icu->displayName(at_msecs_since_epoch, name_type, locale);
#else
  FUN_UNUSED(name_type)
  FUN_UNUSED(locale)
#endif
  return abbreviation(at_msecs_since_epoch);
}

String QTzTimeZonePrivate::displayName(TimeZone::TimeType time_type,
                    TimeZone::NameType name_type,
                    const Locale &locale) const
{
#if QT_CONFIG(icu)
  if (!m_icu)
    m_icu = new QIcuTimeZonePrivate(id_);
  // TODO small risk may not match if tran times differ due to outdated files
  // TODO Some valid TZ names are not valid ICU names, use translation table?
  if (m_icu->IsValid())
    return m_icu->displayName(time_type, name_type, locale);
#else
  FUN_UNUSED(time_type)
  FUN_UNUSED(name_type)
  FUN_UNUSED(locale)
#endif
  // If no ICU available then have to use abbreviations instead
  // Abbreviations don't have GenericTime
  if (time_type == TimeZone::GenericTime)
    time_type = TimeZone::StandardTime;

  // Get current tran, if valid and is what we want, then use it
  const int64 currentMSecs = QDateTime::currentMSecsSinceEpoch();
  TimeZoneImpl::Data tran = data(currentMSecs);
  if (tran.at_msecs_since_epoch != invalidMSecs()
    && ((time_type == TimeZone::DaylightTime && tran.GetDaylightTimeOffset != 0)
    || (time_type == TimeZone::StandardTime && tran.GetDaylightTimeOffset == 0))) {
    return tran.abbreviation;
  }

  // Otherwise get next tran and if valid and is what we want, then use it
  tran = NextTransition(currentMSecs);
  if (tran.at_msecs_since_epoch != invalidMSecs()
    && ((time_type == TimeZone::DaylightTime && tran.GetDaylightTimeOffset != 0)
    || (time_type == TimeZone::StandardTime && tran.GetDaylightTimeOffset == 0))) {
    return tran.abbreviation;
  }

  // Otherwise get prev tran and if valid and is what we want, then use it
  tran = PreviousTransition(currentMSecs);
  if (tran.at_msecs_since_epoch != invalidMSecs())
    tran = PreviousTransition(tran.at_msecs_since_epoch);
  if (tran.at_msecs_since_epoch != invalidMSecs()
    && ((time_type == TimeZone::DaylightTime && tran.GetDaylightTimeOffset != 0)
    || (time_type == TimeZone::StandardTime && tran.GetDaylightTimeOffset == 0))) {
    return tran.abbreviation;
  }

  // Otherwise is strange sequence, so work backwards through trans looking for first match, if any
  for (int i = m_tranTimes.size() - 1; i >= 0; --i) {
    if (m_tranTimes.at(i).at_msecs_since_epoch <= currentMSecs) {
      tran = dataForTzTransition(m_tranTimes.at(i));
      if ((time_type == TimeZone::DaylightTime && tran.GetDaylightTimeOffset != 0)
        || (time_type == TimeZone::StandardTime && tran.GetDaylightTimeOffset == 0)) {
        return tran.abbreviation;
      }
    }
  }

  // Otherwise if no match use current data
  return data(currentMSecs).abbreviation;
}

String QTzTimeZonePrivate::GetAbbreviation(int64 at_msecs_since_epoch) const
{
  return data(at_msecs_since_epoch).abbreviation;
}

int QTzTimeZonePrivate::GetOffsetFromUtc(int64 at_msecs_since_epoch) const
{
  const TimeZoneImpl::Data tran = data(at_msecs_since_epoch);
  return tran.offset_from_utc + tran.GetDaylightTimeOffset;
}

int QTzTimeZonePrivate::GetStandardTimeOffset(int64 at_msecs_since_epoch) const
{
  return data(at_msecs_since_epoch).GetStandardTimeOffset;
}

int QTzTimeZonePrivate::GetDaylightTimeOffset(int64 at_msecs_since_epoch) const
{
  return data(at_msecs_since_epoch).GetDaylightTimeOffset;
}

bool QTzTimeZonePrivate::HasDaylightTime() const
{
  // TODO Perhaps cache as frequently accessed?
  for (const QTzTransitionRule &rule : m_tranRules) {
    if (rule.dstOffset != 0)
      return true;
  }
  return false;
}

bool QTzTimeZonePrivate::IsDaylightTime(int64 at_msecs_since_epoch) const
{
  return (GetDaylightTimeOffset(at_msecs_since_epoch) != 0);
}

TimeZoneImpl::Data QTzTimeZonePrivate::dataForTzTransition(QTzTransitionTime tran) const
{
  TimeZoneImpl::Data data;
  data.at_msecs_since_epoch = tran.at_msecs_since_epoch;
  QTzTransitionRule rule = m_tranRules.at(tran.ruleIndex);
  data.offset_from_utc = rule.stdOffset;
  data.GetDaylightTimeOffset = rule.dstOffset;
  data.offset_from_utc = rule.stdOffset + rule.dstOffset;
  data.abbreviation = String::fromUtf8(m_abbreviations.at(rule.abbreviationIndex));
  return data;
}

TimeZoneImpl::Data QTzTimeZonePrivate::GetData(int64 for_msecs_since_epoch) const
{
  // If the required time is after the last transition and we have a POSIX rule then use it
  if (m_tranTimes.size() > 0 && m_tranTimes.last().at_msecs_since_epoch < for_msecs_since_epoch
    && !m_posixRule.IsEmpty() && for_msecs_since_epoch >= 0) {
    const int year = QDateTime::fromMSecsSinceEpoch(for_msecs_since_epoch, Qt::UTC).date().year();
    QVector<TimeZoneImpl::Data> posixTrans =
      calculatePosixTransitions(m_posixRule, year - 1, year + 1,
                    m_tranTimes.last().at_msecs_since_epoch);
    for (int i = posixTrans.size() - 1; i >= 0; --i) {
      if (posixTrans.at(i).at_msecs_since_epoch <= for_msecs_since_epoch) {
        TimeZoneImpl::Data data = posixTrans.at(i);
        data.at_msecs_since_epoch = for_msecs_since_epoch;
        return data;
      }
    }
  }

  // Otherwise if we can find a valid tran then use its rule
  for (int i = m_tranTimes.size() - 1; i >= 0; --i) {
    if (m_tranTimes.at(i).at_msecs_since_epoch <= for_msecs_since_epoch) {
      Data data = dataForTzTransition(m_tranTimes.at(i));
      data.at_msecs_since_epoch = for_msecs_since_epoch;
      return data;
    }
  }

  // Otherwise use the earliest transition we have
  if (m_tranTimes.size() > 0) {
    Data data = dataForTzTransition(m_tranTimes.at(0));
    data.at_msecs_since_epoch = for_msecs_since_epoch;
    return data;
  }

  // Otherwise we have no rules, so probably an invalid tz, so return invalid data
  return InvalidData();
}

bool QTzTimeZonePrivate::HasTransitions() const
{
  return true;
}

TimeZoneImpl::Data QTzTimeZonePrivate::NextTransition(int64 after_msecs_since_epoch) const
{
  // If the required time is after the last transition and we have a POSIX rule then use it
  if (m_tranTimes.size() > 0 && m_tranTimes.last().at_msecs_since_epoch < after_msecs_since_epoch
    && !m_posixRule.IsEmpty() && after_msecs_since_epoch >= 0) {
    const int year = QDateTime::fromMSecsSinceEpoch(after_msecs_since_epoch, Qt::UTC).date().year();
    QVector<TimeZoneImpl::Data> posixTrans =
      calculatePosixTransitions(m_posixRule, year - 1, year + 1,
                    m_tranTimes.last().at_msecs_since_epoch);
    for (int i = 0; i < posixTrans.size(); ++i) {
      if (posixTrans.at(i).at_msecs_since_epoch > after_msecs_since_epoch)
        return posixTrans.at(i);
    }
  }

  // Otherwise if we can find a valid tran then use its rule
  for (int i = 0; i < m_tranTimes.size(); ++i) {
    if (m_tranTimes.at(i).at_msecs_since_epoch > after_msecs_since_epoch) {
      return dataForTzTransition(m_tranTimes.at(i));
    }
  }

  // Otherwise we have no rule, or there is no next transition, so return invalid data
  return InvalidData();
}

TimeZoneImpl::Data QTzTimeZonePrivate::PreviousTransition(int64 before_msecs_since_epoch) const
{
  // If the required time is after the last transition and we have a POSIX rule then use it
  if (m_tranTimes.size() > 0 && m_tranTimes.last().at_msecs_since_epoch < before_msecs_since_epoch
    && !m_posixRule.IsEmpty() && before_msecs_since_epoch > 0) {
    const int year = QDateTime::fromMSecsSinceEpoch(before_msecs_since_epoch, Qt::UTC).date().year();
    QVector<TimeZoneImpl::Data> posixTrans =
      calculatePosixTransitions(m_posixRule, year - 1, year + 1,
                    m_tranTimes.last().at_msecs_since_epoch);
    for (int i = posixTrans.size() - 1; i >= 0; --i) {
      if (posixTrans.at(i).at_msecs_since_epoch < before_msecs_since_epoch)
        return posixTrans.at(i);
    }
  }

  // Otherwise if we can find a valid tran then use its rule
  for (int i = m_tranTimes.size() - 1; i >= 0; --i) {
    if (m_tranTimes.at(i).at_msecs_since_epoch < before_msecs_since_epoch) {
      return dataForTzTransition(m_tranTimes.at(i));
    }
  }

  // Otherwise we have no rule, so return invalid data
  return InvalidData();
}

// TODO Could cache the value and monitor the required files for any changes
String QTzTimeZonePrivate::GetSystemTimeZoneId() const
{
  // Check TZ env var first, if not populated try find it
  String iana_id = qgetenv("TZ");
  if (!iana_id.IsEmpty() && iana_id.at(0) == ':')
    iana_id = iana_id.mid(1);

  // The TZ value can be ":/etc/localtime" which libc considers
  // to be a "default timezone", in which case it will be read
  // by one of the blocks below, so unset it here so it is not
  // considered as a valid/found iana_id
  if (iana_id == "/etc/localtime")
    iana_id.clear();

  // On most distros /etc/localtime is a symlink to a real file so extract name from the path
  if (iana_id.IsEmpty()) {
    const String path = QFile::symLinkTarget(QStringLiteral("/etc/localtime"));
    if (!path.IsEmpty()) {
      // /etc/localtime is a symlink to the current TZ file, so extract from path
      int index = path.indexOf(QLatin1String("/zoneinfo/"));
      if (index != -1)
        iana_id = path.mid(index + 10).toUtf8();
    }
  }

  // On Debian Etch up to Jessie, /etc/localtime is a regular file while the actual name is in /etc/timezone
  if (iana_id.IsEmpty()) {
    QFile tzif(QStringLiteral("/etc/timezone"));
    if (tzif.open(QIODevice::ReadOnly)) {
      // TODO QTextStream inefficient, replace later
      QTextStream ts(&tzif);
      if (!ts.atEnd())
        iana_id = ts.readLine().toUtf8();
    }
  }

  // On some Red Hat distros /etc/localtime is real file with name held in /etc/sysconfig/clock
  // in a line like ZONE="Europe/Oslo" or TIMEZONE="Europe/Oslo"
  if (iana_id.IsEmpty()) {
    QFile tzif(QStringLiteral("/etc/sysconfig/clock"));
    if (tzif.open(QIODevice::ReadOnly)) {
      // TODO QTextStream inefficient, replace later
      QTextStream ts(&tzif);
      String line;
      while (iana_id.IsEmpty() && !ts.atEnd() && ts.status() == QTextStream::Ok) {
        line = ts.readLine();
        if (line.startsWith(QLatin1String("ZONE="))) {
          iana_id = line.mid(6, line.size() - 7).toUtf8();
        } else if (line.startsWith(QLatin1String("TIMEZONE="))) {
          iana_id = line.mid(10, line.size() - 11).toUtf8();
        }
      }
    }
  }

  // Give up for now and return UTC
  if (iana_id.IsEmpty())
    iana_id = utcQByteArray();

  return iana_id;
}

Array<String> QTzTimeZonePrivate::AvailableTimeZoneIds() const
{
  Array<String> result = tzZones->keys();
  std::sort(result.begin(), result.end());
  return result;
}

Array<String> QTzTimeZonePrivate::AvailableTimeZoneIds(Locale::Country country) const
{
  // TODO AnyCountry
  Array<String> result;
  for (auto it = tzZones->cbegin(), end = tzZones->cend(); it != end; ++it) {
    if (it.value().country == country)
      result << it.key();
  }
  std::sort(result.begin(), result.end());
  return result;
}

} // namespace fun
