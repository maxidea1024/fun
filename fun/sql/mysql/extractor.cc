#include "fun/sql/mysql/extractor.h"

#include "fun/base/dynamic/var.h"
#include "fun/sql/date.h"
#include "fun/sql/time.h"

namespace fun {
namespace sql {
namespace mysql {

Extractor::Extractor(StatementExecutor& st, ResultMetadata& md)
    : stmt_(st), metadata_(md) {}

Extractor::~Extractor() {}

bool Extractor::Extract(size_t pos, int8& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_TINY, &val);
}

bool Extractor::Extract(size_t pos, uint8& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_TINY, &val, true);
}

bool Extractor::Extract(size_t pos, int16& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_SHORT, &val);
}

bool Extractor::Extract(size_t pos, uint16& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_SHORT, &val, true);
}

bool Extractor::Extract(size_t pos, int32& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_LONG, &val);
}

bool Extractor::Extract(size_t pos, uint32& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_LONG, &val, true);
}

bool Extractor::Extract(size_t pos, int64& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_LONGLONG, &val);
}

bool Extractor::Extract(size_t pos, uint64& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_LONGLONG, &val, true);
}

#ifndef FUN_LONG_IS_64_BIT
bool Extractor::Extract(size_t pos, long& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_LONG, &val);
}

bool Extractor::Extract(size_t pos, unsigned long& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_LONG, &val, true);
}
#endif

bool Extractor::Extract(size_t pos, bool& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_TINY, &val);
}

bool Extractor::Extract(size_t pos, float& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_FLOAT, &val);
}

bool Extractor::Extract(size_t pos, double& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_DOUBLE, &val);
}

bool Extractor::Extract(size_t pos, char& val) {
  return RealExtractFixed(pos, MYSQL_TYPE_TINY, &val);
}

bool Extractor::Extract(size_t pos, String& val) {
  if (metadata_.ReturnedColumnCount() <= pos) {
    throw MySqlException(
        "Extractor: attempt to Extract more parameters, than query result "
        "contain");
  }

  if (metadata_.IsNull(static_cast<uint32>(pos))) {
    return false;
  }

  // mysql reports TEXT types as FDT_BLOB when being extracted
  MetaColumn::ColumnDataType column_type =
      metadata_.MetaColumnAt(static_cast<uint32>(pos)).Type();
  if (column_type != fun::sql::MetaColumn::FDT_STRING &&
      column_type != fun::sql::MetaColumn::FDT_BLOB) {
    throw MySqlException("Extractor: not a string");
  }

  if (column_type == fun::sql::MetaColumn::FDT_BLOB &&
      metadata_.length(pos) > 0 && metadata_.GetRawData(pos) == NULL) {
    std::vector<char> buffer(metadata_.length(pos), 0);
    bool ret = RealExtractFixedBlob(pos, metadata_.Row()[pos].buffer_type,
                                    buffer.data(), buffer.size());
    if (ret) {
      val.Assign(buffer.data(), buffer.size());
    }

    return ret;
  }

  val.Assign(reinterpret_cast<const char*>(metadata_.rawData(pos)),
             metadata_.length(pos));
  return true;
}

bool Extractor::Extract(size_t pos, fun::sql::BLOB& val) {
  if (metadata_.ReturnedColumnCount() <= pos) {
    throw MySqlException(
        "Extractor: attempt to Extract more parameters, than query result "
        "contain");
  }

  if (metadata_.IsNull(static_cast<uint32>(pos))) {
    return false;
  }

  MetaColumn::ColumnDataType column_type =
      metadata_.MetaColumnAt(static_cast<uint32>(pos)).type();
  if (column_type != fun::sql::MetaColumn::FDT_BLOB) {
    throw MySqlException("Extractor: not a blob");
  }

  if (column_type == fun::sql::MetaColumn::FDT_BLOB &&
      metadata_.length(pos) > 0 && metadata_.rawData(pos) == NULL) {
    std::vector<unsigned char> buffer(metadata_.length(pos), 0);
    bool ret = RealExtractFixedBlob(pos, metadata_.Row()[pos].buffer_type,
                                    buffer.data(), buffer.size());
    if (ret) {
      val.AssignRaw(buffer.data(), buffer.size());
    }

    return ret;
  }

  val.AssignRaw(metadata_.rawData(pos), metadata_.length(pos));
  return true;
}

