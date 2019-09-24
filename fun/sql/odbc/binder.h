#pragma once

#include "fun/sql/odbc/odbc.h"
#include "fun/sql/binder_base.h"
#include "fun/sql/lob.h"
#include "fun/sql/odbc/handle.h"
#include "fun/sql/odbc/parameter.h"
#include "fun/sql/odbc/odbc_meta_column.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/odbc/type_info.h"
#include "fun/base/exception.h"

#include <vector>
#include <deque>
#include <list>
#include <map>

#include <sqlext.h>

namespace fun {

class DateTime;

namespace sql {

class Date;
class Time;

namespace odbc {

/**
 * Binds placeholders in the sql query to the provided values. Performs data types mapping.
 */
class FUN_ODBC_API Binder: public fun::sql::BinderBase {

  struct ParamDescriptor {
    ParamDescriptor() : colSize(0), cDataType(0), decDigits(-1) {}

    ParamDescriptor(SQLINTEGER colSize_, SQLSMALLINT cDataType_, SQLSMALLINT decDigits_)
      : colSize(colSize_), cDataType(cDataType_), decDigits(decDigits_) {}

    bool IsDefined() const { return cDataType != 0; }

    SQLINTEGER colSize;
    SQLSMALLINT cDataType;
    SQLSMALLINT decDigits;
  };

 public:
  typedef BinderBase::Direction Direction;
  typedef std::map<SQLPOINTER, SQLLEN> ParamMap;

  static const size_t DEFAULT_PARAM_SIZE = 1024;

  enum ParameterBinding {
    PB_IMMEDIATE,
    PB_AT_EXEC
  };

  /**
   * Creates the Binder.
   */
  Binder( const StatementHandle& stmt,
          size_t maxFieldSize,
          ParameterBinding data_binding,
          TypeInfo* pDataTypes,
          bool insert_only);

  /**
   * Destroys the Binder.
   */
  ~Binder();

  void Bind(size_t pos, const int8& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<int8>& val, Direction dir);
  void Bind(size_t pos, const std::deque<int8>& val, Direction dir);
  void Bind(size_t pos, const std::list<int8>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<int8> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<int8> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<int8> >& val, Direction dir);
  void Bind(size_t pos, const uint8& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<uint8>& val, Direction dir);
  void Bind(size_t pos, const std::deque<uint8>& val, Direction dir);
  void Bind(size_t pos, const std::list<uint8>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<uint8> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<uint8> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<uint8> >& val, Direction dir);
  void Bind(size_t pos, const int16& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<int16>& val, Direction dir);
  void Bind(size_t pos, const std::deque<int16>& val, Direction dir);
  void Bind(size_t pos, const std::list<int16>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<int16> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<int16> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<int16> >& val, Direction dir);
  void Bind(size_t pos, const uint16& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<uint16>& val, Direction dir);
  void Bind(size_t pos, const std::deque<uint16>& val, Direction dir);
  void Bind(size_t pos, const std::list<uint16>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<uint16> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<uint16> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<uint16> >& val, Direction dir);
  void Bind(size_t pos, const int32& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<int32>& val, Direction dir);
  void Bind(size_t pos, const std::deque<int32>& val, Direction dir);
  void Bind(size_t pos, const std::list<int32>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<int32> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<int32> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<int32> >& val, Direction dir);
  void Bind(size_t pos, const uint32& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<uint32>& val, Direction dir);
  void Bind(size_t pos, const std::deque<uint32>& val, Direction dir);
  void Bind(size_t pos, const std::list<uint32>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<uint32> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<uint32> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<uint32> >& val, Direction dir);
  void Bind(size_t pos, const int64& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<int64>& val, Direction dir);
  void Bind(size_t pos, const std::deque<int64>& val, Direction dir);
  void Bind(size_t pos, const std::list<int64>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<int64> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<int64> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<int64> >& val, Direction dir);
  void Bind(size_t pos, const uint64& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<uint64>& val, Direction dir);
  void Bind(size_t pos, const std::deque<uint64>& val, Direction dir);
  void Bind(size_t pos, const std::list<uint64>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<uint64> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<uint64> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<uint64> >& val, Direction dir);

#ifndef FUN_LONG_IS_64_BIT
  void Bind(size_t pos, const long& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const unsigned long& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<long>& val, Direction dir);
  void Bind(size_t pos, const std::deque<long>& val, Direction dir);
  void Bind(size_t pos, const std::list<long>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<long> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<long> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<long> >& val, Direction dir);
#endif

