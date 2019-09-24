#pragma once

#include <vector>
#include "fun/base/any.h"
#include "fun/base/date_time.h"
#include "fun/base/dynamic_any.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/utf_string.h"
#include "fun/sql/constants.h"
#include "fun/sql/lob.h"
#include "fun/sql/odbc/handle.h"
#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/odbc_meta_column.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/preparator_base.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqlext.h>

namespace fun {
namespace sql {

class Date;
class Time;

namespace odbc {

/**
 * Class used for database preparation where we first have to register all data
 * types with respective memory output locations before extracting data.
 * Extraction works in two-phases: first Prepare is called once, then extract
 * n-times. In ODBC, SQLBindCol/SQLFetch is the preferred method of data
 * retrieval (SQLGetData is available, however with numerous driver
 * implementation dependent limitations and inferior performance). In order to
 * fit this functionality into fun DataConnectors framework, every ODBC sql
 * statement instantiates its own Preparator object. This is done once per
 * statement execution (from StatementImpl::BindImpl()).
 *
 * Preparator object is used to :
 *
 *   1) Prepare sql statement.
 *   2) Provide and contain the memory locations where retrieved values are
 * placed during recordset iteration. 3) Keep count of returned number of
 * columns with their respective datatypes and sizes.
 *
 * Notes:
 *
 * - Value datatypes in this interface Prepare() calls serve only for the
 * purpose of type distinction.
 * - Preparator keeps its own std::vector<Any> buffer for fetched data to be
 * later retrieved by Extractor.
 * - Prepare() methods should not be called when extraction mode is DE_MANUAL
 */
class FUN_ODBC_API Preparator : public PreparatorBase {
 public:
  typedef std::vector<char*> CharArray;
  typedef SharedPtr<Preparator> Ptr;

  enum DataExtraction { DE_MANUAL, DE_BOUND };

  enum DataType {
    DT_BOOL,
    DT_BOOL_ARRAY,
    DT_CHAR,
    DT_WCHAR,
    DT_UCHAR,
    DT_CHAR_ARRAY,
    DT_WCHAR_ARRAY,
    DT_UCHAR_ARRAY,
    DT_DATE,
    DT_TIME,
    DT_DATETIME
  };

  /**
   * Creates the Preparator.
   */
  Preparator(const StatementHandle& stmt, const String& statement,
             size_t maxFieldSize, DataExtraction dataExtraction);

  /**
   * Copy constructs the Preparator.
   */
  Preparator(const Preparator& other);

  /**
   * Destroys the Preparator.
   */
  ~Preparator();

  void Prepare(size_t pos, const int8& val);
  void Prepare(size_t pos, const std::vector<int8>& val);
  void Prepare(size_t pos, const std::deque<int8>& val);
  void Prepare(size_t pos, const std::list<int8>& val);
  void Prepare(size_t pos, const uint8& val);
  void Prepare(size_t pos, const std::vector<uint8>& val);
  void Prepare(size_t pos, const std::deque<uint8>& val);
  void Prepare(size_t pos, const std::list<uint8>& val);
  void Prepare(size_t pos, const int16& val);
  void Prepare(size_t pos, const std::vector<int16>& val);
  void Prepare(size_t pos, const std::deque<int16>& val);
  void Prepare(size_t pos, const std::list<int16>& val);
  void Prepare(size_t pos, const uint16& val);
  void Prepare(size_t pos, const std::vector<uint16>& val);
  void Prepare(size_t pos, const std::deque<uint16>& val);
  void Prepare(size_t pos, const std::list<uint16>& val);
  void Prepare(size_t pos, const int32& val);
  void Prepare(size_t pos, const std::vector<int32>& val);
  void Prepare(size_t pos, const std::deque<int32>& val);
  void Prepare(size_t pos, const std::list<int32>& val);
  void Prepare(size_t pos, const uint32& val);
  void Prepare(size_t pos, const std::vector<uint32>& val);
  void Prepare(size_t pos, const std::deque<uint32>& val);
  void Prepare(size_t pos, const std::list<uint32>& val);
  void Prepare(size_t pos, const int64& val);
  void Prepare(size_t pos, const std::vector<int64>& val);
  void Prepare(size_t pos, const std::deque<int64>& val);
  void Prepare(size_t pos, const std::list<int64>& val);
  void Prepare(size_t pos, const uint64& val);
  void Prepare(size_t pos, const std::vector<uint64>& val);
  void Prepare(size_t pos, const std::deque<uint64>& val);
  void Prepare(size_t pos, const std::list<uint64>& val);

#ifndef FUN_LONG_IS_64_BIT
  void Prepare(size_t pos, const long& val);
  void Prepare(size_t pos, const unsigned long& val);
  void Prepare(size_t pos, const std::vector<long>& val);
  void Prepare(size_t pos, const std::deque<long>& val);
  void Prepare(size_t pos, const std::list<long>& val);
#endif

