#pragma once

#include "fun/sql/constants.h"
#include "fun/sql/odbc/odbc.h"
#include "fun/sql/extractor_base.h"
#include "fun/sql/odbc/preparator.h"
#include "fun/sql/odbc/odbc_meta_column.h"
#include "fun/sql/odbc/error.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/date.h"
#include "fun/sql/time.h"
#include "fun/base/date_time.h"
#include "fun/base/any.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/nullable.h"
#include "fun/base/utf_string.h"
#include "fun/base/exception.h"
#include <map>

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
  #include <windows.h>
#endif

#include <sqltypes.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * Extracts and converts data values from the result row returned by ODBC.
 * If NULL is received, the incoming val value is not changed and false is returned
 */
class FUN_ODBC_API Extractor : public fun::sql::ExtractorBase {
 public:
  typedef Preparator::Ptr PreparatorPtr;

  /**
   * Creates the Extractor.
   */
  Extractor(const StatementHandle& stmt, Preparator::Ptr preparator);

  /**
   * Destroys the Extractor.
   */
  ~Extractor();


  bool Extract(size_t pos, int8& val);
  bool Extract(size_t pos, std::vector<int8>& val);
  bool Extract(size_t pos, std::deque<int8>& val);
  bool Extract(size_t pos, std::list<int8>& val);
  bool Extract(size_t pos, uint8& val);
  bool Extract(size_t pos, std::vector<uint8>& val);
  bool Extract(size_t pos, std::deque<uint8>& val);
  bool Extract(size_t pos, std::list<uint8>& val);
  bool Extract(size_t pos, int16& val);
  bool Extract(size_t pos, std::vector<int16>& val);
  bool Extract(size_t pos, std::deque<int16>& val);
  bool Extract(size_t pos, std::list<int16>& val);
  bool Extract(size_t pos, uint16& val);
  bool Extract(size_t pos, std::vector<uint16>& val);
  bool Extract(size_t pos, std::deque<uint16>& val);
  bool Extract(size_t pos, std::list<uint16>& val);
  bool Extract(size_t pos, int32& val);
  bool Extract(size_t pos, std::vector<int32>& val);
  bool Extract(size_t pos, std::deque<int32>& val);
  bool Extract(size_t pos, std::list<int32>& val);
  bool Extract(size_t pos, uint32& val);
  bool Extract(size_t pos, std::vector<uint32>& val);
  bool Extract(size_t pos, std::deque<uint32>& val);
  bool Extract(size_t pos, std::list<uint32>& val);
  bool Extract(size_t pos, int64& val);
  bool Extract(size_t pos, std::vector<int64>& val);
  bool Extract(size_t pos, std::deque<int64>& val);
  bool Extract(size_t pos, std::list<int64>& val);
  bool Extract(size_t pos, uint64& val);
  bool Extract(size_t pos, std::vector<uint64>& val);
  bool Extract(size_t pos, std::deque<uint64>& val);
  bool Extract(size_t pos, std::list<uint64>& val);

#ifndef FUN_LONG_IS_64_BIT
  bool Extract(size_t pos, long& val);
  bool Extract(size_t pos, unsigned long& val);
  bool Extract(size_t pos, std::vector<long>& val);
  bool Extract(size_t pos, std::deque<long>& val);
  bool Extract(size_t pos, std::list<long>& val);
#endif