  void Bind(size_t pos, const bool& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<bool>& val, Direction dir);
  void Bind(size_t pos, const std::deque<bool>& val, Direction dir);
  void Bind(size_t pos, const std::list<bool>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<bool> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<bool> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<bool> >& val, Direction dir);
  void Bind(size_t pos, const float& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<float>& val, Direction dir);
  void Bind(size_t pos, const std::deque<float>& val, Direction dir);
  void Bind(size_t pos, const std::list<float>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<float> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<float> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<float> >& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<double> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<double> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<double> >& val, Direction dir);
  void Bind(size_t pos, const double& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<double>& val, Direction dir);
  void Bind(size_t pos, const std::deque<double>& val, Direction dir);
  void Bind(size_t pos, const std::list<double>& val, Direction dir);
  void Bind(size_t pos, const char& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<char>& val, Direction dir);
  void Bind(size_t pos, const std::deque<char>& val, Direction dir);
  void Bind(size_t pos, const std::list<char>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<char> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<char> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<char> >& val, Direction dir);
  void Bind(size_t pos, const String& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<String>& val, Direction dir);
  void Bind(size_t pos, const std::deque<String>& val, Direction dir);
  void Bind(size_t pos, const std::list<String>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<String> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<String> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<String> >& val, Direction dir);
  void Bind(size_t pos, const UString& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<UString>& val, Direction dir);
  void Bind(size_t pos, const std::deque<UString>& val, Direction dir);
  void Bind(size_t pos, const std::list<UString>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<UString> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<UString> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<UString> >& val, Direction dir);
  void Bind(size_t pos, const BLOB& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const CLOB& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<BLOB>& val, Direction dir);
  void Bind(size_t pos, const std::deque<BLOB>& val, Direction dir);
  void Bind(size_t pos, const std::list<BLOB>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<BLOB> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<BLOB> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<BLOB> >& val, Direction dir);
  void Bind(size_t pos, const std::vector<CLOB>& val, Direction dir);
  void Bind(size_t pos, const std::deque<CLOB>& val, Direction dir);
  void Bind(size_t pos, const std::list<CLOB>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<CLOB> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<CLOB> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<CLOB> >& val, Direction dir);
  void Bind(size_t pos, const Date& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<Date>& val, Direction dir);
  void Bind(size_t pos, const std::deque<Date>& val, Direction dir);
  void Bind(size_t pos, const std::list<Date>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<Date> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<Date> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<Date> >& val, Direction dir);
  void Bind(size_t pos, const Time& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<Time>& val, Direction dir);
  void Bind(size_t pos, const std::deque<Time>& val, Direction dir);
  void Bind(size_t pos, const std::list<Time>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<Time> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<Time> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<Time> >& val, Direction dir);
  void Bind(size_t pos, const DateTime& val, Direction dir, const WhenNullCb& null_cb);
  void Bind(size_t pos, const std::vector<DateTime>& val, Direction dir);
  void Bind(size_t pos, const std::deque<DateTime>& val, Direction dir);
  void Bind(size_t pos, const std::list<DateTime>& val, Direction dir);
  void Bind(size_t pos, const std::vector<Nullable<DateTime> >& val, Direction dir);
  void Bind(size_t pos, const std::deque<Nullable<DateTime> >& val, Direction dir);
  void Bind(size_t pos, const std::list<Nullable<DateTime> >& val, Direction dir);
  void Bind(size_t pos, const NullData& val, Direction dir, const std::type_info& bind_type);
  void Bind(size_t pos, const std::vector<NullData>& val, Direction dir, const std::type_info& bind_type);
  void Bind(size_t pos, const std::deque<NullData>& val, Direction dir, const std::type_info& bind_type);
  void Bind(size_t pos, const std::list<NullData>& val, Direction dir, const std::type_info& bind_type);

  /**
   * Set data binding type.
   */
  void SetDataBinding(ParameterBinding binding);

  /**
   * Return data binding type.
   */
  ParameterBinding GetDataBinding() const;

  /**
   * Returns bound data size for parameter at specified position.
   */
  size_t GetParameterSize(SQLPOINTER pAddr) const;

  /**
   * Transfers the results of non-POD outbound parameters from internal
   * holders back into the externally supplied buffers.
   */
  void Synchronize();

  /**
   * Clears the cached storage.
   */
  void Reset();