  void Prepare(size_t pos, const bool& val);
  void Prepare(size_t pos, const std::vector<bool>& val);
  void Prepare(size_t pos, const std::deque<bool>& val);
  void Prepare(size_t pos, const std::list<bool>& val);
  void Prepare(size_t pos, const float& val);
  void Prepare(size_t pos, const std::vector<float>& val);
  void Prepare(size_t pos, const std::deque<float>& val);
  void Prepare(size_t pos, const std::list<float>& val);
  void Prepare(size_t pos, const double& val);
  void Prepare(size_t pos, const std::vector<double>& val);
  void Prepare(size_t pos, const std::deque<double>& val);
  void Prepare(size_t pos, const std::list<double>& val);
  void Prepare(size_t pos, const char& val);
  void Prepare(size_t pos, const std::vector<char>& val);
  void Prepare(size_t pos, const std::deque<char>& val);
  void Prepare(size_t pos, const std::list<char>& val);
  void Prepare(size_t pos, const String& val);
  void Prepare(size_t pos, const std::vector<String>& val);
  void Prepare(size_t pos, const std::deque<String>& val);
  void Prepare(size_t pos, const std::list<String>& val);
  void Prepare(size_t pos, const UString& val);
  void Prepare(size_t pos, const std::vector<UString>& val);
  void Prepare(size_t pos, const std::deque<UString>& val);
  void Prepare(size_t pos, const std::list<UString>& val);
  void Prepare(size_t pos, const fun::sql::BLOB& val);
  void Prepare(size_t pos, const std::vector<fun::sql::BLOB>& val);
  void Prepare(size_t pos, const std::deque<fun::sql::BLOB>& val);
  void Prepare(size_t pos, const std::list<fun::sql::BLOB>& val);
  void Prepare(size_t pos, const fun::sql::CLOB& val);
  void Prepare(size_t pos, const std::vector<fun::sql::CLOB>& val);
  void Prepare(size_t pos, const std::deque<fun::sql::CLOB>& val);
  void Prepare(size_t pos, const std::list<fun::sql::CLOB>& val);
  void Prepare(size_t pos, const fun::sql::Date& val);
  void Prepare(size_t pos, const std::vector<fun::sql::Date>& val);
  void Prepare(size_t pos, const std::deque<fun::sql::Date>& val);
  void Prepare(size_t pos, const std::list<fun::sql::Date>& val);
  void Prepare(size_t pos, const fun::sql::Time& val);
  void Prepare(size_t pos, const std::vector<fun::sql::Time>& val);
  void Prepare(size_t pos, const std::deque<fun::sql::Time>& val);
  void Prepare(size_t pos, const std::list<fun::sql::Time>& val);
  void Prepare(size_t pos, const fun::DateTime& val);
  void Prepare(size_t pos, const std::vector<fun::DateTime>& val);
  void Prepare(size_t pos, const std::deque<fun::DateTime>& val);
  void Prepare(size_t pos, const std::list<fun::DateTime>& val);
  void Prepare(size_t pos, const fun::Any& val);
  void Prepare(size_t pos, const std::vector<fun::Any>& val);
  void Prepare(size_t pos, const std::deque<fun::Any>& val);
  void Prepare(size_t pos, const std::list<fun::Any>& val);
  void Prepare(size_t pos, const fun::dynamicAny& val);
  void Prepare(size_t pos, const std::vector<fun::dynamicAny>& val);
  void Prepare(size_t pos, const std::deque<fun::dynamicAny>& val);
  void Prepare(size_t pos, const std::list<fun::dynamicAny>& val);

  /**
   * Returns the number of columns.
   * Resizes the internal storage if the size is zero.
   */
  size_t ColumnCount() const;

  /**
   * Returns reference to column data.
   */
  fun::Any& operator[](size_t pos);