  bool Extract(size_t pos, bool& val);
  bool Extract(size_t pos, std::vector<bool>& val);
  bool Extract(size_t pos, std::deque<bool>& val);
  bool Extract(size_t pos, std::list<bool>& val);
  bool Extract(size_t pos, float& val);
  bool Extract(size_t pos, std::vector<float>& val);
  bool Extract(size_t pos, std::deque<float>& val);
  bool Extract(size_t pos, std::list<float>& val);
  bool Extract(size_t pos, double& val);
  bool Extract(size_t pos, std::vector<double>& val);
  bool Extract(size_t pos, std::deque<double>& val);
  bool Extract(size_t pos, std::list<double>& val);
  bool Extract(size_t pos, char& val);
  bool Extract(size_t pos, std::vector<char>& val);
  bool Extract(size_t pos, std::deque<char>& val);
  bool Extract(size_t pos, std::list<char>& val);
  bool Extract(size_t pos, String& val);
  bool Extract(size_t pos, std::vector<String>& val);
  bool Extract(size_t pos, std::deque<String>& val);
  bool Extract(size_t pos, std::list<String>& val);
  bool Extract(size_t pos, UString& val);
  bool Extract(size_t pos, std::vector<UString>& val);
  bool Extract(size_t pos, std::deque<UString>& val);
  bool Extract(size_t pos, std::list<UString>& val);
  bool Extract(size_t pos, fun::sql::BLOB& val);
  bool Extract(size_t pos, fun::sql::CLOB& val);
  bool Extract(size_t pos, std::vector<fun::sql::BLOB>& val);
  bool Extract(size_t pos, std::deque<fun::sql::BLOB>& val);
  bool Extract(size_t pos, std::list<fun::sql::BLOB>& val);
  bool Extract(size_t pos, std::vector<fun::sql::CLOB>& val);
  bool Extract(size_t pos, std::deque<fun::sql::CLOB>& val);
  bool Extract(size_t pos, std::list<fun::sql::CLOB>& val);
  bool Extract(size_t pos, fun::sql::Date& val);
  bool Extract(size_t pos, std::vector<fun::sql::Date>& val);
  bool Extract(size_t pos, std::deque<fun::sql::Date>& val);
  bool Extract(size_t pos, std::list<fun::sql::Date>& val);
  bool Extract(size_t pos, fun::sql::Time& val);
  bool Extract(size_t pos, std::vector<fun::sql::Time>& val);
  bool Extract(size_t pos, std::deque<fun::sql::Time>& val);
  bool Extract(size_t pos, std::list<fun::sql::Time>& val);
  bool Extract(size_t pos, fun::DateTime& val);
  bool Extract(size_t pos, std::vector<fun::DateTime>& val);
  bool Extract(size_t pos, std::deque<fun::DateTime>& val);
  bool Extract(size_t pos, std::list<fun::DateTime>& val);
  bool Extract(size_t pos, fun::Any& val);
  bool Extract(size_t pos, std::vector<fun::Any>& val);
  bool Extract(size_t pos, std::deque<fun::Any>& val);
  bool Extract(size_t pos, std::list<fun::Any>& val);
  bool Extract(size_t pos, fun::dynamicAny& val);
  bool Extract(size_t pos, std::vector<fun::dynamicAny>& val);
  bool Extract(size_t pos, std::deque<fun::dynamicAny>& val);
  bool Extract(size_t pos, std::list<fun::dynamicAny>& val);

  /**
   * Set data extraction mode.
   */
  void SetDataExtraction(Preparator::DataExtraction ext);

  /**
   * Returns data extraction mode.
   */
  Preparator::DataExtraction GetDataExtraction() const;

  /**
   * Returns true if the value at [col,row] is null.
   */
  bool IsNull(size_t col, size_t row = FUN_DATA_INVALID_ROW);

  /**
   * Resets the internally cached length indicators.
   */
  void Reset();

 private:
  /**
   * Amount of data retrieved in one SQLGetData() request when doing manual Extract.
   */
  static const int CHUNK_SIZE = 1024;

  /**
   * String format for the exception message when the field size is exceeded.
   */
  static const String FLD_SIZE_EXCEEDED_FMT;

  /**
   * This check is only performed for bound data
   * retrieval from variable length columns.
   * The reason for this check is to ensure we can
   * accept the value ODBC driver is supplying
   * (i.e. the bound buffer is large enough to receive
   * the returned value)
   */
  void CheckDataSize(size_t size);

  /**
   * Resizes the vector holding extracted data lengths to the
   * appropriate size.
   */
  void ResizeLengths(size_t pos);

  template <typename T>
  bool ExtractBoundImpl(size_t pos, T& val) {
    if (IsNull(pos)) {
      return false;
    }

    fun_check_dbg(typeid(T) == preparator_->at(pos).type());
    val = *AnyCast<T>(&preparator_->at(pos));
    return true;
  }

  bool ExtractBoundImpl(size_t pos, fun::sql::BLOB& val);
  bool ExtractBoundImpl(size_t pos, fun::sql::CLOB& val);

  template <typename C>
  bool ExtractBoundImplContainer(size_t pos, C& val) {
    typedef typename C::value_type Type;
    fun_check_dbg(typeid(std::vector<Type>) == preparator_->at(pos).type());
    std::vector<Type>& v = RefAnyCast<std::vector<Type> >(preparator_->at(pos));
    val.Assign(v.begin(), v.end());
    return true;
  }

  bool ExtractBoundImplContainer(size_t pos, std::vector<String>& values);
  bool ExtractBoundImplContainer(size_t pos, std::deque<String>& values);
  bool ExtractBoundImplContainer(size_t pos, std::list<String>& values);
  bool ExtractBoundImplContainer(size_t pos, std::vector<fun::UString>& values);
  bool ExtractBoundImplContainer(size_t pos, std::deque<fun::UString>& values);
  bool ExtractBoundImplContainer(size_t pos, std::list<fun::UString>& values);
  bool ExtractBoundImplContainer(size_t pos, std::vector<fun::sql::CLOB>& values);
  bool ExtractBoundImplContainer(size_t pos, std::deque<fun::sql::CLOB>& values);
  bool ExtractBoundImplContainer(size_t pos, std::list<fun::sql::CLOB>& values);
  bool ExtractBoundImplContainer(size_t pos, std::vector<fun::sql::BLOB>& values);
  bool ExtractBoundImplContainer(size_t pos, std::deque<fun::sql::BLOB>& values);
  bool ExtractBoundImplContainer(size_t pos, std::list<fun::sql::BLOB>& values);