 private:
  typedef std::vector<ParamDescriptor>                     ParameterInfoVec;
  typedef std::vector<SQLLEN*>                             LengthPtrVec;
  typedef std::vector<SQLLEN>                              LengthVec;
  typedef std::vector<LengthVec*>                          LengthVecVec;
  typedef std::vector<char*>                               CharPtrVec;
  typedef std::vector<UTF16Char*>                          UTF16CharPtrVec;
  typedef std::vector<bool*>                               BoolPtrVec;
  typedef std::vector<SQL_DATE_STRUCT>                     DateVec;
  typedef std::vector<DateVec*>                            DateVecVec;
  typedef std::vector<SQL_TIME_STRUCT>                     TimeVec;
  typedef std::vector<TimeVec*>                            TimeVecVec;
  typedef std::vector<SQL_TIMESTAMP_STRUCT>                DateTimeVec;
  typedef std::vector<DateTimeVec*>                        DateTimeVecVec;
  typedef std::vector<fun::Any*>                          AnyPtrVec;
  typedef std::vector<AnyPtrVec>                           AnyPtrVecVec;
  typedef std::map<char*, String*>                    StringMap;
  typedef std::map<UString::value_type*, UString*> UTF16StringMap;
  typedef std::map<SQL_DATE_STRUCT*, Date*>                DateMap;
  typedef std::map<SQL_TIME_STRUCT*, Time*>                TimeMap;
  typedef std::map<SQL_TIMESTAMP_STRUCT*, DateTime*>       TimestampMap;
  typedef std::map<SQLLEN*, WhenNullCb>                    NullCbMap;

  /**
   * Sets the description field for the parameter, if needed.
   */
  void DescribeParameter(size_t pos);

  /**
   * Binds a const char ptr.
   * This is a private no-op in this implementation
   * due to security risk.
   */
  void Bind(size_t pos, const char* const& pVal, Direction dir, const WhenNullCb& null_cb);

  /**
   * Returns ODBC parameter direction based on the parameter binding direction
   * specified by user.
   */
  SQLSMALLINT ToOdbcDirection(Direction dir) const;

  template <typename T>
  void BindImpl(size_t pos, T& val, SQLSMALLINT cDataType, Direction dir, const WhenNullCb& null_cb) {
    SQLINTEGER colSize = 0;
    SQLSMALLINT decDigits = 0;
    GetColSizeAndPrecision(pos, cDataType, colSize, decDigits);
    SQLLEN* pLenIn = NULL;
    if (IsOutBound(dir) && null_cb.IsDefined()) {
      pLenIn = new SQLLEN;
      *pLenIn = SQL_NTS; // microsoft example does that, otherwise no null indicator is returned
      _nullCbMap.insert(NullCbMap::value_type(pLenIn, null_cb));
    }
    _lengthIndicator.push_back(pLenIn);

    if (Utility::IsError(SQLBindParameter(stmt_,
                    (SQLUSMALLINT) (pos + 1),
                    ToOdbcDirection(dir),
                    cDataType,
                    static_cast<SQLSMALLINT>(Utility::sqlDataType(cDataType)),
                    colSize,
                    decDigits,
                    (SQLPOINTER)& val, 0,
                    _lengthIndicator.back()))) {
      throw StatementException(stmt_, "SQLBindParameter()");
    }
  }

  template <typename L>
  void BindImplLOB(size_t pos, const L& val, Direction dir, const WhenNullCb& null_cb) {
    if (IsOutBound(dir) || !IsInBound(dir)) {
      throw NotImplementedException("LOB parameter type can only be inbound.");
    }

    SQLPOINTER pVal = (SQLPOINTER) val.GetRawContent();
    SQLINTEGER size = (SQLINTEGER) val.size();

    in_params_.insert(ParamMap::value_type(pVal, size));

    SQLLEN* pLenIn = new SQLLEN;
    *pLenIn  = size;

    if (PB_AT_EXEC == param_binding_) {
      *pLenIn  = SQL_LEN_DATA_AT_EXEC(size);
    }

    if (IsOutBound(dir) && null_cb.IsDefined()) {
      _nullCbMap.insert(NullCbMap::value_type(pLenIn, null_cb));
    }

    _lengthIndicator.push_back(pLenIn);
    SQLSMALLINT sqlType = (IsInBound(dir) && size <= _maxVarBinColSize) ? SQL_VARBINARY : SQL_LONGVARBINARY;

    if (Utility::IsError(SQLBindParameter(stmt_,
                                          (SQLUSMALLINT) pos + 1,
                                          SQL_PARAM_INPUT,
                                          SQL_C_BINARY,
                                          sqlType,
                                          (SQLUINTEGER) size,
                                          0,
                                          pVal,
                                          (SQLINTEGER) size,
                                          _lengthIndicator.back()))) {
      throw StatementException(stmt_, "SQLBindParameter(LOB)");
    }
  }

  template <typename T>
  size_t GetValueSize(const T& val) {
    return 0;
  }

