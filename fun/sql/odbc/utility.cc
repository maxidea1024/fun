#include "fun/sql/odbc/utility.h"
#include <cmath>
#include "fun/base/date_time.h"
#include "fun/base/number_formatter.h"
#include "fun/sql/odbc/handle.h"
#include "fun/sql/odbc/odbc_exception.h"

namespace fun {
namespace sql {
namespace odbc {

const TypeInfo Utility::data_types_;

Utility::DriverMap& Utility::drivers(Utility::DriverMap& driverMap) {
  static const EnvironmentHandle henv;
  const int length = sizeof(SQLCHAR) * 512;

  SQLCHAR desc[length];
  UnsafeMemory::Memset(desc, 0, length);
  SQLSMALLINT len1 = length;
  SQLCHAR attr[length];
  UnsafeMemory::Memset(attr, 0, length);
  SQLSMALLINT len2 = length;
  RETCODE rc = 0;

  if (!Utility::IsError(rc = fun::sql::odbc::SQLDrivers(henv, SQL_FETCH_FIRST,
                                                        desc, length, &len1,
                                                        attr, len2, &len2))) {
    do {
      driverMap.insert(
          DSNMap::value_type(String((char*)desc), String((char*)attr)));
      UnsafeMemory::Memset(desc, 0, length);
      UnsafeMemory::Memset(attr, 0, length);
      len2 = length;
    } while (!Utility::IsError(
        rc = fun::sql::odbc::SQLDrivers(henv, SQL_FETCH_NEXT, desc, length,
                                        &len1, attr, len2, &len2)));
  }

  if (SQL_NO_DATA != rc) {
    throw EnvironmentException(henv);
  }

  return driverMap;
}

Utility::DSNMap& Utility::dataSources(Utility::DSNMap& dsnMap) {
  static const EnvironmentHandle henv;
  const int length = sizeof(SQLCHAR) * 512;
  const int dsnLength = sizeof(SQLCHAR) * (SQL_MAX_DSN_LENGTH + 1);

  SQLCHAR dsn[dsnLength];
  UnsafeMemory::Memset(dsn, 0, dsnLength);
  SQLSMALLINT len1 = sizeof(SQLCHAR) * SQL_MAX_DSN_LENGTH;
  SQLCHAR desc[length];
  UnsafeMemory::Memset(desc, 0, length);
  SQLSMALLINT len2 = length;
  RETCODE rc = 0;

  while (!Utility::IsError(rc = fun::sql::odbc::SQLDataSources(
                               henv, SQL_FETCH_NEXT, dsn, SQL_MAX_DSN_LENGTH,
                               &len1, desc, len2, &len2))) {
    dsnMap.insert(DSNMap::value_type(String((char*)dsn), String((char*)desc)));
    UnsafeMemory::Memset(dsn, 0, dsnLength);
    UnsafeMemory::Memset(desc, 0, length);
    len2 = length;
  }

  if (SQL_NO_DATA != rc) {
    throw EnvironmentException(henv);
  }

  return dsnMap;
}

void Utility::DateTimeSync(fun::DateTime& dt, const SQL_TIMESTAMP_STRUCT& ts) {
  double msec = ts.fraction / 1000000.0;
  double usec = 1000 * (msec - std::floor(msec));

  dt.Assign(ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second,
            (int)std::floor(msec), (int)std::floor(usec));
}

void Utility::DateSync(SQL_DATE_STRUCT& ds, const Date& d) {
  ds.year = static_cast<SQLSMALLINT>(d.Year());
  ds.month = static_cast<SQLUSMALLINT>(d.Month());
  ds.day = static_cast<SQLUSMALLINT>(d.Day());
}

void Utility::TimeSync(SQL_TIME_STRUCT& ts, const Time& t) {
  ts.hour = static_cast<SQLUSMALLINT>(t.Hour());
  ts.minute = static_cast<SQLUSMALLINT>(t.Minute());
  ts.second = static_cast<SQLUSMALLINT>(t.Second());
}

void Utility::DateTimeSync(SQL_TIMESTAMP_STRUCT& ts, const fun::DateTime& dt) {
  ts.year = static_cast<SQLSMALLINT>(dt.Year());
  ts.month = static_cast<SQLUSMALLINT>(dt.Month());
  ts.day = static_cast<SQLUSMALLINT>(dt.Day());
  ts.hour = static_cast<SQLUSMALLINT>(dt.Hour());
  ts.minute = static_cast<SQLUSMALLINT>(dt.Minute());
  ts.second = static_cast<SQLUSMALLINT>(dt.Second());
  // Fraction support is limited to milliseconds due to MS sql Server limitation
  // see http://support.microsoft.com/kb/263872
  ts.fraction = (dt.Millisecond() * 1000000);  // + (dt.Microsecond() * 1000);
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
