#include "fun/sql/sqlite/binder.h"
#include "fun/sql/sqlite/utility.h"
#include "fun/sql/date.h"
#include "fun/sql/time.h"
#include "fun/base/exception.h"
#include "fun/base/date_time_formatter.h"
#include "fun/base/date_time_format.h"
#include <cstdlib>

using fun::DateTimeFormatter;
using fun::DateTimeFormat;

namespace fun {
namespace sql {
namespace sqlite {

Binder::Binder(sqlite3_stmt* stmt) : stmt_(stmt) {}

Binder::~Binder() {}

void Binder::Bind(size_t pos, const int32 & val, Direction dir, const WhenNullCb& null_cb) {
  int rc = sqlite3_bind_int(stmt_, (int)pos, val);
  CheckReturn(rc);
}

void Binder::Bind(size_t pos, const int64 & val, Direction dir, const WhenNullCb& null_cb) {
  int rc = sqlite3_bind_int64(stmt_, (int)pos, val);
  CheckReturn(rc);
}

#ifndef FUN_LONG_IS_64_BIT
void Binder::Bind(size_t pos, const long & val, Direction dir, const WhenNullCb& null_cb) {
  long tmp = static_cast<long>(val);
  int rc = sqlite3_bind_int(stmt_, (int)pos, tmp);
  CheckReturn(rc);
}

void Binder::Bind(size_t pos, const unsigned long & val, Direction dir, const WhenNullCb& null_cb) {
  long tmp = static_cast<long>(val);
  int rc = sqlite3_bind_int(stmt_, (int)pos, tmp);
  CheckReturn(rc);
}
#endif

void Binder::Bind(size_t pos, const double & val, Direction dir, const WhenNullCb& null_cb) {
  int rc = sqlite3_bind_double(stmt_, (int)pos, val);
  CheckReturn(rc);
}

void Binder::Bind(size_t pos, const String& val, Direction dir, const WhenNullCb& null_cb) {
  int rc = sqlite3_bind_text(stmt_, (int)pos, val.c_str(), (int) val.size()*sizeof(char), SQLITE_TRANSIENT);
  CheckReturn(rc);
}

void Binder::Bind(size_t pos, const Date& val, Direction dir, const WhenNullCb& null_cb) {
  DateTime dt(val.Year(), val.Month(), val.Day());
  String str(DateTimeFormatter::Format(dt, Utility::SQLITE_DATE_FORMAT));
  Bind(pos, str, dir, null_cb);
}

void Binder::Bind(size_t pos, const Time& val, Direction dir, const WhenNullCb& null_cb) {
  DateTime dt;
  dt.Assign(dt.Year(), dt.Month(), dt.Day(), val.Hour(), val.Minute(), val.Second());
  String str(DateTimeFormatter::Format(dt, Utility::SQLITE_TIME_FORMAT));
  Bind(pos, str, dir, null_cb);
}

void Binder::Bind(size_t pos, const DateTime& val, Direction dir, const WhenNullCb& null_cb) {
  String dt(DateTimeFormatter::Format(val, DateTimeFormat::ISO8601_FORMAT));
  Bind(pos, dt, dir, null_cb);
}

void Binder::Bind(size_t pos, const NullData&, Direction, const std::type_info& /*bind_type*/) {
  sqlite3_bind_null(stmt_, static_cast<int>(pos));
}

void Binder::CheckReturn(int rc) {
  if (rc != SQLITE_OK) {
    Utility::ThrowException(sqlite3_db_handle(stmt_), rc);
  }
}

} // namespace sqlite
} // namespace sql
} // namespace fun