  size_t GetValueSize(const String& val) {
    return val.length();
  }

  size_t GetValueSize(const UString& val) {
    return val.length() * sizeof(UTF16Char);
  }

  template <typename T>
  size_t GetValueSize(const LOB<T>& val) {
    return val.size();
  }

  template <typename T>
  size_t GetValueSize(const Nullable<T>& val) {
    if (val.IsNull()) {
      return 0;
    }

    return GetValueSize(val.value());
  }

  template <typename T>
  SQLLEN GetLengthIndicator(const T& val) {
    return GetValueSize(val);
  }

  SQLLEN GetLengthIndicator(const NullData& val) {
    return SQL_NULL_DATA;
  }

  template <typename T>
  SQLLEN GetLengthIndicator(const Nullable<T>& val) {
    if (val.IsNull()) {
      return SQL_NULL_DATA;
    } else {
      return GetLengthIndicator(val.value());
    }
  }

  template <typename C>
  void InitLengthIndicator(size_t pos, const C& val, size_t length) {
    SetParamSetSize(length);

    if (_vecLengthIndicator.size() <= pos) {
      _vecLengthIndicator.resize(pos + 1);
      LengthVec* lenVec = new LengthVec(length);
      _vecLengthIndicator[pos] = lenVec;
      LengthVec::iterator itLen = lenVec->begin();
      for (typename C::const_iterator it = val.begin(); it != val.end(); ++it, ++itLen) {
        *itLen = GetLengthIndicator(*it);
      }
    }
  }

  template <typename T>
  bool IsVariableSizeType(const T*) {
    return false;
  }

  bool IsVariableSizeType(const String*) {
    return true;
  }

  bool IsVariableSizeType(const UString*) {
    return true;
  }

  template <typename T>
  bool IsVariableSizeType(const LOB<T>*) {
    return true;
  }

  template <typename T>
  bool IsVariableSizeType(const Nullable<T>*) {
    return IsVariableSizeType((T*)nullptr);
  }

  template <typename C>
  void GetColSizeAndPrecision(size_t pos,
                              const C& val,
                              SQLSMALLINT cDataType,
                              SQLINTEGER& colSize,
                              SQLSMALLINT& decDigits,
                              SQLINTEGER& bufSize) {
    if (IsVariableSizeType((typename C::value_type*)nullptr)) {
      size_t maxSize = 0;
      for (typename C::const_iterator it = val.begin(); it != val.end(); ++it) {
        maxSize = std::max(GetValueSize(*it), maxSize);
      }

      colSize = static_cast<SQLINTEGER>(maxSize);
      decDigits = 0;
      bufSize = static_cast<SQLINTEGER>(maxSize);
    } else {
      GetColSizeAndPrecision(pos, cDataType, colSize, decDigits);
      bufSize = 0;
    }
  }

  template <typename T>
  void Next(T& target, SQLINTEGER) {
    target++;
  }

  template <typename T>
  void Next(T*& target, SQLINTEGER) {
    target++;
  }

  void Next(char*& target, SQLINTEGER bufSize) {
    target += bufSize;
  }

  void Next(UTF16Char*& target, SQLINTEGER bufSize) {
    target += bufSize / 2;
  }

  template <typename T>
  void CopyValue(T& target, const T& val, SQLINTEGER) {
    target = val;
  }

  void CopyValue(char& target, const String& str, SQLINTEGER bufSize) {
    size_t strSize = str.length();
    if (strSize > bufSize) {
      throw LengthExceededException("CopyValue(std::vector of strings)");
    }

    UnsafeMemory::Memcpy(&target, str.c_str(), strSize);
  }

  void CopyValue(UTF16Char& target, const UString& str, SQLINTEGER bufSize) {
    size_t strSize = str.length();
    if (strSize > bufSize) {
      throw LengthExceededException("CopyValue(std::vector of strings)");
    }

    UnsafeMemory::Memcpy(&target, str.c_str(), strSize * sizeof(UTF16Char));
  }

  template <typename T>
  void CopyValue(char& target, const LOB<T>& lob, SQLINTEGER bufSize) {
    size_t lobSize = lob.size();
    if (lobSize > bufSize) {
      throw LengthExceededException("CopyValue(std::vector of LOB)");
    }

    UnsafeMemory::Memcpy(&target, lob.GetRawContent(), lobSize * sizeof(typename LOB<T>::ValueType));
  }

  void CopyValue(SQL_DATE_STRUCT& target, const Date& dt, SQLINTEGER) {
    Utility::DateSync(target, dt);
  }

