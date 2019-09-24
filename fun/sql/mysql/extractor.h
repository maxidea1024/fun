#pragma once

#include "fun/sql/mysql/mysql.h"
#include "fun/sql/mysql/statement_executor.h"
#include "fun/sql/mysql/result_metadata.h"
#include "fun/sql/extractor_base.h"
#include "fun/sql/lob.h"
#include "fun/sql/date.h"
#include "fun/sql/time.h"

namespace fun {

namespace Dynamic {
  class Var;
}

namespace sql {
namespace mysql {

/**
 * Extracts and converts data values from the result row returned by MySQL.
 * If NULL is received, the incoming val value is not changed and false is returned
 */
class FUN_MYSQL_API Extractor : public fun::sql::ExtractorBase {
 public:
  typedef SharedPtr<Extractor> Ptr;

  /**
   * Creates the Extractor.
   */
  Extractor(StatementExecutor& st, ResultMetadata& md);

  /**
   * Destroys the Extractor.
   */
  virtual ~Extractor();

  virtual bool Extract(size_t pos, int8& val);
  virtual bool Extract(size_t pos, uint8& val);
  virtual bool Extract(size_t pos, int16& val);
  virtual bool Extract(size_t pos, uint16& val);
  virtual bool Extract(size_t pos, int32& val);
  virtual bool Extract(size_t pos, uint32& val);
  virtual bool Extract(size_t pos, int64& val);
  virtual bool Extract(size_t pos, uint64& val);
#ifndef FUN_LONG_IS_64_BIT
  virtual bool Extract(size_t pos, long& val);
  virtual bool Extract(size_t pos, unsigned long& val);
#endif

  virtual bool Extract(size_t pos, bool& val);
  virtual bool Extract(size_t pos, float& val);
  virtual bool Extract(size_t pos, double& val);
  virtual bool Extract(size_t pos, char& val);
  virtual bool Extract(size_t pos, String& val);
  virtual bool Extract(size_t pos, fun::sql::BLOB& val);
  virtual bool Extract(size_t pos, fun::sql::CLOB& val);
  virtual bool Extract(size_t pos, DateTime& val);
  virtual bool Extract(size_t pos, Date& val);
  virtual bool Extract(size_t pos, Time& val);
  virtual bool Extract(size_t pos, Any& val);
  virtual bool Extract(size_t pos, Dynamic::Var& val);

  /**
   * Returns true if the value at [col,row] position is null.
   */
  virtual bool IsNull(size_t col, size_t row);

  /**
   * Resets any information internally cached by the extractor.
   */
  virtual void Reset();


  //
  // Not implemented Extract functions
  //

  virtual bool Extract(size_t pos, std::vector<int8>& val);
  virtual bool Extract(size_t pos, std::deque<int8>& val);
  virtual bool Extract(size_t pos, std::list<int8>& val);
  virtual bool Extract(size_t pos, std::vector<uint8>& val);
  virtual bool Extract(size_t pos, std::deque<uint8>& val);
  virtual bool Extract(size_t pos, std::list<uint8>& val);
  virtual bool Extract(size_t pos, std::vector<int16>& val);
  virtual bool Extract(size_t pos, std::deque<int16>& val);
  virtual bool Extract(size_t pos, std::list<int16>& val);
  virtual bool Extract(size_t pos, std::vector<uint16>& val);
  virtual bool Extract(size_t pos, std::deque<uint16>& val);
  virtual bool Extract(size_t pos, std::list<uint16>& val);
  virtual bool Extract(size_t pos, std::vector<int32>& val);
  virtual bool Extract(size_t pos, std::deque<int32>& val);
  virtual bool Extract(size_t pos, std::list<int32>& val);
  virtual bool Extract(size_t pos, std::vector<uint32>& val);
  virtual bool Extract(size_t pos, std::deque<uint32>& val);
  virtual bool Extract(size_t pos, std::list<uint32>& val);
  virtual bool Extract(size_t pos, std::vector<int64>& val);
  virtual bool Extract(size_t pos, std::deque<int64>& val);
  virtual bool Extract(size_t pos, std::list<int64>& val);
  virtual bool Extract(size_t pos, std::vector<uint64>& val);
  virtual bool Extract(size_t pos, std::deque<uint64>& val);
  virtual bool Extract(size_t pos, std::list<uint64>& val);

#ifndef FUN_LONG_IS_64_BIT
  virtual bool Extract(size_t pos, std::vector<long>& val);
  virtual bool Extract(size_t pos, std::deque<long>& val);
  virtual bool Extract(size_t pos, std::list<long>& val);
#endif