  /**
   * Returns reference to column data.
   */
  fun::Any& At(size_t pos);

  /**
   * Sets maximum supported field size.
   */
  void SetMaxFieldSize(size_t size);

  /**
   * Returns maximum supported field size.
   */
  size_t GetMaxFieldSize() const;

  /**
   * Returns true if the values can potentially be very large.
   */
  bool IsPotentiallyHuge(size_t col) const;

  /**
   * Returns max supported size for column at position pos.
   * Returned length for variable length fields is the one
   * supported by this implementation, not the underlying DB.
   */
  size_t maxDataSize(size_t pos) const;

  /**
   * Returns the returned length for the column and row specified.
   * This is usually equal to the column size, except for
   * variable length fields (BLOB and variable length strings).
   * For null values, the return value is -1 (SQL_NO_DATA)
   */
  size_t actualDataSize(size_t col, size_t row = FUN_DATA_INVALID_ROW) const;

  /**
   * Returns bulk size. Column argument is optional
   * since all columns must be the same size.
   */
  size_t GetBulkSize(size_t col = 0) const;

  /**
   * Set data extraction mode.
   */
  void SetDataExtraction(DataExtraction ext);

  /**
   * Returns data extraction mode.
   */
  DataExtraction GetDataExtraction() const;

 private:
  typedef std::vector<fun::Any> ValueVec;
  typedef std::vector<bool> HugeFlagVec;
  typedef std::vector<SQLLEN> LengthVec;
  typedef std::vector<LengthVec> LengthLengthVec;
  typedef std::map<size_t, DataType> IndexMap;

  Preparator();
  Preparator& operator=(const Preparator&);