  void CopyValue(SQL_TIME_STRUCT& target, const Time& dt, SQLINTEGER) {
    Utility::TimeSync(target, dt);
  }

  void CopyValue(SQL_TIMESTAMP_STRUCT& target, const DateTime& dt, SQLINTEGER) {
    Utility::DateTimeSync(target, dt);
  }

  template <typename T, typename C>
  std::vector<T>* AllocBuffer(size_t pos, size_t bufSize, const C& val, const T*) {
    if (containers_.size() <= pos) {
      containers_.resize(pos + 1);
    }

    containers_[pos].push_back(new Any(std::vector<T>(val.size())));
    std::vector<T>& cont = RefAnyCast<std::vector<T> >(*containers_[pos].back());
    return &cont;
  }

  template <typename C>
  char* AllocBuffer(size_t pos, size_t bufSize, const C& val, const String*) {
    if (_charPtrs.size() <= pos) {
      _charPtrs.resize(pos + 1, 0);
    }

    _charPtrs[pos] = (char*)std::calloc(val.size() * bufSize, sizeof(char));
    return _charPtrs[pos];
  }

  template <typename C>
  UTF16Char* AllocBuffer(size_t pos, size_t bufSize, const C& val, const UString*) {
    if (_utf16CharPtrs.size() <= pos) {
      _utf16CharPtrs.resize(pos + 1, 0);
    }

    _utf16CharPtrs[pos] = (UTF16Char*)std::calloc(val.size() * bufSize, sizeof(UTF16Char));
    return _utf16CharPtrs[pos];
  }

  template <typename T, typename C>
  char* AllocBuffer(size_t pos, size_t bufSize, const C& val, const LOB<T>*) {
    if (_charPtrs.size() <= pos) {
      _charPtrs.resize(pos + 1, 0);
    }

    _charPtrs[pos] = (char*)std::calloc(val.size() * bufSize, sizeof(typename LOB<T>::ValueType));
    return _charPtrs[pos];
  }

  template <typename C>
  DateVec* AllocBuffer(size_t pos, size_t bufSize, const C& val, const Date*) {
    if (_dateVecVec.size() <= pos) {
      _dateVecVec.resize(pos + 1, 0);
    }

    _dateVecVec[pos] = new DateVec(val.size());
    return _dateVecVec[pos];
  }

  template <typename C>
  TimeVec* AllocBuffer(size_t pos, size_t bufSize, const C& val, const Time*) {
    if (_timeVecVec.size() <= pos) {
      _timeVecVec.resize(pos + 1, 0);
    }

    _timeVecVec[pos] = new TimeVec(val.size());
    return _timeVecVec[pos];
  }

  template <typename C>
  DateTimeVec* AllocBuffer(size_t pos, size_t bufSize, const C& val, const DateTime*) {
    if (_dateTimeVecVec.size() <= pos) {
      _dateTimeVecVec.resize(pos + 1, 0);
    }

    _dateTimeVecVec[pos] = new DateTimeVec(val.size());
    return _dateTimeVecVec[pos];
  }

  template <typename C>
  bool* AllocBuffer(size_t pos, size_t bufSize, const C& val, const bool*) {
    if (_boolPtrs.size() <= pos) {
      _boolPtrs.resize(pos + 1);
    }

    _boolPtrs[pos] = new bool[val.size()];
    return _boolPtrs[pos];
  }

  template <typename T, typename C>
  auto* AllocBuffer(size_t pos, size_t bufSize, const C& val, const Nullable<T>* dummy) {
    return AllocBuffer(pos, bufSize, val, (const T*)nullptr);
  }

  template <typename C, typename OUTPUT, typename T>
  void CopyContainerValues(OUTPUT out, size_t bufSize, const C& val, const T*) {
    for (typename C::const_iterator it = val.begin(); it != val.end(); ++it) {
      CopyValue(*out, *it, static_cast<SQLINTEGER>(bufSize));
      next(out, static_cast<SQLINTEGER>(bufSize));
    }
  }

  template <typename C, typename OUTPUT, typename T>
  void CopyContainerValues(OUTPUT out, size_t bufSize, const C& val, const Nullable<T>*) {
    for (typename C::const_iterator it = val.begin(); it != val.end(); ++it) {
      if (!it->IsNull()) {
        CopyValue(*out, it->value(), static_cast<SQLINTEGER>(bufSize));
      }
      next(out, static_cast<SQLINTEGER>(bufSize));
    }
  }

  template <typename T>
  typename std::vector<T>::iterator GetOutput(std::vector<T>* cont) {
    return cont->begin();
  }