bool Extractor::Extract(size_t pos, fun::sql::CLOB& val) {
  if (metadata_.ReturnedColumnCount() <= pos) {
    throw MySqlException(
        "Extractor: attempt to Extract more parameters, than query result "
        "contain");
  }

  if (metadata_.IsNull(static_cast<uint32>(pos))) {
    return false;
  }

  MetaColumn::ColumnDataType column_type =
      metadata_.MetaColumnAt(static_cast<uint32>(pos)).type();
  if (column_type != fun::sql::MetaColumn::FDT_BLOB) {
    throw MySqlException("Extractor: not a blob");
  }

  if (column_type == fun::sql::MetaColumn::FDT_BLOB &&
      metadata_.length(pos) > 0 && metadata_.rawData(pos) == NULL) {
    std::vector<char> buffer(metadata_.length(pos), 0);
    bool ret = RealExtractFixedBlob(pos, metadata_.Row()[pos].buffer_type,
                                    buffer.data(), buffer.size());
    if (ret) {
      val.AssignRaw(buffer.data(), buffer.size());
    }

    return ret;
  }

  val.AssignRaw(reinterpret_cast<const char*>(metadata_.rawData(pos)),
                metadata_.length(pos));
  return true;
}

bool Extractor::Extract(size_t pos, DateTime& val) {
  MYSQL_TIME mt = {0};

  if (!RealExtractFixed(pos, MYSQL_TYPE_DATETIME, &mt)) {
    return false;
  }

  val.Assign(mt.year, mt.month, mt.day, mt.hour, mt.minute, mt.second,
             mt.second_part / 1000, mt.second_part % 1000);
  return true;
}

bool Extractor::Extract(size_t pos, Date& val) {
  MYSQL_TIME mt = {0};

  if (!RealExtractFixed(pos, MYSQL_TYPE_DATE, &mt)) {
    return false;
  }

  val.Assign(mt.year, mt.month, mt.day);
  return true;
}

bool Extractor::Extract(size_t pos, Time& val) {
  MYSQL_TIME mt = {0};

  if (!RealExtractFixed(pos, MYSQL_TYPE_TIME, &mt)) {
    return false;
  }

  val.Assign(mt.hour, mt.minute, mt.second);
  return true;
}

bool Extractor::Extract(size_t pos, Any& val) {
  return ExtractToDynamic<Any>(pos, val);
}

bool Extractor::Extract(size_t pos, Dynamic::Var& val) {
  return ExtractToDynamic<Dynamic::Var>(pos, val);
}

bool Extractor::IsNull(size_t col, size_t row) {
  fun_check(row == FUN_DATA_INVALID_ROW);

  if (metadata_.ReturnedColumnCount() <= col) {
    throw MySqlException(
        "Extractor: attempt to Extract more parameters, than query result "
        "contain");
  }

  if (metadata_.IsNull(static_cast<uint32>(col))) {
    return true;
  }

  return false;
}

void Extractor::Reset() { ExtractorBase::Reset(); }

bool Extractor::RealExtractFixed(size_t pos, enum_field_types type,
                                 void* buffer, bool is_unsigned) {
  MYSQL_BIND bind = {0};
  my_bool IsNull = 0;

  bind.is_null = &IsNull;
  bind.buffer_type = type;
  bind.buffer = buffer;
  bind.is_unsigned = is_unsigned;

  if (!stmt_.FetchColumn(pos, &bind)) {
    return false;
  }

  return IsNull == 0;
}

bool Extractor::RealExtractFixedBlob(size_t pos, enum_field_types type,
                                     void* buffer, size_t len) {
  MYSQL_BIND bind = {0};
  my_bool IsNull = 0;

  bind.is_null = &IsNull;
  bind.buffer_type = type;
  bind.buffer = buffer;
  bind.buffer_length = static_cast<unsigned long>(len);

  if (!stmt_.FetchColumn(pos, &bind)) {
    return false;
  }

  return IsNull == 0;
}

//
// Not implemented
//

bool Extractor::Extract(size_t, std::vector<int8>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int8>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int8>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint8>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint8>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint8>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<int16>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int16>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int16>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint16>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint16>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint16>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<int32>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int32>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int32>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint32>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint32>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint32>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<int64>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int64>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int64>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint64>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint64>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint64>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

#ifndef FUN_LONG_IS_64_BIT
bool Extractor::Extract(size_t, std::vector<long>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<long>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<long>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}
#endif

bool Extractor::Extract(size_t, std::vector<bool>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<bool>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<bool>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<float>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<float>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<float>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<double>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<double>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<double>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<char>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<char>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<char>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<String>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<String>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<String>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<BLOB>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<BLOB>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<BLOB>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<CLOB>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<CLOB>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<CLOB>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<DateTime>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<DateTime>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<DateTime>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Date>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Date>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Date>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Time>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Time>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Time>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Any>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Any>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Any>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Dynamic::Var>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Dynamic::Var>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Dynamic::Var>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

}  // namespace mysql
}  // namespace sql
}  // namespace fun
