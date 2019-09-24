#include "fun/sql/ODBC/Binder.h"
#include <sql.h>
#include "fun/base/date_time.h"
#include "fun/base/exception.h"
#include "fun/sql/ODBC/OdbcException.h"
#include "fun/sql/ODBC/Utility.h"
#include "fun/sql/lob.h"

namespace fun {
namespace sql {
namespace odbc {

static void GetProp(const TypeInfo& dataTypes, SQLSMALLINT sqlType,
                    size_t& val) {
  const String NM("COLUMN_SIZE");
  fun::dynamicAny r;
  if (dataTypes.tryGetInfo(sqlType, NM, r)) {
#ifndef FUN_LONG_IS_64_BIT
    long sz = r.convert<long>();
#else
    int64 sz = r.convert<int64>();
#endif
    // Postgres driver returns SQL_NO_TOTAL(-4) in some cases
    if (sz >= 0) val = static_cast<size_t>(sz);
  }
}

Binder::Binder(const StatementHandle& stmt, size_t maxFieldSize,
               Binder::ParameterBinding data_binding, TypeInfo* pDataTypes,
               bool insert_only)
    : stmt_(stmt),
      param_binding_(data_binding),
      _pTypeInfo(pDataTypes),
      param_set_size_(0),
      max_field_size_(maxFieldSize),
      _maxCharColLength(1024),
      _maxWCharColLength(1024),
      _maxVarBinColSize(1024),
      insert_only_(insert_only) {
  GetProp(*_pTypeInfo, SQL_WVARCHAR, _maxWCharColLength);
  GetProp(*_pTypeInfo, SQL_VARCHAR, _maxCharColLength);
  GetProp(*_pTypeInfo, SQL_VARBINARY, _maxVarBinColSize);
}

Binder::~Binder() { FreeMemory(); }

void Binder::FreeMemory() {
  if (_lengthIndicator.size() > 0) {
    LengthPtrVec::iterator itLen = _lengthIndicator.begin();
    LengthPtrVec::iterator itLenEnd = _lengthIndicator.end();
    for (; itLen != itLenEnd; ++itLen) delete *itLen;
  }

  if (_vecLengthIndicator.size() > 0) {
    LengthVecVec::iterator itVecLen = _vecLengthIndicator.begin();
    LengthVecVec::iterator itVecLenEnd = _vecLengthIndicator.end();
    for (; itVecLen != itVecLenEnd; ++itVecLen) delete *itVecLen;
  }

  if (times_.size() > 0) {
    TimeMap::iterator itT = times_.begin();
    TimeMap::iterator itTEnd = times_.end();
    for (; itT != itTEnd; ++itT) delete itT->first;
  }

  if (dates_.size() > 0) {
    DateMap::iterator itD = dates_.begin();
    DateMap::iterator itDEnd = dates_.end();
    for (; itD != itDEnd; ++itD) delete itD->first;
  }

  if (_timestamps.size() > 0) {
    TimestampMap::iterator itTS = _timestamps.begin();
    TimestampMap::iterator itTSEnd = _timestamps.end();
    for (; itTS != itTSEnd; ++itTS) delete itTS->first;
  }

  if (_strings.size() > 0) {
    StringMap::iterator itStr = _strings.begin();
    StringMap::iterator itStrEnd = _strings.end();
    for (; itStr != itStrEnd; ++itStr) std::free(itStr->first);
  }

  if (_utf16Strings.size() > 0) {
    UTF16StringMap::iterator itStr = _utf16Strings.begin();
    UTF16StringMap::iterator itStrEnd = _utf16Strings.end();
    for (; itStr != itStrEnd; ++itStr) std::free(itStr->first);
  }

  if (_charPtrs.size() > 0) {
    CharPtrVec::iterator itChr = _charPtrs.begin();
    CharPtrVec::iterator endChr = _charPtrs.end();
    for (; itChr != endChr; ++itChr) std::free(*itChr);
  }

  if (_utf16CharPtrs.size() > 0) {
    UTF16CharPtrVec::iterator itUTF16Chr = _utf16CharPtrs.begin();
    UTF16CharPtrVec::iterator endUTF16Chr = _utf16CharPtrs.end();
    for (; itUTF16Chr != endUTF16Chr; ++itUTF16Chr) std::free(*itUTF16Chr);
  }

  if (_boolPtrs.size() > 0) {
    BoolPtrVec::iterator itBool = _boolPtrs.begin();
    BoolPtrVec::iterator endBool = _boolPtrs.end();
    for (; itBool != endBool; ++itBool) delete[] * itBool;
  }

  if (_dateVecVec.size() > 0) {
    DateVecVec::iterator itDateVec = _dateVecVec.begin();
    DateVecVec::iterator itDateVecEnd = _dateVecVec.end();
    for (; itDateVec != itDateVecEnd; ++itDateVec) delete *itDateVec;
  }

  if (_timeVecVec.size() > 0) {
    TimeVecVec::iterator itTimeVec = _timeVecVec.begin();
    TimeVecVec::iterator itTimeVecEnd = _timeVecVec.end();
    for (; itTimeVec != itTimeVecEnd; ++itTimeVec) delete *itTimeVec;
  }

  if (_dateTimeVecVec.size() > 0) {
    DateTimeVecVec::iterator itDateTimeVec = _dateTimeVecVec.begin();
    DateTimeVecVec::iterator itDateTimeVecEnd = _dateTimeVecVec.end();
    for (; itDateTimeVec != itDateTimeVecEnd; ++itDateTimeVec)
      delete *itDateTimeVec;
  }

  if (containers_.size() > 0) {
    AnyPtrVecVec::iterator itAnyVec = containers_.begin();
    AnyPtrVecVec::iterator itAnyVecEnd = containers_.end();
    for (; itAnyVec != itAnyVecEnd; ++itAnyVec) {
      AnyPtrVec::iterator b = itAnyVec->begin();
      AnyPtrVec::iterator e = itAnyVec->end();
      for (; b != e; ++b) delete *b;
    }
  }
}

void Binder::Bind(size_t pos, const String& val, Direction dir,
                  const WhenNullCb& null_cb) {
  SQLPOINTER pVal = 0;
  SQLINTEGER size = (SQLINTEGER)val.size();
  SQLINTEGER colSize = 0;
  SQLSMALLINT decDigits = 0;
  GetColSizeAndPrecision(pos, SQL_C_CHAR, colSize, decDigits, val.size());

  if (IsOutBound(dir)) {
    getColumnOrParameterSize(pos, size);
    char* pChar = (char*)std::calloc(size, sizeof(char));

    if (IsInOutBound(dir)) std::strcpy(pChar, val.c_str());

    pVal = (SQLPOINTER)pChar;
    out_params_.insert(ParamMap::value_type(pVal, size));
    _strings.insert(StringMap::value_type(pChar, const_cast<String*>(&val)));
  } else if (IsInBound(dir)) {
    pVal = (SQLPOINTER)val.c_str();
    in_params_.insert(ParamMap::value_type(pVal, size));
  } else
    throw InvalidArgumentException("Parameter must be [in] OR [out] bound.");

  SQLLEN* pLenIn = new SQLLEN;
  if (IsOutBound(dir) && null_cb.IsDefined())
    _nullCbMap.insert(NullCbMap::value_type(pLenIn, null_cb));

  *pLenIn = SQL_NTS;

  if (PB_AT_EXEC == param_binding_) *pLenIn = SQL_LEN_DATA_AT_EXEC(size);

  _lengthIndicator.push_back(pLenIn);

  SQLSMALLINT sqType =
      (size <= _maxCharColLength) ? SQL_VARCHAR : SQL_LONGVARCHAR;

  if (Utility::IsError(
          SQLBindParameter(stmt_, (SQLUSMALLINT)pos + 1, ToOdbcDirection(dir),
                           SQL_C_CHAR, sqType, (SQLUINTEGER)colSize, 0, pVal,
                           (SQLINTEGER)size, _lengthIndicator.back()))) {
    throw StatementException(stmt_, "SQLBindParameter(String)");
  }
}

void Binder::Bind(size_t pos, const UString& val, Direction dir,
                  const WhenNullCb& null_cb) {
  typedef UString::value_type CharT;

  SQLPOINTER pVal = 0;
  SQLINTEGER size = (SQLINTEGER)(val.size() * sizeof(CharT));
  SQLINTEGER colSize = 0;
  SQLSMALLINT decDigits = 0;
  GetColSizeAndPrecision(pos, SQL_C_WCHAR, colSize, decDigits,
                         val.size() * sizeof(CharT));

  if (IsOutBound(dir)) {
    getColumnOrParameterSize(pos, size);
    CharT* pChar = (CharT*)std::calloc(size, sizeof(CharT));
    pVal = (SQLPOINTER)pChar;
    if (IsInOutBound(dir)) std::copy(val.begin(), val.end(), pChar);
    out_params_.insert(ParamMap::value_type(pVal, size));
    _utf16Strings.insert(
        UTF16StringMap::value_type(pChar, const_cast<UString*>(&val)));
  } else if (IsInBound(dir)) {
    pVal = (SQLPOINTER)val.c_str();
    in_params_.insert(ParamMap::value_type(pVal, size));
  } else
    throw InvalidArgumentException("Parameter must be [in] OR [out] bound.");

  SQLLEN* pLenIn = new SQLLEN;
  if (IsOutBound(dir) && null_cb.IsDefined())
    _nullCbMap.insert(NullCbMap::value_type(pLenIn, null_cb));

  *pLenIn = SQL_NTS;

  if (PB_AT_EXEC == param_binding_) {
    *pLenIn = SQL_LEN_DATA_AT_EXEC(size);
  }

  _lengthIndicator.push_back(pLenIn);

  SQLSMALLINT sqType =
      (size <= _maxWCharColLength) ? SQL_WVARCHAR : SQL_WLONGVARCHAR;

  if (Utility::IsError(
          SQLBindParameter(stmt_, (SQLUSMALLINT)pos + 1, ToOdbcDirection(dir),
                           SQL_C_WCHAR, sqType, (SQLUINTEGER)colSize, 0, pVal,
                           (SQLINTEGER)size, _lengthIndicator.back()))) {
    throw StatementException(stmt_, "SQLBindParameter(String)");
  }
}

void Binder::Bind(size_t pos, const Date& val, Direction dir,
                  const WhenNullCb& null_cb) {
  SQLLEN* pLenIn = new SQLLEN;
  *pLenIn = SQL_NTS;  // microsoft example does that, otherwise no null
                      // indicator is returned

  _lengthIndicator.push_back(pLenIn);
  if (IsOutBound(dir) && null_cb.IsDefined())
    _nullCbMap.insert(NullCbMap::value_type(pLenIn, null_cb));
  SQL_DATE_STRUCT* pDS = new SQL_DATE_STRUCT;
  Utility::DateSync(*pDS, val);

  dates_.insert(DateMap::value_type(pDS, const_cast<Date*>(&val)));

  SQLINTEGER colSize = 0;
  SQLSMALLINT decDigits = 0;
  GetColSizeAndPrecision(pos, SQL_TYPE_DATE, colSize, decDigits);

  if (Utility::IsError(
          SQLBindParameter(stmt_, (SQLUSMALLINT)pos + 1, ToOdbcDirection(dir),
                           SQL_C_TYPE_DATE, SQL_TYPE_DATE, colSize, decDigits,
                           (SQLPOINTER)pDS, 0, _lengthIndicator.back()))) {
    throw StatementException(stmt_, "SQLBindParameter(Date)");
  }
}

void Binder::Bind(size_t pos, const Time& val, Direction dir,
                  const WhenNullCb& null_cb) {
  SQLLEN* pLenIn = new SQLLEN;
  *pLenIn = SQL_NTS;  // microsoft example does that, otherwise no null
                      // indicator is returned
  if (IsOutBound(dir) && null_cb.IsDefined())
    _nullCbMap.insert(NullCbMap::value_type(pLenIn, null_cb));

  _lengthIndicator.push_back(pLenIn);

  SQL_TIME_STRUCT* pTS = new SQL_TIME_STRUCT;
  Utility::TimeSync(*pTS, val);

  times_.insert(TimeMap::value_type(pTS, const_cast<Time*>(&val)));

  SQLINTEGER colSize = 0;
  SQLSMALLINT decDigits = 0;
  GetColSizeAndPrecision(pos, SQL_TYPE_TIME, colSize, decDigits);

  if (Utility::IsError(
          SQLBindParameter(stmt_, (SQLUSMALLINT)pos + 1, ToOdbcDirection(dir),
                           SQL_C_TYPE_TIME, SQL_TYPE_TIME, colSize, decDigits,
                           (SQLPOINTER)pTS, 0, _lengthIndicator.back()))) {
    throw StatementException(stmt_, "SQLBindParameter(Time)");
  }
}

void Binder::Bind(size_t pos, const fun::DateTime& val, Direction dir,
                  const WhenNullCb& null_cb) {
  SQLLEN* pLenIn = new SQLLEN;
  *pLenIn = SQL_NTS;  // microsoft example does that, otherwise no null
                      // indicator is returned
  if (IsOutBound(dir) && null_cb.IsDefined())
    _nullCbMap.insert(NullCbMap::value_type(pLenIn, null_cb));

  _lengthIndicator.push_back(pLenIn);

  SQL_TIMESTAMP_STRUCT* pTS = new SQL_TIMESTAMP_STRUCT;
  Utility::DateTimeSync(*pTS, val);

  _timestamps.insert(
      TimestampMap::value_type(pTS, const_cast<DateTime*>(&val)));

  SQLINTEGER colSize = 0;
  SQLSMALLINT decDigits = 0;
  GetColSizeAndPrecision(pos, SQL_TYPE_TIMESTAMP, colSize, decDigits);

  if (Utility::IsError(SQLBindParameter(
          stmt_, (SQLUSMALLINT)pos + 1, ToOdbcDirection(dir),
          SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, colSize, decDigits,
          (SQLPOINTER)pTS, 0, _lengthIndicator.back()))) {
    throw StatementException(stmt_, "SQLBindParameter(DateTime)");
  }
}

void Binder::Bind(size_t pos, const NullData& val, Direction dir,
                  const std::type_info& bind_type) {
  if (IsOutBound(dir) || !IsInBound(dir))
    throw NotImplementedException("NULL parameter type can only be inbound.");

  in_params_.insert(ParamMap::value_type(SQLPOINTER(0), SQLINTEGER(0)));

  SQLLEN* pLenIn = new SQLLEN;
  *pLenIn = SQL_NULL_DATA;

  _lengthIndicator.push_back(pLenIn);

  SQLINTEGER colSize = 0;
  SQLSMALLINT decDigits = 0;

  const SQLSMALLINT colType =
      (bind_type == typeid(void) || bind_type == typeid(NullData) ||
       bind_type == typeid(NullType))
          ? _pTypeInfo->nullDataType(val)
          : _pTypeInfo->tryTypeidToCType(bind_type, SQL_C_TINYINT);

  GetColSizeAndPrecision(pos, colType, colSize, decDigits);

  if (Utility::IsError(SQLBindParameter(
          stmt_, (SQLUSMALLINT)pos + 1, SQL_PARAM_INPUT, colType,
          static_cast<SQLSMALLINT>(Utility::sqlDataType(colType)), colSize,
          decDigits, 0, 0, _lengthIndicator.back()))) {
    throw StatementException(stmt_, "SQLBindParameter()");
  }
}

size_t Binder::GetParameterSize(SQLPOINTER pAddr) const {
  ParamMap::const_iterator it = in_params_.find(pAddr);
  if (it != in_params_.end()) return it->second;

  it = out_params_.find(pAddr);
  if (it != out_params_.end()) return it->second;

  throw NotFoundException("Requested data size not found.");
}

void Binder::Bind(size_t pos, const char* const& /*pVal*/, Direction dir,
                  const WhenNullCb& null_cb) {
  throw NotImplementedException(
      "char* binding not implemented, Use String instead.");
}

SQLSMALLINT Binder::ToOdbcDirection(Direction dir) const {
  bool in = IsInBound(dir);
  bool out = IsOutBound(dir);
  SQLSMALLINT ioType = SQL_PARAM_TYPE_UNKNOWN;
  if (in && out)
    ioType = SQL_PARAM_INPUT_OUTPUT;
  else if (in)
    ioType = SQL_PARAM_INPUT;
  else if (out)
    ioType = SQL_PARAM_OUTPUT;
  else
    throw fun::IllegalStateException(
        "Binder not bound (must be [in] OR [out]).");

  return ioType;
}

void Binder::synchronize() {
  if (dates_.size()) {
    DateMap::iterator it = dates_.begin();
    DateMap::iterator end = dates_.end();
    for (; it != end; ++it) Utility::DateSync(*it->second, *it->first);
  }

  if (times_.size()) {
    TimeMap::iterator it = times_.begin();
    TimeMap::iterator end = times_.end();
    for (; it != end; ++it) Utility::TimeSync(*it->second, *it->first);
  }

  if (_timestamps.size()) {
    TimestampMap::iterator it = _timestamps.begin();
    TimestampMap::iterator end = _timestamps.end();
    for (; it != end; ++it) Utility::DateTimeSync(*it->second, *it->first);
  }

  if (_strings.size()) {
    StringMap::iterator it = _strings.begin();
    StringMap::iterator end = _strings.end();
    for (; it != end; ++it)
      it->second->Assign(it->first, std::strlen(it->first));
  }

  if (_utf16Strings.size()) {
    UTF16StringMap::iterator it = _utf16Strings.begin();
    UTF16StringMap::iterator end = _utf16Strings.end();
    for (; it != end; ++it)
      it->second->Assign(
          it->first,
          UTF16CharTraits::length((UTF16CharTraits::char_type*)it->first));
  }

  if (_nullCbMap.size()) {
    NullCbMap::iterator it = _nullCbMap.begin();
    NullCbMap::iterator end = _nullCbMap.end();
    for (; it != end; ++it)
      if (*it->first == SQL_NULL_DATA) it->second.onNull();
  }
}

void Binder::Reset() {
  FreeMemory();

  if (_lengthIndicator.size() > 0) LengthPtrVec().Swap(_lengthIndicator);
  if (in_params_.size() > 0) in_params_.clear();
  if (out_params_.size() > 0) out_params_.clear();
  if (dates_.size() > 0) dates_.clear();
  if (times_.size() > 0) times_.clear();
  if (_timestamps.size() > 0) _timestamps.clear();
  if (_strings.size() > 0) _strings.clear();
  if (_utf16Strings.size() > 0) _utf16Strings.clear();
  if (_dateVecVec.size() > 0) _dateVecVec.clear();
  if (_timeVecVec.size() > 0) _timeVecVec.clear();
  if (_dateTimeVecVec.size() > 0) _dateTimeVecVec.clear();
  if (_charPtrs.size() > 0) _charPtrs.clear();
  if (_boolPtrs.size() > 0) _boolPtrs.clear();
  if (containers_.size() > 0) containers_.clear();
  if (_nullCbMap.size() > 0) _nullCbMap.clear();
  param_set_size_ = 0;
  if (!insert_only_) parameters_.clear();
}

void Binder::GetColSizeAndPrecision(size_t pos, SQLSMALLINT cDataType,
                                    SQLINTEGER& colSize, SQLSMALLINT& decDigits,
                                    size_t actualSize) {
  colSize = 0;
  decDigits = 0;
  // Not all drivers are equally willing to cooperate in this matter.
  // Hence the funky flow control.

  if (_pTypeInfo) {
    DynamicAny tmp;
    bool found = _pTypeInfo->tryGetInfo(cDataType, "COLUMN_SIZE", tmp);
    if (found) colSize = tmp;
    if (colSize && actualSize > colSize) {
      throw LengthExceededException(
          fun::Format("Error binding column %z size=%z, max size=%ld)", pos,
                      actualSize, static_cast<long>(colSize)));
    }
    found = _pTypeInfo->tryGetInfo(cDataType, "MINIMUM_SCALE", tmp);
    if (found) {
      decDigits = tmp;
      return;
    }
  }

  if (parameters_.size() <= pos || !parameters_[pos].IsDefined()) {
    if (parameters_.size() <= pos) parameters_.resize(pos + 1);
    parameters_[pos] = ParamDescriptor(0, cDataType, 0);

    try {
      {
        Parameter p(stmt_, pos);
        parameters_[pos] =
            ParamDescriptor(static_cast<SQLINTEGER>(p.columnSize()), cDataType,
                            static_cast<SQLSMALLINT>(p.decimalDigits()));
      }
    } catch (StatementException&) {
      try {
        OdbcMetaColumn c(stmt_, pos);
        parameters_[pos] =
            ParamDescriptor(static_cast<SQLINTEGER>(c.length()), cDataType,
                            static_cast<SQLSMALLINT>(c.precision()));
      } catch (StatementException&) {
      }
    }
  }

  // we may have no success, so use zeros and hope for the best
  // (most drivers do not require these most of the times anyway)
  if (0 == colSize) {
    colSize = parameters_[pos].colSize;
    if (actualSize > colSize) {
      throw LengthExceededException(
          fun::Format("Error binding column %z size=%z, max size=%ld)", pos,
                      actualSize, static_cast<long>(colSize)));
    }
  }
  decDigits = parameters_[pos].decDigits;
}

void Binder::getColumnOrParameterSize(size_t pos, SQLINTEGER& size) {
  size_t colSize = 0;
  size_t paramSize = 0;

  try {
    OdbcMetaColumn col(stmt_, pos);
    colSize = col.length();
  } catch (StatementException&) {
  }

  try {
    Parameter p(stmt_, pos);
    paramSize = p.columnSize();
  } catch (StatementException&) {
    size = DEFAULT_PARAM_SIZE;
// On Linux, PostgreSQL driver segfaults on SQLGetDescField, so this is disabled
// for now
#ifdef FUN_PLATFORM_WINDOWS_FAMILY
    SQLHDESC hIPD = 0;
    if (!Utility::IsError(fun::sql::odbc::SQLGetStmtAttr(
            stmt_, SQL_ATTR_IMP_PARAM_DESC, &hIPD, SQL_IS_POINTER, 0))) {
      SQLUINTEGER sz = 0;
      if (!Utility::IsError(fun::sql::odbc::SQLGetDescField(
              hIPD, (SQLSMALLINT)pos + 1, SQL_DESC_LENGTH, &sz, SQL_IS_UINTEGER,
              0)) &&
          sz > 0) {
        size = sz;
      }
    }
#endif
  }

  if (colSize > 0 && paramSize > 0) {
    size = colSize < paramSize ? static_cast<SQLINTEGER>(colSize)
                               : static_cast<SQLINTEGER>(paramSize);
  } else if (colSize > 0) {
    size = static_cast<SQLINTEGER>(colSize);
  } else if (paramSize > 0) {
    size = static_cast<SQLINTEGER>(paramSize);
  }

  if (size > max_field_size_) {
    size = static_cast<SQLINTEGER>(max_field_size_);
  }
}

void Binder::SetParamSetSize(size_t length) {
  if (0 == param_set_size_) {
    if (Utility::IsError(fun::sql::odbc::SQLSetStmtAttr(
            stmt_, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN,
            SQL_IS_UINTEGER)) ||
        Utility::IsError(fun::sql::odbc::SQLSetStmtAttr(
            stmt_, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)length,
            SQL_IS_UINTEGER))) {
      throw StatementException(stmt_, "SQLSetStmtAttr()");
    }

    param_set_size_ = static_cast<SQLINTEGER>(length);
  }
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