  template <typename T>
  T* GetOutput(T* cont) {
    return cont;
  }

  template <typename T>
  SQLPOINTER GetContainerPointer(const std::vector<T>* cont) {
    return (SQLPOINTER)&((*cont)[0]);
  }

  template <typename T>
  SQLPOINTER GetContainerPointer(const T* cont) {
    return (SQLPOINTER)cont;
  }

  template <typename C>
  SQLPOINTER CopyContainer(size_t pos, size_t bufSize, const C& val) {
    auto* cont = AllocBuffer(pos, bufSize, val, (const typename C::value_type*)nullptr);
    CopyContainerValues(GetOutput(cont), bufSize, val, (const typename C::value_type*)nullptr);
    return GetContainerPointer(cont);
  }

  SQLPOINTER CopyContainer(size_t pos, size_t bufSize, const std::vector<bool>& val) {
    bool* cont = AllocBuffer(pos, bufSize, val, (const bool*)nullptr);
    CopyContainerValues(GetOutput(cont), bufSize, val, (const bool*)nullptr);
    return cont;
  }

  template <typename T>
  bool IsNativeType(const T*) {
    return true;
  }

  bool IsNativeType(const Date*) {
    return false;
  }

  bool IsNativeType(const Time*) {
    return false;
  }

  bool IsNativeType(const DateTime*) {
    return false;
  }

  template <typename T>
  SQLPOINTER CopyContainer(size_t pos, size_t bufSize, const std::vector<T>& val) {
    if (IsVariableSizeType((const T*)nullptr) || !IsNativeType((const T*)nullptr)) {
      return CopyContainer<std::vector<T>>(pos, bufSize, val);
    } else {
      return (SQLPOINTER)&val[0];
    }
  }

  template <typename T>
  SQLPOINTER CopyContainer(size_t pos, size_t bufSize, const std::vector<Nullable<T>>& val) {
    auto cont = AllocBuffer(pos, bufSize, val, (const Nullable<T>*)nullptr);
    CopyContainerValues(GetOutput(cont), bufSize, val, (const Nullable<T>*)nullptr);
    return GetContainerPointer(cont);
  }

  template <typename T>
  SQLSMALLINT GetSqlType(const T*, SQLSMALLINT cDataType, size_t colSize) {
    return Utility::sqlDataType(cDataType);
  }

  SQLSMALLINT GetSqlType(const String*, SQLSMALLINT cDataType, size_t colSize) {
    if (colSize > _maxCharColLength) {
      return SQL_LONGVARCHAR;
    }

    return SQL_VARCHAR;
  }

  SQLSMALLINT GetSqlType(const UString*, SQLSMALLINT cDataType, size_t colSize) {
    if (colSize > _maxWCharColLength) {
      return SQL_WLONGVARCHAR;
    }

    return SQL_WVARCHAR;
  }

  template <typename T>
  SQLSMALLINT GetSqlType(const LOB<T>*, SQLSMALLINT cDataType, size_t colSize) {
    if (colSize > _maxVarBinColSize) {
      return SQL_LONGVARBINARY;
    }

    return SQL_VARBINARY;
  }

  template <typename T>
  SQLSMALLINT GetSqlType(const Nullable<T>* val, SQLSMALLINT cDataType, size_t colSize) {
    return GetSqlType((T*)nullptr, cDataType, colSize);
  }

  template <typename T>
  void ValidateDirection(Direction, const T*) {
  }

  void ValidateDirection(Direction dir, const String*) {
    if (IsOutBound(dir) || !IsInBound(dir)) {
      throw NotImplementedException("String container parameter type can only be inbound.");
    }
  }

  void ValidateDirection(Direction dir, const UString*) {
    if (IsOutBound(dir) || !IsInBound(dir)) {
      throw NotImplementedException("UString container parameter type can only be inbound.");
    }
  }

  template <typename T>
  void ValidateDirection(Direction dir, const LOB<T>*) {
    if (IsOutBound(dir) || !IsInBound(dir)) {
      throw NotImplementedException("LOB container parameter type can only be inbound.");
    }
  }

  void ValidateDirection(Direction dir, const Date*) {
    if (IsOutBound(dir) || !IsInBound(dir)) {
      throw NotImplementedException("Date container parameter type can only be inbound.");
    }
  }

  void ValidateDirection(Direction dir, const Time*) {
    if (IsOutBound(dir) || !IsInBound(dir)) {
      throw NotImplementedException("Time container parameter type can only be inbound.");
    }
  }

  void ValidateDirection(Direction dir, const DateTime*) {
    if (IsOutBound(dir) || !IsInBound(dir)) {
      throw NotImplementedException("DateTime container parameter type can only be inbound.");
    }
  }