  template <typename C>
  bool ExtractBoundImplContainerString(size_t pos, C& values) {
    typedef typename C::value_type StringType;
    typedef typename C::iterator ItType;
    typedef typename StringType::value_type CharType;

    CharType** pc = AnyCast<CharType*>(&(preparator_->at(pos)));
    fun_check_dbg(pc);
    fun_check_dbg(preparator_->bulk_size() == values.size());
    size_t colWidth = columnSize(pos);
    ItType it = values.begin();
    ItType end = values.end();
    for (int row = 0; it != end; ++it, ++row) {
      it->Assign(*pc + row * colWidth / sizeof(CharType), preparator_->actualDataSize(pos, row));
      // clean up superfluous null chars returned by some drivers
      typename StringType::size_type trimLen = 0;
      typename StringType::reverse_iterator sIt = it->rbegin();
      typename StringType::reverse_iterator sEnd = it->rend();
      for (; sIt != sEnd; ++sIt) {
        if (*sIt == '\0') {
          ++trimLen;
        } else {
          break;
        }
      }
      if (trimLen) it->Assign(it->begin(), it->begin() + it->length() - trimLen);
    }

    return true;
  }

  template <typename C>
  bool ExtractBoundImplContainerLOB(size_t pos, C& values) {
    typedef typename C::value_type LOBType;
    typedef typename LOBType::ValueType CharType;
    typedef typename C::iterator ItType;

    CharType** pc = AnyCast<CharType*>(&(preparator_->at(pos)));
    fun_check_dbg(pc);
    fun_check_dbg(preparator_->bulk_size() == values.size());
    size_t colWidth = preparator_->maxDataSize(pos);
    ItType it = values.begin();
    ItType end = values.end();
    for (int row = 0; it != end; ++it, ++row) {
      it->AssignRaw(*pc + row * colWidth, preparator_->actualDataSize(pos, row));
    }

    return true;
  }

  template <typename T>
  bool ExtractBoundImplLOB(size_t pos, fun::sql::LOB<T>& val) {
    if (IsNull(pos)) return false;

    size_t dataSize = preparator_->actualDataSize(pos);
    CheckDataSize(dataSize);
    T* sp = AnyCast<T*>(preparator_->at(pos));
    val.AssignRaw(sp, dataSize);

    return true;
  }

  template <typename T>
  bool extractManualImpl(size_t pos, T& val, SQLSMALLINT cType) {
    SQLRETURN rc = 0;
    T value = (T) 0;

    ResizeLengths(pos);

    rc = SQLGetData(stmt_,
      (SQLUSMALLINT) pos + 1,
      cType,  //C data type
      &value, //returned value
      0,      //buffer length (ignored)
      &lengths_[pos]);  //length indicator

    if (Utility::IsError(rc)) {
      throw StatementException(stmt_, "SQLGetData()");
    }

    if (IsNullLengthIndicator(lengths_[pos])) {
      return false;
    } else {
      //for fixed-length data, buffer must be large enough
      //otherwise, driver may write past the end
      fun_check_dbg(lengths_[pos] <= sizeof(T));
      val = value;
    }

    return true;
  }

  template <typename T>
  bool extractManualLOBImpl(size_t pos, fun::sql::LOB<T>& val, SQLSMALLINT cType);

  template <typename T>
  bool extractManualStringImpl(size_t pos, T& val, SQLSMALLINT cType);

  template <typename T, typename NT>
  bool extAny(size_t pos, T& val) {
    NT i;
    if (Extract(pos, i)) {
      val = i;
      return true;
    } else {
      val = Nullable<NT>();
      return false;
    }
  }