  /**
   * Utility function to Prepare Any and DynamicAny.
   */
  template <typename C>
  void PrepareImpl(size_t pos, const C* pVal = 0) {
    OdbcMetaColumn col(stmt_, pos);

    switch (col.type()) {
      case MetaColumn::FDT_INT8:
        if (pVal)
          return PrepareFixedSize<int8>(pos, SQL_C_STINYINT, pVal->size());
        else
          return PrepareFixedSize<int8>(pos, SQL_C_STINYINT);

      case MetaColumn::FDT_UINT8:
        if (pVal)
          return PrepareFixedSize<uint8>(pos, SQL_C_UTINYINT, pVal->size());
        else
          return PrepareFixedSize<uint8>(pos, SQL_C_UTINYINT);

      case MetaColumn::FDT_INT16:
        if (pVal)
          return PrepareFixedSize<int16>(pos, SQL_C_SSHORT, pVal->size());
        else
          return PrepareFixedSize<int16>(pos, SQL_C_SSHORT);

      case MetaColumn::FDT_UINT16:
        if (pVal)
          return PrepareFixedSize<uint16>(pos, SQL_C_USHORT, pVal->size());
        else
          return PrepareFixedSize<uint16>(pos, SQL_C_USHORT);

      case MetaColumn::FDT_INT32:
        if (pVal)
          return PrepareFixedSize<int32>(pos, SQL_C_SLONG, pVal->size());
        else
          return PrepareFixedSize<int32>(pos, SQL_C_SLONG);

      case MetaColumn::FDT_UINT32:
        if (pVal)
          return PrepareFixedSize<uint32>(pos, SQL_C_ULONG, pVal->size());
        else
          return PrepareFixedSize<uint32>(pos, SQL_C_ULONG);

      case MetaColumn::FDT_INT64:
        if (pVal)
          return PrepareFixedSize<int64>(pos, SQL_C_SBIGINT, pVal->size());
        else
          return PrepareFixedSize<int64>(pos, SQL_C_SBIGINT);

      case MetaColumn::FDT_UINT64:
        if (pVal)
          return PrepareFixedSize<uint64>(pos, SQL_C_UBIGINT, pVal->size());
        else
          return PrepareFixedSize<uint64>(pos, SQL_C_UBIGINT);

      case MetaColumn::FDT_BOOL:
        if (pVal)
          return PrepareBoolArray(pos, SQL_C_BIT, pVal->size());
        else
          return PrepareFixedSize<bool>(pos, SQL_C_BIT);

      case MetaColumn::FDT_FLOAT:
        if (pVal)
          return PrepareFixedSize<float>(pos, SQL_C_FLOAT, pVal->size());
        else
          return PrepareFixedSize<float>(pos, SQL_C_FLOAT);

      case MetaColumn::FDT_DOUBLE:
        if (pVal)
          return PrepareFixedSize<double>(pos, SQL_C_DOUBLE, pVal->size());
        else
          return PrepareFixedSize<double>(pos, SQL_C_DOUBLE);

      case MetaColumn::FDT_STRING:
        if (pVal)
          return PrepareCharArray<char, DT_CHAR_ARRAY>(
              pos, SQL_C_CHAR, maxDataSize(pos), pVal->size());
        else
          return prepareVariableLen<char>(pos, SQL_C_CHAR, maxDataSize(pos),
                                          DT_CHAR);

      case MetaColumn::FDT_WSTRING: {
        typedef UString::value_type CharType;
        if (pVal)
          return PrepareCharArray<CharType, DT_WCHAR_ARRAY>(
              pos, SQL_C_WCHAR, maxDataSize(pos), pVal->size());
        else
          return prepareVariableLen<CharType>(pos, SQL_C_WCHAR,
                                              maxDataSize(pos), DT_WCHAR);
      }

      case MetaColumn::FDT_BLOB: {
        typedef fun::sql::BLOB::ValueType CharType;
        if (pVal)
          return PrepareCharArray<CharType, DT_UCHAR_ARRAY>(
              pos, SQL_C_BINARY, maxDataSize(pos), pVal->size());
        else
          return prepareVariableLen<CharType>(pos, SQL_C_BINARY,
                                              maxDataSize(pos), DT_UCHAR);
      }

      case MetaColumn::FDT_CLOB: {
        typedef fun::sql::CLOB::ValueType CharType;
        if (pVal)
          return PrepareCharArray<CharType, DT_CHAR_ARRAY>(
              pos, SQL_C_BINARY, maxDataSize(pos), pVal->size());
        else
          return prepareVariableLen<CharType>(pos, SQL_C_BINARY,
                                              maxDataSize(pos), DT_CHAR);
      }

      case MetaColumn::FDT_DATE:
        if (pVal)
          return PrepareFixedSize<Date>(pos, SQL_C_TYPE_DATE, pVal->size());
        else
          return PrepareFixedSize<Date>(pos, SQL_C_TYPE_DATE);

      case MetaColumn::FDT_TIME:
        if (pVal)
          return PrepareFixedSize<Time>(pos, SQL_C_TYPE_TIME, pVal->size());
        else
          return PrepareFixedSize<Time>(pos, SQL_C_TYPE_TIME);

      case MetaColumn::FDT_TIMESTAMP:
        if (pVal)
          return PrepareFixedSize<DateTime>(pos, SQL_C_TYPE_TIMESTAMP,
                                            pVal->size());
        else
          return PrepareFixedSize<DateTime>(pos, SQL_C_TYPE_TIMESTAMP);

      default:
        throw DataFormatException("Unsupported data type.");
    }
  }

  /**
   * Resize the values and lengths vectors.
   */
  void Resize() const;

  /**
   * Utility function for preparation of fixed length columns.
   */
  template <typename T>
  void PrepareFixedSize(size_t pos, SQLSMALLINT valueType) {
    fun_check(DE_BOUND == data_extraction_);
    size_t dataSize = sizeof(T);

    fun_check(pos < values_.size());
    values_[pos] = fun::Any(T());

    T* pVal = AnyCast<T>(&values_[pos]);
    if (Utility::IsError(SQLBindCol(stmt_, (SQLUSMALLINT)pos + 1, valueType,
                                    (SQLPOINTER)pVal, (SQLINTEGER)dataSize,
                                    &lengths_[pos]))) {
      throw StatementException(stmt_, "SQLBindCol()");
    }
  }

  /**
   * Utility function for preparation of fixed length columns that are
   * bound to a std::vector.
   */
  template <typename T>
  void PrepareFixedSize(size_t pos, SQLSMALLINT valueType, size_t length) {
    fun_check(DE_BOUND == data_extraction_);
    size_t dataSize = sizeof(T);

    fun_check(pos < values_.size());
    fun_check(length);
    values_[pos] = fun::Any(std::vector<T>());
    lengths_[pos] = 0;
    fun_check(0 == len_lengths_[pos].size());
    len_lengths_[pos].resize(length);

    std::vector<T>& cache = RefAnyCast<std::vector<T> >(values_[pos]);
    cache.resize(length);

    if (Utility::IsError(SQLBindCol(stmt_, (SQLUSMALLINT)pos + 1, valueType,
                                    (SQLPOINTER)&cache[0], (SQLINTEGER)dataSize,
                                    &len_lengths_[pos][0]))) {
      throw StatementException(stmt_, "SQLBindCol()");
    }
  }