  template <typename T>
  void ValidateDirection(Direction dir, const Nullable<T>*) {
    ValidateDirection(dir, (T*)nullptr);
  }

  template <typename C>
  void BindImplContainer(size_t pos, const C& val, SQLSMALLINT cDataType, Direction dir) {
    if (val.IsEmpty()) {
      throw InvalidArgumentException("Empty container not allowed.");
    }

    if (PB_IMMEDIATE != param_binding_) {
      throw InvalidAccessException("container can only be bound immediately.");
    }

    ValidateDirection(dir, (typename C::value_type*)nullptr);

    const size_t length = val.size();
    InitLengthIndicator(pos, val, length);

    SQLINTEGER colSize;
    SQLINTEGER bufSize;
    SQLSMALLINT decDigits;
    GetColSizeAndPrecision(pos, val, cDataType, colSize, decDigits, bufSize);
    SQLPOINTER cont = CopyContainer(pos, bufSize, val);

    if (Utility::IsError(SQLBindParameter(stmt_,
                (SQLUSMALLINT) pos + 1,
                ToOdbcDirection(dir),
                cDataType,
                GetSqlType((typename C::value_type*)NULL, cDataType, colSize),
                colSize,
                decDigits,
                cont,
                bufSize,
                &(*_vecLengthIndicator[pos])[0]))) {
      throw StatementException(stmt_, "SQLBindParameter()");
    }
  }

  /**
   * Used to retrieve column size and precision.
   * Not all drivers cooperate with this inquiry under all circumstances
   * This function runs for query and stored procedure parameters (in and
   * out-bound). Some drivers, however, do not care about knowing this
   * information to start with. For that reason, after all the attempts
   * to discover the required values are unsuccessfully exhausted, the values
   * are both set to zero and no exception is thrown.
   * However, if the colSize is succesfully retrieved and it is greater than
   * session-wide maximum allowed field size, LengthExceededException is thrown.
   */
  void GetColSizeAndPrecision(size_t pos,
                              SQLSMALLINT cDataType,
                              SQLINTEGER& colSize,
                              SQLSMALLINT& decDigits,
                              size_t actualSize = 0);

  /**
   * Sets the parameter set size. Used for column-wise binding.
   */
  void SetParamSetSize(size_t length);

  /**
   * Fills the column or parameter size into the 'size' argument.
   * Does nothing if neither can be obtained from the driver, so
   * size should be set to some default value prior to calling this
   * function in order to avoid undefined size value.
   */
  void getColumnOrParameterSize(size_t pos, SQLINTEGER& size);

  /**
   * Frees all dynamically allocated memory resources.
   */
  void FreeMemory();

  const StatementHandle& stmt_;

  LengthPtrVec     _lengthIndicator;
  LengthVecVec     _vecLengthIndicator;

  ParamMap         in_params_;
  ParamMap         out_params_;
  ParameterBinding param_binding_;
  ParameterInfoVec parameters_;

  DateMap          dates_;
  TimeMap          times_;
  TimestampMap     _timestamps;
  StringMap        _strings;
  UTF16StringMap   _utf16Strings;

  DateVecVec       _dateVecVec;
  TimeVecVec       _timeVecVec;
  DateTimeVecVec   _dateTimeVecVec;
  CharPtrVec       _charPtrs;
  UTF16CharPtrVec  _utf16CharPtrs;
  BoolPtrVec       _boolPtrs;
  const TypeInfo*  _pTypeInfo;
  SQLINTEGER       param_set_size_;
  size_t      max_field_size_;
  AnyPtrVecVec     containers_;
  size_t      _maxCharColLength;
  size_t      _maxWCharColLength;
  size_t      _maxVarBinColSize;
  NullCbMap        _nullCbMap;
  bool             insert_only_;
};


//
// inlines
//

inline void Binder::Bind(size_t pos, const int8& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_STINYINT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<int8>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<int8>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<int8>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<int8> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<int8> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<int8> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const uint8& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_UTINYINT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<uint8>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UTINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<uint8>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UTINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<uint8>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UTINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<uint8> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UTINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<uint8> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UTINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<uint8> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UTINYINT, dir);
}

inline void Binder::Bind(size_t pos, const int16& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_SSHORT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<int16>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SSHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<int16>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SSHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<int16>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SSHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<int16> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SSHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<int16> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SSHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<int16> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SSHORT, dir);
}

inline void Binder::Bind(size_t pos, const uint16& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_USHORT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<uint16>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_USHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<uint16>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_USHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<uint16>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_USHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<uint16> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_USHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<uint16> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_USHORT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<uint16> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_USHORT, dir);
}