  /**
   * Utility function for extraction of Any and DynamicAny.
   */
  template <typename T>
  bool ExtractImpl(size_t pos, T& val) {
    OdbcMetaColumn column(stmt_, pos);

    switch (column.type()) {
      case MetaColumn::FDT_INT8: { return extAny<T, int8>(pos, val); }

      case MetaColumn::FDT_UINT8: { return extAny<T, uint8>(pos, val); }

      case MetaColumn::FDT_INT16: { return extAny<T, int16>(pos, val); }

      case MetaColumn::FDT_UINT16: { return extAny<T, uint16>(pos, val); }

      case MetaColumn::FDT_INT32: { return extAny<T, int32>(pos, val); }

      case MetaColumn::FDT_UINT32: { return extAny<T, uint32>(pos, val); }

      case MetaColumn::FDT_INT64: { return extAny<T, int64>(pos, val); }

      case MetaColumn::FDT_UINT64: { return extAny<T, uint64>(pos, val); }

      case MetaColumn::FDT_BOOL: { return extAny<T, bool>(pos, val); }

      case MetaColumn::FDT_FLOAT: { return extAny<T, float>(pos, val); }

      case MetaColumn::FDT_DOUBLE: { return extAny<T, double>(pos, val); }

      case MetaColumn::FDT_STRING: { return extAny<T, String>(pos, val); }

      case MetaColumn::FDT_WSTRING: { return extAny<T, fun::UString>(pos, val); }

      case MetaColumn::FDT_BLOB: { return extAny<T, fun::sql::BLOB>(pos, val); }

      case MetaColumn::FDT_CLOB: { return extAny<T, fun::sql::CLOB>(pos, val); }

      case MetaColumn::FDT_DATE: { return extAny<T, fun::sql::Date>(pos, val); }

      case MetaColumn::FDT_TIME: { return extAny<T, fun::sql::Time>(pos, val); }

      case MetaColumn::FDT_TIMESTAMP: { return extAny<T, fun::DateTime>(pos, val); }

      default:
        throw DataFormatException("Unsupported data type.");
    }

    return false;
  }

  /**
   * The reason for this utility wrapper are platforms where
   * SQLLEN macro (a.k.a. SQLINTEGER) yields 64-bit value,
   * while SQL_NULL_DATA (#define'd as -1 literal) remains 32-bit.
   */
  bool IsNullLengthIndicator(SQLLEN val) const;

  SQLINTEGER columnSize(size_t pos) const;

  const StatementHandle& stmt_;
  PreparatorPtr preparator_;
  Preparator::DataExtraction data_extraction_;
  std::vector<SQLLEN> lengths_;
};


//
// inlines
//

inline bool Extractor::ExtractBoundImpl(size_t pos, fun::sql::BLOB& val) {
  return ExtractBoundImplLOB<BLOB::ValueType>(pos, val);
}

inline bool Extractor::ExtractBoundImpl(size_t pos, fun::sql::CLOB& val) {
  return ExtractBoundImplLOB<CLOB::ValueType>(pos, val);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos, std::vector<String>& values) {
  return ExtractBoundImplContainerString(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos, std::deque<String>& values) {
  return ExtractBoundImplContainerString(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos, std::list<String>& values) {
  return ExtractBoundImplContainerString(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos, std::vector<fun::UString>& values) {
  return ExtractBoundImplContainerString(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos, std::deque<fun::UString>& values) {
  return ExtractBoundImplContainerString(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos, std::list<fun::UString>& values) {
  return ExtractBoundImplContainerString(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos,
  std::vector<fun::sql::CLOB>& values) {
  return ExtractBoundImplContainerLOB(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos,
  std::deque<fun::sql::CLOB>& values) {
  return ExtractBoundImplContainerLOB(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos,
  std::list<fun::sql::CLOB>& values) {
  return ExtractBoundImplContainerLOB(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos,
  std::vector<fun::sql::BLOB>& values) {
  return ExtractBoundImplContainerLOB(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos,
  std::deque<fun::sql::BLOB>& values) {
  return ExtractBoundImplContainerLOB(pos, values);
}

inline bool Extractor::ExtractBoundImplContainer(size_t pos,
  std::list<fun::sql::BLOB>& values) {
  return ExtractBoundImplContainerLOB(pos, values);
}

inline void Extractor::SetDataExtraction(Preparator::DataExtraction ext) {
  preparator_->SetDataExtraction(data_extraction_ = ext);
}

inline Preparator::DataExtraction Extractor::GetDataExtraction() const {
  return data_extraction_;
}

inline void Extractor::Reset() {
  lengths_.clear();
}

inline void Extractor::ResizeLengths(size_t pos) {
  if (pos >= lengths_.size()) {
    lengths_.resize(pos + 1, (SQLLEN) 0);
  }
}

inline bool Extractor::IsNullLengthIndicator(SQLLEN val) const {
  return SQL_NULL_DATA == (int) val;
}

inline SQLINTEGER Extractor::columnSize(size_t pos) const {
  size_t size = OdbcMetaColumn(stmt_, pos).length();
  size_t maxSize = preparator_->maxDataSize(pos);
  if (size > maxSize) {
    size = maxSize;
  }
  return (SQLINTEGER) size;
}

} // namespace odbc
} // namespace sql
} // namespace fun