  virtual bool Extract(size_t pos, std::vector<bool>& val);
  virtual bool Extract(size_t pos, std::deque<bool>& val);
  virtual bool Extract(size_t pos, std::list<bool>& val);
  virtual bool Extract(size_t pos, std::vector<float>& val);
  virtual bool Extract(size_t pos, std::deque<float>& val);
  virtual bool Extract(size_t pos, std::list<float>& val);
  virtual bool Extract(size_t pos, std::vector<double>& val);
  virtual bool Extract(size_t pos, std::deque<double>& val);
  virtual bool Extract(size_t pos, std::list<double>& val);
  virtual bool Extract(size_t pos, std::vector<char>& val);
  virtual bool Extract(size_t pos, std::deque<char>& val);
  virtual bool Extract(size_t pos, std::list<char>& val);
  virtual bool Extract(size_t pos, std::vector<String>& val);
  virtual bool Extract(size_t pos, std::deque<String>& val);
  virtual bool Extract(size_t pos, std::list<String>& val);
  virtual bool Extract(size_t pos, std::vector<BLOB>& val);
  virtual bool Extract(size_t pos, std::deque<BLOB>& val);
  virtual bool Extract(size_t pos, std::list<BLOB>& val);
  virtual bool Extract(size_t pos, std::vector<CLOB>& val);
  virtual bool Extract(size_t pos, std::deque<CLOB>& val);
  virtual bool Extract(size_t pos, std::list<CLOB>& val);
  virtual bool Extract(size_t pos, std::vector<DateTime>& val);
  virtual bool Extract(size_t pos, std::deque<DateTime>& val);
  virtual bool Extract(size_t pos, std::list<DateTime>& val);
  virtual bool Extract(size_t pos, std::vector<Date>& val);

  virtual bool Extract(size_t pos, std::deque<Date>& val);
  virtual bool Extract(size_t pos, std::list<Date>& val);
  virtual bool Extract(size_t pos, std::vector<Time>& val);
  virtual bool Extract(size_t pos, std::deque<Time>& val);
  virtual bool Extract(size_t pos, std::list<Time>& val);
  virtual bool Extract(size_t pos, std::vector<Any>& val);
  virtual bool Extract(size_t pos, std::deque<Any>& val);
  virtual bool Extract(size_t pos, std::list<Any>& val);
  virtual bool Extract(size_t pos, std::vector<Dynamic::Var>& val);
  virtual bool Extract(size_t pos, std::deque<Dynamic::Var>& val);
  virtual bool Extract(size_t pos, std::list<Dynamic::Var>& val);

 private:
  bool RealExtractFixed(size_t pos, enum_field_types type, void* buffer, bool is_unsigned = false);
  bool RealExtractFixedBlob(size_t pos, enum_field_types type, void* buffer, size_t len);

  template <typename T>
  T ExtractAny(size_t pos, bool& success) {
    T value;
    success = Extract(pos, value);
    return value;
  }

  template <typename T>
  bool ExtractToDynamic(size_t pos, T& val);

  // Prevent VC8 warning "operator= could not be generated"
  Extractor& operator=(const Extractor&);

 private:
  StatementExecutor& stmt_;
  ResultMetadata& metadata_;
};

template <typename T>
bool Extractor::ExtractToDynamic(size_t pos, T & val) {
  MetaColumn::ColumnDataType column_type = metadata_.MetaColumnAt(static_cast<uint32>(pos)).Type();

  T result_value;
  bool success = false;

  switch (column_type) {
    case MetaColumn::FDT_BOOL:
      result_value = ExtractAny<bool>(pos, success);
      break;

    case MetaColumn::FDT_INT8:
      result_value = ExtractAny<int8>(pos, success);
      break;

    case MetaColumn::FDT_UINT8:
      result_value = ExtractAny<uint8>(pos, success);
      break;

    case MetaColumn::FDT_INT16:
      result_value = ExtractAny<int16>(pos, success);
      break;

    case MetaColumn::FDT_UINT16:
      result_value = ExtractAny<uint16>(pos, success);
      break;

    case MetaColumn::FDT_INT32:
      result_value = ExtractAny<int32>(pos, success);
      break;

    case MetaColumn::FDT_UINT32:
      result_value = ExtractAny<uint32>(pos, success);
      break;

    case MetaColumn::FDT_INT64:
      result_value = ExtractAny<int64>(pos, success);
      break;

    case MetaColumn::FDT_UINT64:
      result_value = ExtractAny<uint64>(pos, success);
      break;

    case MetaColumn::FDT_FLOAT:
      result_value = ExtractAny<float>(pos, success);
      break;

    case MetaColumn::FDT_DOUBLE:
      result_value = ExtractAny<double>(pos, success);
      break;

    case MetaColumn::FDT_STRING:
      result_value = ExtractAny<String>(pos, success);
      break;

    case MetaColumn::FDT_WSTRING:
      return false;

    case MetaColumn::FDT_BLOB:
      result_value = ExtractAny<String>(pos, success);
      break;

    case MetaColumn::FDT_CLOB:
      result_value = ExtractAny<CLOB>(pos, success);
      break;

    case MetaColumn::FDT_DATE:
      result_value = ExtractAny<Date>(pos, success);
      break;

    case MetaColumn::FDT_TIME:
      result_value = ExtractAny<Time>(pos, success);
      break;

    case MetaColumn::FDT_TIMESTAMP:
      result_value = ExtractAny<DateTime>(pos, success);
      break;

    case MetaColumn::FDT_UNKNOWN:
      return false;
  }

  if (success) {
    val.Swap(result_value);
  }

  return success;
}

} // namespace mysql
} // namespace sql
} // namespace fun
