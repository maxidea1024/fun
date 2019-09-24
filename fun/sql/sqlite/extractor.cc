#include "fun/sql/sqlite/extractor.h"
#include "fun/base/exception.h"
#include "fun/date_time_parser.h"
#include "fun/sql/date.h"
#include "fun/sql/lob.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/sqlite/utility.h"
#include "fun/sql/time.h"

#if defined(FUN_UNBUNDLED)
#include <sqlite3.h>
#else
#include "sqlite3.h"
#endif

#include <cstdlib>

using fun::DateTimeParser;

namespace fun {
namespace sql {
namespace sqlite {

Extractor::Extractor(sqlite3_stmt* stmt) : stmt_(stmt) {}

Extractor::~Extractor() {}

bool Extractor::Extract(size_t pos, int32& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, int64& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int64(stmt_, (int)pos);
  return true;
}

#ifndef FUN_LONG_IS_64_BIT
bool Extractor::Extract(size_t pos, long& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, unsigned long& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}
#endif

bool Extractor::Extract(size_t pos, double& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_double(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, String& val) {
  if (IsNull(pos)) {
    return false;
  }

  const char* buf =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt_, (int)pos));
  if (!buf) {
    val.Clear();
  } else {
    val.Assign(buf);
  }
  return true;
}

bool Extractor::Extract(size_t pos, int8& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, uint8& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, int16& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, uint16& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, uint32& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, uint64& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int64(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, bool& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = (0 != sqlite3_column_int(stmt_, (int)pos));
  return true;
}

bool Extractor::Extract(size_t pos, float& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = static_cast<float>(sqlite3_column_double(stmt_, (int)pos));
  return true;
}

bool Extractor::Extract(size_t pos, char& val) {
  if (IsNull(pos)) {
    return false;
  }

  val = sqlite3_column_int(stmt_, (int)pos);
  return true;
}

bool Extractor::Extract(size_t pos, Date& val) {
  if (IsNull(pos)) {
    return false;
  }

  String str;
  Extract(pos, str);
  int tzd;
  DateTime dt = DateTimeParser::Parse(Utility::SQLITE_DATE_FORMAT, str, tzd);
  val = dt;
  return true;
}

bool Extractor::Extract(size_t pos, Time& val) {
  if (IsNull(pos)) {
    return false;
  }

  String str;
  Extract(pos, str);
  int tzd;
  DateTime dt = DateTimeParser::Parse(Utility::SQLITE_TIME_FORMAT, str, tzd);
  val = dt;
  return true;
}

bool Extractor::Extract(size_t pos, DateTime& val) {
  if (IsNull(pos)) {
    return false;
  }

  String dt;
  Extract(pos, dt);
  int tzd;
  DateTimeParser::Parse(dt, val, tzd);
  return true;
}

bool Extractor::Extract(size_t pos, fun::Any& val) {
  return ExtractImpl(pos, val);
}

bool Extractor::Extract(size_t pos, fun::dynamicAny& val) {
  return ExtractImpl(pos, val);
}

bool Extractor::IsNull(size_t pos, size_t) {
  if (pos >= nulls_.size()) {
    nulls_.Resize(pos + 1);
  }

  if (!nulls_[pos].first) {
    nulls_[pos].first = true;
    nulls_[pos].second =
        (SQLITE_NULL == sqlite3_column_type(stmt_, static_cast<int>(pos)));
  }

  return nulls_[pos].second;
}

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