inline void Binder::Bind(size_t pos, const int32& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_SLONG, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<int32>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<int32>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::list<int32>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<int32> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<int32> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<int32> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const uint32& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_ULONG, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<uint32>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_ULONG, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<uint32>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_ULONG, dir);
}

inline void Binder::Bind(size_t pos, const std::list<uint32>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_ULONG, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<uint32> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_ULONG, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<uint32> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_ULONG, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<uint32> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_ULONG, dir);
}

inline void Binder::Bind(size_t pos, const int64& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_SBIGINT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<int64>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<int64>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<int64>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<int64> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<int64> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<int64> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const uint64& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_UBIGINT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<uint64>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<uint64>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<uint64>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<uint64> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<uint64> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UBIGINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<uint64> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_UBIGINT, dir);
}

#ifndef FUN_LONG_IS_64_BIT
inline void Binder::Bind(size_t pos, const long& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_SLONG, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const unsigned long& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_SLONG, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<long>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<long>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::list<long>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<long> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<long> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<long> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_SLONG, dir);
}
#endif

inline void Binder::Bind(size_t pos, const float& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_FLOAT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<float>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_FLOAT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<float>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_FLOAT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<float>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_FLOAT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<float> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_FLOAT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<float> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_FLOAT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<float> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_FLOAT, dir);
}

inline void Binder::Bind(size_t pos, const double& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_DOUBLE, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<double>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_DOUBLE, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<double>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_DOUBLE, dir);
}

inline void Binder::Bind(size_t pos, const std::list<double>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_DOUBLE, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<double> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_DOUBLE, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<double> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_DOUBLE, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<double> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_DOUBLE, dir);
}

inline void Binder::Bind(size_t pos, const bool& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_BIT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<bool>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BIT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<bool>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BIT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<bool>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BIT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<bool> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BIT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<bool> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BIT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<bool> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BIT, dir);
}

inline void Binder::Bind(size_t pos, const char& val, Direction dir, const WhenNullCb& null_cb) {
  BindImpl(pos, val, SQL_C_STINYINT, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<char>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<char>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<char>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<char> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<char> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<char> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<String>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_CHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<String>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_CHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::list<String>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_CHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<String> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_CHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<String> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_CHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<String> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_CHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<UString>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_WCHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<UString>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_WCHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::list<UString>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_WCHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<UString> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_WCHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<UString> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_WCHAR, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<UString> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_WCHAR, dir);
}

inline void Binder::Bind(size_t pos, const BLOB& val, Direction dir, const WhenNullCb& null_cb) {
  BindImplLOB<BLOB>(pos, val, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const CLOB& val, Direction dir, const WhenNullCb& null_cb) {
  BindImplLOB<CLOB>(pos, val, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const std::vector<BLOB>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<BLOB>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::list<BLOB>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<BLOB> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<BLOB> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<BLOB> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<CLOB>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<CLOB>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::list<CLOB>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<CLOB> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<CLOB> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<CLOB> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_BINARY, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Date>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_DATE, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Date>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_DATE, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Date>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_DATE, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<Date> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_DATE, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<Date> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_DATE, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<Date> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_DATE, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Time>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIME, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Time>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIME, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Time>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIME, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<Time> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIME, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<Time> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIME, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<Time> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIME, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<DateTime>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIMESTAMP, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<DateTime>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIMESTAMP, dir);
}

inline void Binder::Bind(size_t pos, const std::list<DateTime>& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIMESTAMP, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<Nullable<DateTime> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIMESTAMP, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<Nullable<DateTime> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIMESTAMP, dir);
}

inline void Binder::Bind(size_t pos, const std::list<Nullable<DateTime> >& val, Direction dir) {
  BindImplContainer(pos, val, SQL_C_TYPE_TIMESTAMP, dir);
}

inline void Binder::Bind(size_t pos, const std::vector<NullData>& val, Direction dir, const std::type_info& bindElemType) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::deque<NullData>& val, Direction dir, const std::type_info& bindElemType) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::Bind(size_t pos, const std::list<NullData>& val, Direction dir, const std::type_info& bindElemType) {
  BindImplContainer(pos, val, SQL_C_STINYINT, dir);
}

inline void Binder::SetDataBinding(Binder::ParameterBinding binding) {
  param_binding_ = binding;
}

inline Binder::ParameterBinding Binder::GetDataBinding() const {
  return param_binding_;
}

} // namespace odbc
} // namespace sql
} // namespace fun
