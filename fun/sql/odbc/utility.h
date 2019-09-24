#pragma once

#include <sqltypes.h>
#include <map>
#include <sstream>
#include "fun/base/date_time.h"
#include "fun/sql/date.h"
#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/typeinfo.h"
#include "fun/sql/time.h"

namespace fun {
namespace sql {
namespace odbc {

/**
 * Various utility functions
 */
class FUN_ODBC_API Utility {
 public:
  typedef std::map<String, String> DSNMap;
  typedef DSNMap DriverMap;

  static bool IsError(SQLRETURN rc);
  /// Returns true if return code is error

  static DriverMap& drivers(DriverMap& driverMap);
  /// Returns driver-attributes map of available ODBC drivers.

  static DSNMap& dataSources(DSNMap& dsnMap);
  /// Returns DSN-description map of available ODBC data sources.

  template <typename MapType, typename KeyArgType, typename ValueArgType>
  static typename MapType::iterator mapInsert(MapType& m, const KeyArgType& k,
                                              const ValueArgType& v)
  /// Utility map "insert or replace" function (from S. Meyers: Effective STL,
  /// Item 24)
  {
    typename MapType::iterator lb = m.lower_bound(k);
    if (lb != m.end() && !(m.key_comp()(k, lb->first))) {
      lb->second = v;
      return lb;
    } else {
      typedef typename MapType::value_type MVT;
      return m.insert(lb, MVT(k, v));
    }
  }

  static int cDataType(int sqlDataType);
  /// Returns C data type corresponding to supplied sql data type.

  static int sqlDataType(int cDataType);
  /// Returns sql data type corresponding to supplied C data type.

  static void DateSync(Date& dt, const SQL_DATE_STRUCT& ts);
  /// Transfers data from ODBC SQL_DATE_STRUCT to fun::DateTime.

  template <typename T, typename F>
  static void DateSync(T& d, const F& ds)
  /// Transfers data from ODBC SQL_DATE_STRUCT container to fun::DateTime
  /// container.
  {
    size_t size = ds.size();
    if (d.size() != size) d.resize(size);
    typename T::iterator dIt = d.begin();
    typename F::const_iterator it = ds.begin();
    typename F::const_iterator end = ds.end();
    for (; it != end; ++it, ++dIt) DateSync(*dIt, *it);
  }

  static void TimeSync(Time& dt, const SQL_TIME_STRUCT& ts);
  /// Transfers data from ODBC SQL_TIME_STRUCT to fun::DateTime.

  template <typename T, typename F>
  static void TimeSync(T& t, const F& ts)
  /// Transfers data from ODBC SQL_TIME_STRUCT container to fun::DateTime
  /// container.
  {
    size_t size = ts.size();
    if (t.size() != size) t.resize(size);
    typename T::iterator dIt = t.begin();
    typename F::const_iterator it = ts.begin();
    typename F::const_iterator end = ts.end();
    for (; it != end; ++it, ++dIt) TimeSync(*dIt, *it);
  }

  static void DateTimeSync(fun::DateTime& dt, const SQL_TIMESTAMP_STRUCT& ts);
  /// Transfers data from ODBC SQL_TIMESTAMP_STRUCT to fun::DateTime.

  template <typename T, typename F>
  static void DateTimeSync(T& dt, const F& ts)
  /// Transfers data from ODBC SQL_TIMESTAMP_STRUCT container to fun::DateTime
  /// container.
  {
    size_t size = ts.size();
    if (dt.size() != size) dt.resize(size);
    typename T::iterator dIt = dt.begin();
    typename F::const_iterator it = ts.begin();
    typename F::const_iterator end = ts.end();
    for (; it != end; ++it, ++dIt) DateTimeSync(*dIt, *it);
  }

  static void DateSync(SQL_DATE_STRUCT& ts, const Date& dt);
  /// Transfers data from fun::sql::Date to ODBC SQL_DATE_STRUCT.

  template <typename C>
  static void DateSync(std::vector<SQL_DATE_STRUCT>& ds, const C& d)
  /// Transfers data from fun::sql::Date vector to ODBC SQL_DATE_STRUCT
  /// container.
  {
    size_t size = d.size();
    if (ds.size() != size) ds.resize(size);
    std::vector<SQL_DATE_STRUCT>::iterator dIt = ds.begin();
    typename C::const_iterator it = d.begin();
    typename C::const_iterator end = d.end();
    for (; it != end; ++it, ++dIt) DateSync(*dIt, *it);
  }

  static void TimeSync(SQL_TIME_STRUCT& ts, const Time& dt);
  /// Transfers data from fun::sql::Time to ODBC SQL_TIME_STRUCT.

  template <typename C>
  static void TimeSync(std::vector<SQL_TIME_STRUCT>& ts, const C& t)
  /// Transfers data from fun::sql::Time container to ODBC SQL_TIME_STRUCT
  /// vector.
  {
    size_t size = t.size();
    if (ts.size() != size) ts.resize(size);
    std::vector<SQL_TIME_STRUCT>::iterator tIt = ts.begin();
    typename C::const_iterator it = t.begin();
    typename C::const_iterator end = t.end();
    for (; it != end; ++it, ++tIt) TimeSync(*tIt, *it);
  }

  static void DateTimeSync(SQL_TIMESTAMP_STRUCT& ts, const fun::DateTime& dt);
  /// Transfers data from fun::DateTime to ODBC SQL_TIMESTAMP_STRUCT.

  template <typename C>
  static void DateTimeSync(std::vector<SQL_TIMESTAMP_STRUCT>& ts, const C& dt)
  /// Transfers data from fun::DateTime to ODBC SQL_TIMESTAMP_STRUCT.
  {
    size_t size = dt.size();
    if (ts.size() != size) ts.resize(size);
    std::vector<SQL_TIMESTAMP_STRUCT>::iterator tIt = ts.begin();
    typename C::const_iterator it = dt.begin();
    typename C::const_iterator end = dt.end();
    for (; it != end; ++it, ++tIt) DateTimeSync(*tIt, *it);
  }

 private:
  static const TypeInfo data_types_;
  /// C <==> sql data type mapping
};

//
// inlines
//

inline bool Utility::IsError(SQLRETURN rc) { return (0 != (rc & (~1))); }

inline int Utility::cDataType(int sqlDataType) {
  return data_types_.cDataType(sqlDataType);
}

inline int Utility::sqlDataType(int cDataType) {
  return data_types_.sqlDataType(cDataType);
}

inline void Utility::DateSync(Date& d, const SQL_DATE_STRUCT& ts) {
  d.Assign(ts.year, ts.month, ts.day);
}

inline void Utility::TimeSync(Time& t, const SQL_TIME_STRUCT& ts) {
  t.Assign(ts.hour, ts.minute, ts.second);
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
