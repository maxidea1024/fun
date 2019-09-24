#pragma once

#include <utility>
#include <vector>
#include "fun/base/any.h"
#include "fun/base/dynamic_any.h"
#include "fun/sql/constants.h"
#include "fun/sql/date.h"
#include "fun/sql/extractor_base.h"
#include "fun/sql/meta_column.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/sqlite/sqlite.h"
#include "fun/sql/sqlite/utility.h"
#include "fun/sql/time.h"
#include "sqlite3.h"

namespace fun {
namespace sql {
namespace sqlite {

/**
 * Extracts and converts data values form the result row returned by SQLite.
 * If NULL is received, the incoming val value is not changed and false is
 * returned
 */
class FUN_SQLITE_API Extractor : public fun::sql::ExtractorBase {
 public:
  /**
   * Type for null indicators container.
   */
  typedef std::vector<std::pair<bool, bool> > NullIndVec;

  /**
   * Creates the Extractor.
   */
  Extractor(sqlite3_stmt* stmt);

  /**
   * Destroys the Extractor.
   */
  ~Extractor();

  bool Extract(size_t pos, int8& val);
  bool Extract(size_t pos, uint8& val);
  bool Extract(size_t pos, int16& val);
  bool Extract(size_t pos, uint16& val);
  bool Extract(size_t pos, int32& val);
  bool Extract(size_t pos, uint32& val);
  bool Extract(size_t pos, int64& val);
  bool Extract(size_t pos, uint64& val);

#ifndef FUN_LONG_IS_64_BIT
  bool Extract(size_t pos, long& val);
  bool Extract(size_t pos, unsigned long& val);
#endif

  bool Extract(size_t pos, bool& val);
  bool Extract(size_t pos, float& val);
  bool Extract(size_t pos, double& val);
  bool Extract(size_t pos, char& val);
  bool Extract(size_t pos, String& val);
  bool Extract(size_t pos, fun::sql::BLOB& val);
  bool Extract(size_t pos, fun::sql::CLOB& val);
  bool Extract(size_t pos, fun::sql::Date& val);
  bool Extract(size_t pos, fun::sql::Time& val);
  bool Extract(size_t pos, fun::DateTime& val);
  bool Extract(size_t pos, fun::Any& val);
  bool Extract(size_t pos, fun::dynamicAny& val);

  /**
   * Returns true if the current row value at pos column is null.
   * Because of the loss of information about null-ness of the
   * underlying database values due to the nature of SQLite engine,
   * (once null value is converted to default value, SQLite API
   * treats it  as non-null), a null indicator container member
   * variable is used to cache the indicators of the underlying nulls
   * thus rendering this function idempotent.
   * The container is a vector of [bool, bool] pairs.
   * The vector index corresponds to the column position, the first
   * bool value in the pair is true if the null indicator has
   * been set and the second bool value in the pair is true if
   * the column is actually null.
   * The row argument, needed for connectors with bulk capabilities,
   * is ignored in this implementation.
   */
  bool IsNull(size_t pos, size_t row = FUN_DATA_INVALID_ROW);

  /**
   * Clears the cached nulls indicator vector.
   */
  void Reset();

 private:
  /**
   * Utility function for extraction of Any and DynamicAny.
   */
  template <typename T>
  bool ExtractImpl(size_t pos, T& val) {
    if (IsNull(pos)) {
      return false;
    }

    bool ret = false;

    switch (Utility::GetColumnType(stmt_, pos)) {
      case MetaColumn::FDT_BOOL: {
        bool i = false;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_INT8: {
        int8 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_UINT8: {
        uint8 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_INT16: {
        int16 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_UINT16: {
        uint16 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_INT32: {
        int32 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_UINT32: {
        uint32 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_INT64: {
        int64 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_UINT64: {
        uint64 i = 0;
        ret = Extract(pos, i);
        val = i;
        break;
      }
      case MetaColumn::FDT_STRING: {
        String s;
        ret = Extract(pos, s);
        val = s;
        break;
      }
      case MetaColumn::FDT_DOUBLE: {
        double d(0.0);
        ret = Extract(pos, d);
        val = d;
        break;
      }
      case MetaColumn::FDT_FLOAT: {
        float f(0.0);
        ret = Extract(pos, f);
        val = f;
        break;
      }
      case MetaColumn::FDT_BLOB: {
        BLOB b;
        ret = Extract(pos, b);
        val = b;
        break;
      }
      case MetaColumn::FDT_DATE: {
        Date d;
        ret = Extract(pos, d);
        val = d;
        break;
      }
      case MetaColumn::FDT_TIME: {
        Time t;
        ret = Extract(pos, t);
        val = t;
        break;
      }
      case MetaColumn::FDT_TIMESTAMP: {
        DateTime dt;
        ret = Extract(pos, dt);
        val = dt;
        break;
      }
      default:
        throw fun::sql::UnknownTypeException("Unknown type during extraction");
    }

    return ret;
  }

  template <typename T>
  bool ExtractLOB(size_t pos, fun::sql::LOB<T>& val) {
    if (IsNull(pos)) {
      return false;
    }

    int size = sqlite3_column_bytes(stmt_, (int)pos);
    const T* blob =
        reinterpret_cast<const T*>(sqlite3_column_blob(stmt_, (int)pos));
    val = fun::sql::LOB<T>(blob, size);
    return true;
  }

  sqlite3_stmt* stmt_;
  NullIndVec nulls_;
};

//
// inlines
//

inline void Extractor::Reset() { nulls_.Clear(); }

inline bool Extractor::Extract(size_t pos, fun::sql::BLOB& val) {
  return ExtractLOB<fun::sql::BLOB::ValueType>(pos, val);
}

inline bool Extractor::Extract(size_t pos, fun::sql::CLOB& val) {
  return ExtractLOB<fun::sql::CLOB::ValueType>(pos, val);
}

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