  /**
   * Utility function for preparation of variable length columns.
   */
  template <typename T>
  void prepareVariableLen(size_t pos, SQLSMALLINT valueType, size_t size,
                          DataType dt) {
    fun_check(DE_BOUND == data_extraction_);
    fun_check(pos < values_.size());

    OdbcMetaColumn col(stmt_, pos);
    if (col.length() >= max_field_size_) {
      huge_flags_[pos] = true;
    } else {
      T* pCache = new T[size];
      UnsafeMemory::Memset(pCache, 0, size);

      values_[pos] = Any(pCache);
      lengths_[pos] = (SQLLEN)size;
      var_length_arrays_.insert(IndexMap::value_type(pos, dt));

      if (Utility::IsError(SQLBindCol(
              stmt_, (SQLUSMALLINT)pos + 1, valueType, (SQLPOINTER)pCache,
              (SQLINTEGER)size * sizeof(T), &lengths_[pos]))) {
        throw StatementException(stmt_, "SQLBindCol()");
      }
    }
  }

  /**
   * Utility function for preparation of bulk variable length character and LOB
   * columns.
   */
  template <typename T, DataType DT>
  void PrepareCharArray(size_t pos, SQLSMALLINT valueType, size_t size,
                        size_t length) {
    fun_check_dbg(DE_BOUND == data_extraction_);
    fun_check_dbg(pos < values_.size());
    fun_check_dbg(pos < lengths_.size());
    fun_check_dbg(pos < len_lengths_.size());

    T* pArray = (T*)std::calloc(length * size, sizeof(T));

    values_[pos] = Any(pArray);
    lengths_[pos] = 0;
    len_lengths_[pos].resize(length);
    var_length_arrays_.insert(IndexMap::value_type(pos, DT));

    if (Utility::IsError(SQLBindCol(stmt_, (SQLUSMALLINT)pos + 1, valueType,
                                    (SQLPOINTER)pArray, (SQLINTEGER)size,
                                    &len_lengths_[pos][0]))) {
      throw StatementException(stmt_, "SQLBindCol()");
    }
  }

  /**
   * Utility function for preparation of bulk bool columns.
   */
  void PrepareBoolArray(size_t pos, SQLSMALLINT valueType, size_t length);

  /**
   * Utility function. Releases memory allocated for variable length columns.
   */
  void FreeMemory() const;

  template <typename T>
  void DeleteCachedArray(size_t pos) const {
    T** p = fun::AnyCast<T*>(&values_[pos]);
    if (p) {
      delete[] * p;
    }
  }

  const StatementHandle& stmt_;
  mutable ValueVec values_;
  mutable HugeFlagVec huge_flags_;
  mutable LengthVec lengths_;
  mutable LengthLengthVec len_lengths_;
  mutable IndexMap var_length_arrays_;
  size_t max_field_size_;
  DataExtraction data_extraction_;
};

//
// inlines
//

inline void Preparator::Prepare(size_t pos, const int8&) {
  PrepareFixedSize<int8>(pos, SQL_C_STINYINT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<int8>& val) {
  PrepareFixedSize<int8>(pos, SQL_C_STINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<int8>& val) {
  PrepareFixedSize<int8>(pos, SQL_C_STINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<int8>& val) {
  PrepareFixedSize<int8>(pos, SQL_C_STINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const uint8&) {
  PrepareFixedSize<uint8>(pos, SQL_C_UTINYINT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<uint8>& val) {
  PrepareFixedSize<uint8>(pos, SQL_C_UTINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<uint8>& val) {
  PrepareFixedSize<uint8>(pos, SQL_C_UTINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<uint8>& val) {
  PrepareFixedSize<uint8>(pos, SQL_C_UTINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const int16&) {
  PrepareFixedSize<int16>(pos, SQL_C_SSHORT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<int16>& val) {
  PrepareFixedSize<int16>(pos, SQL_C_SSHORT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<int16>& val) {
  PrepareFixedSize<int16>(pos, SQL_C_SSHORT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<int16>& val) {
  PrepareFixedSize<int16>(pos, SQL_C_SSHORT, val.size());
}

inline void Preparator::Prepare(size_t pos, const uint16&) {
  PrepareFixedSize<uint16>(pos, SQL_C_USHORT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<uint16>& val) {
  PrepareFixedSize<uint16>(pos, SQL_C_USHORT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<uint16>& val) {
  PrepareFixedSize<uint16>(pos, SQL_C_USHORT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<uint16>& val) {
  PrepareFixedSize<uint16>(pos, SQL_C_USHORT, val.size());
}

inline void Preparator::Prepare(size_t pos, const int32&) {
  PrepareFixedSize<int32>(pos, SQL_C_SLONG);
}

inline void Preparator::Prepare(size_t pos, const std::vector<int32>& val) {
  PrepareFixedSize<int32>(pos, SQL_C_SLONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<int32>& val) {
  PrepareFixedSize<int32>(pos, SQL_C_SLONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<int32>& val) {
  PrepareFixedSize<int32>(pos, SQL_C_SLONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const uint32&) {
  PrepareFixedSize<uint32>(pos, SQL_C_ULONG);
}

inline void Preparator::Prepare(size_t pos, const std::vector<uint32>& val) {
  PrepareFixedSize<uint32>(pos, SQL_C_ULONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<uint32>& val) {
  PrepareFixedSize<uint32>(pos, SQL_C_ULONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<uint32>& val) {
  PrepareFixedSize<uint32>(pos, SQL_C_ULONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const int64&) {
  PrepareFixedSize<int64>(pos, SQL_C_SBIGINT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<int64>& val) {
  PrepareFixedSize<int64>(pos, SQL_C_SBIGINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<int64>& val) {
  PrepareFixedSize<int64>(pos, SQL_C_SBIGINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<int64>& val) {
  PrepareFixedSize<int64>(pos, SQL_C_SBIGINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const uint64&) {
  PrepareFixedSize<uint64>(pos, SQL_C_UBIGINT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<uint64>& val) {
  PrepareFixedSize<uint64>(pos, SQL_C_UBIGINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<uint64>& val) {
  PrepareFixedSize<uint64>(pos, SQL_C_UBIGINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<uint64>& val) {
  PrepareFixedSize<uint64>(pos, SQL_C_UBIGINT, val.size());
}

#ifndef FUN_LONG_IS_64_BIT
inline void Preparator::Prepare(size_t pos, const long&) {
  PrepareFixedSize<long>(pos, SQL_C_SLONG);
}

inline void Preparator::Prepare(size_t pos, const unsigned long&) {
  PrepareFixedSize<long>(pos, SQL_C_SLONG);
}

inline void Preparator::Prepare(size_t pos, const std::vector<long>& val) {
  PrepareFixedSize<long>(pos, SQL_C_SLONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<long>& val) {
  PrepareFixedSize<long>(pos, SQL_C_SLONG, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<long>& val) {
  PrepareFixedSize<long>(pos, SQL_C_SLONG, val.size());
}
#endif

inline void Preparator::Prepare(size_t pos, const bool&) {
  PrepareFixedSize<bool>(pos, SQL_C_BIT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<bool>& val) {
  PrepareBoolArray(pos, SQL_C_BIT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<bool>& val) {
  PrepareBoolArray(pos, SQL_C_BIT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<bool>& val) {
  PrepareBoolArray(pos, SQL_C_BIT, val.size());
}

inline void Preparator::Prepare(size_t pos, const float&) {
  PrepareFixedSize<float>(pos, SQL_C_FLOAT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<float>& val) {
  PrepareFixedSize<float>(pos, SQL_C_FLOAT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<float>& val) {
  PrepareFixedSize<float>(pos, SQL_C_FLOAT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<float>& val) {
  PrepareFixedSize<float>(pos, SQL_C_FLOAT, val.size());
}

inline void Preparator::Prepare(size_t pos, const double&) {
  PrepareFixedSize<double>(pos, SQL_C_DOUBLE);
}

inline void Preparator::Prepare(size_t pos, const std::vector<double>& val) {
  PrepareFixedSize<double>(pos, SQL_C_DOUBLE, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<double>& val) {
  PrepareFixedSize<double>(pos, SQL_C_DOUBLE, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<double>& val) {
  PrepareFixedSize<double>(pos, SQL_C_DOUBLE, val.size());
}

inline void Preparator::Prepare(size_t pos, const char&) {
  PrepareFixedSize<char>(pos, SQL_C_STINYINT);
}

inline void Preparator::Prepare(size_t pos, const std::vector<char>& val) {
  PrepareFixedSize<char>(pos, SQL_C_STINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<char>& val) {
  PrepareFixedSize<char>(pos, SQL_C_STINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<char>& val) {
  PrepareFixedSize<char>(pos, SQL_C_STINYINT, val.size());
}

inline void Preparator::Prepare(size_t pos, const String&) {
  prepareVariableLen<char>(pos, SQL_C_CHAR, maxDataSize(pos), DT_CHAR);
}

inline void Preparator::Prepare(size_t pos, const std::vector<String>& val) {
  PrepareCharArray<char, DT_CHAR_ARRAY>(pos, SQL_C_CHAR, maxDataSize(pos),
                                        val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<String>& val) {
  PrepareCharArray<char, DT_CHAR_ARRAY>(pos, SQL_C_CHAR, maxDataSize(pos),
                                        val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<String>& val) {
  PrepareCharArray<char, DT_CHAR_ARRAY>(pos, SQL_C_CHAR, maxDataSize(pos),
                                        val.size());
}

inline void Preparator::Prepare(size_t pos, const UString&) {
  prepareVariableLen<UString::value_type>(pos, SQL_C_WCHAR, maxDataSize(pos),
                                          DT_WCHAR);
}

inline void Preparator::Prepare(size_t pos, const std::vector<UString>& val) {
  PrepareCharArray<UString::value_type, DT_WCHAR_ARRAY>(
      pos, SQL_C_WCHAR, maxDataSize(pos), val.size());
}

inline void Preparator::Prepare(size_t pos, const std::deque<UString>& val) {
  PrepareCharArray<UString::value_type, DT_WCHAR_ARRAY>(
      pos, SQL_C_WCHAR, maxDataSize(pos), val.size());
}

inline void Preparator::Prepare(size_t pos, const std::list<UString>& val) {
  PrepareCharArray<UString::value_type, DT_WCHAR_ARRAY>(
      pos, SQL_C_WCHAR, maxDataSize(pos), val.size());
}

inline void Preparator::Prepare(size_t pos, const fun::sql::BLOB&) {
  prepareVariableLen<fun::sql::BLOB::ValueType>(pos, SQL_C_BINARY,
                                                maxDataSize(pos), DT_UCHAR);
}

inline void Preparator::Prepare(size_t pos,
                                const std::vector<fun::sql::BLOB>& val) {
  PrepareCharArray<char, DT_UCHAR_ARRAY>(pos, SQL_C_BINARY, maxDataSize(pos),
                                         val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::deque<fun::sql::BLOB>& val) {
  PrepareCharArray<char, DT_UCHAR_ARRAY>(pos, SQL_C_BINARY, maxDataSize(pos),
                                         val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::list<fun::sql::BLOB>& val) {
  PrepareCharArray<char, DT_UCHAR_ARRAY>(pos, SQL_C_BINARY, maxDataSize(pos),
                                         val.size());
}

inline void Preparator::Prepare(size_t pos, const fun::sql::CLOB&) {
  prepareVariableLen<fun::sql::CLOB::ValueType>(pos, SQL_C_BINARY,
                                                maxDataSize(pos), DT_CHAR);
}

inline void Preparator::Prepare(size_t pos,
                                const std::vector<fun::sql::CLOB>& val) {
  PrepareCharArray<char, DT_CHAR_ARRAY>(pos, SQL_C_BINARY, maxDataSize(pos),
                                        val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::deque<fun::sql::CLOB>& val) {
  PrepareCharArray<char, DT_CHAR_ARRAY>(pos, SQL_C_BINARY, maxDataSize(pos),
                                        val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::list<fun::sql::CLOB>& val) {
  PrepareCharArray<char, DT_CHAR_ARRAY>(pos, SQL_C_BINARY, maxDataSize(pos),
                                        val.size());
}

inline void Preparator::Prepare(size_t pos, const fun::sql::Date&) {
  PrepareFixedSize<SQL_DATE_STRUCT>(pos, SQL_C_TYPE_DATE);
}

inline void Preparator::Prepare(size_t pos,
                                const std::vector<fun::sql::Date>& val) {
  PrepareFixedSize<SQL_DATE_STRUCT>(pos, SQL_C_TYPE_DATE, val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::deque<fun::sql::Date>& val) {
  PrepareFixedSize<SQL_DATE_STRUCT>(pos, SQL_C_TYPE_DATE, val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::list<fun::sql::Date>& val) {
  PrepareFixedSize<SQL_DATE_STRUCT>(pos, SQL_C_TYPE_DATE, val.size());
}

inline void Preparator::Prepare(size_t pos, const fun::sql::Time&) {
  PrepareFixedSize<SQL_TIME_STRUCT>(pos, SQL_C_TYPE_TIME);
}

inline void Preparator::Prepare(size_t pos,
                                const std::vector<fun::sql::Time>& val) {
  PrepareFixedSize<SQL_TIME_STRUCT>(pos, SQL_C_TYPE_TIME, val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::deque<fun::sql::Time>& val) {
  PrepareFixedSize<SQL_TIME_STRUCT>(pos, SQL_C_TYPE_TIME, val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::list<fun::sql::Time>& val) {
  PrepareFixedSize<SQL_TIME_STRUCT>(pos, SQL_C_TYPE_TIME, val.size());
}

inline void Preparator::Prepare(size_t pos, const fun::DateTime&) {
  PrepareFixedSize<SQL_TIMESTAMP_STRUCT>(pos, SQL_C_TYPE_TIMESTAMP);
}

inline void Preparator::Prepare(size_t pos,
                                const std::vector<fun::DateTime>& val) {
  PrepareFixedSize<SQL_TIMESTAMP_STRUCT>(pos, SQL_C_TYPE_TIMESTAMP, val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::deque<fun::DateTime>& val) {
  PrepareFixedSize<SQL_TIMESTAMP_STRUCT>(pos, SQL_C_TYPE_TIMESTAMP, val.size());
}

inline void Preparator::Prepare(size_t pos,
                                const std::list<fun::DateTime>& val) {
  PrepareFixedSize<SQL_TIMESTAMP_STRUCT>(pos, SQL_C_TYPE_TIMESTAMP, val.size());
}

inline void Preparator::Prepare(size_t pos, const fun::Any& val) {
  PrepareImpl<std::vector<fun::Any> >(pos);
}

inline void Preparator::Prepare(size_t pos, const std::vector<fun::Any>& val) {
  PrepareImpl<std::vector<fun::Any> >(pos, &val);
}

inline void Preparator::Prepare(size_t pos, const std::deque<fun::Any>& val) {
  PrepareImpl<std::deque<fun::Any> >(pos, &val);
}

inline void Preparator::Prepare(size_t pos, const std::list<fun::Any>& val) {
  PrepareImpl<std::list<fun::Any> >(pos, &val);
}

inline void Preparator::Prepare(size_t pos, const fun::dynamicAny& val) {
  PrepareImpl<std::vector<fun::dynamicAny> >(pos);
}

inline void Preparator::Prepare(size_t pos,
                                const std::vector<fun::dynamicAny>& val) {
  PrepareImpl<std::vector<fun::dynamicAny> >(pos, &val);
}

inline void Preparator::Prepare(size_t pos,
                                const std::deque<fun::dynamicAny>& val) {
  PrepareImpl<std::deque<fun::dynamicAny> >(pos, &val);
}

inline void Preparator::Prepare(size_t pos,
                                const std::list<fun::dynamicAny>& val) {
  PrepareImpl<std::list<fun::dynamicAny> >(pos, &val);
}

inline size_t Preparator::GetBulkSize(size_t col) const {
  fun_check(col < len_lengths_.size());

  return len_lengths_[col].size();
}

inline void Preparator::SetMaxFieldSize(size_t size) { max_field_size_ = size; }

inline size_t Preparator::GetMaxFieldSize() const { return max_field_size_; }

inline bool Preparator::IsPotentiallyHuge(size_t col) const {
  return huge_flags_[col];
}

inline void Preparator::SetDataExtraction(Preparator::DataExtraction ext) {
  data_extraction_ = ext;
}

inline Preparator::DataExtraction Preparator::GetDataExtraction() const {
  return data_extraction_;
}

inline fun::Any& Preparator::operator[](size_t pos) { return At(pos); }

inline fun::Any& Preparator::At(size_t pos) { return values_.At(pos); }

}  // namespace odbc
}  // namespace sql
}  // namespace fun
