#include "fun/sql/odbc/extractor.h"
#include <typeinfo>
#include "fun/base/buffer.h"
#include "fun/base/exception.h"
#include "fun/sql/lob.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/odbc/odbc_meta_column.h"
#include "fun/sql/odbc/utility.h"

namespace fun {
namespace sql {
namespace odbc {

const String Extractor::FLD_SIZE_EXCEEDED_FMT =
    "Specified data size (%z bytes) "
    "exceeds maximum value (%z).\n"
    "Use Session.SetProperty(\"maxFieldSize\", value) "
    "to increase the maximum allowed data size\n";

Extractor::Extractor(const StatementHandle& stmt, Preparator::Ptr preparator)
    : stmt_(stmt),
      preparator_(preparator),
      data_extraction_(preparator->GetDataExtraction()) {}

Extractor::~Extractor() {}

template <>
bool Extractor::ExtractBoundImpl<String>(size_t pos, String& val) {
  if (IsNull(pos)) {
    return false;
  }

  size_t dataSize = preparator_->actualDataSize(pos);
  char* sp = AnyCast<char*>(preparator_->at(pos));
  size_t len = std::strlen(sp);
  if (len < dataSize) dataSize = len;
  CheckDataSize(dataSize);
  val.Assign(sp, dataSize);

  return true;
}

template <>
bool Extractor::ExtractBoundImpl<UString>(size_t pos, UString& val) {
  typedef UString::value_type CharT;
  if (IsNull(pos)) {
    return false;
  }

  size_t dataSize = preparator_->actualDataSize(pos);
  CharT* sp = 0;
  UString us;
  const std::type_info& ti = preparator_->at(pos).type();
  if (ti == typeid(CharT*)) {
    sp = AnyCast<CharT*>(preparator_->at(pos));
  } else if (ti == typeid(char*)) {
    String s(AnyCast<char*>(preparator_->at(pos)));
    fun::UnicodeConverter::convert(s, us);
    sp = const_cast<CharT*>(us.c_str());
  } else {
    throw fun::sql::ExtractException("Unsupported string type: " +
                                     String(ti.name()));
  }
  size_t len = fun::UnicodeConverter::UTFStrlen(sp);
  if (len < dataSize) dataSize = len;
  CheckDataSize(dataSize);
  val.Assign(sp, dataSize);

  return true;
}

template <>
bool Extractor::ExtractBoundImpl<fun::sql::Date>(size_t pos,
                                                 fun::sql::Date& val) {
  if (IsNull(pos)) {
    return false;
  }

  SQL_DATE_STRUCT& ds = *AnyCast<SQL_DATE_STRUCT>(&(preparator_->at(pos)));
  Utility::DateSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::vector<fun::sql::Date> >(
    size_t pos, std::vector<fun::sql::Date>& val) {
  std::vector<SQL_DATE_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_DATE_STRUCT> >(preparator_->at(pos));
  Utility::DateSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::deque<fun::sql::Date> >(
    size_t pos, std::deque<fun::sql::Date>& val) {
  std::vector<SQL_DATE_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_DATE_STRUCT> >(preparator_->at(pos));
  Utility::DateSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::list<fun::sql::Date> >(
    size_t pos, std::list<fun::sql::Date>& val) {
  std::vector<SQL_DATE_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_DATE_STRUCT> >(preparator_->at(pos));
  Utility::DateSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImpl<fun::sql::Time>(size_t pos,
                                                 fun::sql::Time& val) {
  if (IsNull(pos)) {
    return false;
  }

  size_t dataSize = preparator_->actualDataSize(pos);
  CheckDataSize(dataSize);
  SQL_TIME_STRUCT& ts = *AnyCast<SQL_TIME_STRUCT>(&preparator_->at(pos));
  Utility::TimeSync(val, ts);

  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::vector<fun::sql::Time> >(
    size_t pos, std::vector<fun::sql::Time>& val) {
  std::vector<SQL_TIME_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_TIME_STRUCT> >(preparator_->at(pos));
  Utility::TimeSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::deque<fun::sql::Time> >(
    size_t pos, std::deque<fun::sql::Time>& val) {
  std::vector<SQL_TIME_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_TIME_STRUCT> >(preparator_->at(pos));
  Utility::TimeSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::list<fun::sql::Time> >(
    size_t pos, std::list<fun::sql::Time>& val) {
  std::vector<SQL_TIME_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_TIME_STRUCT> >(preparator_->at(pos));
  Utility::TimeSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImpl<fun::DateTime>(size_t pos,
                                                fun::DateTime& val) {
  if (IsNull(pos)) {
    return false;
  }

  size_t dataSize = preparator_->actualDataSize(pos);
  CheckDataSize(dataSize);
  SQL_TIMESTAMP_STRUCT& tss =
      *AnyCast<SQL_TIMESTAMP_STRUCT>(&preparator_->at(pos));
  Utility::DateTimeSync(val, tss);

  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::vector<fun::DateTime> >(
    size_t pos, std::vector<fun::DateTime>& val) {
  std::vector<SQL_TIMESTAMP_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_TIMESTAMP_STRUCT> >(preparator_->at(pos));
  Utility::DateTimeSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::deque<fun::DateTime> >(
    size_t pos, std::deque<fun::DateTime>& val) {
  std::vector<SQL_TIMESTAMP_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_TIMESTAMP_STRUCT> >(preparator_->at(pos));
  Utility::DateTimeSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::list<fun::DateTime> >(
    size_t pos, std::list<fun::DateTime>& val) {
  std::vector<SQL_TIMESTAMP_STRUCT>& ds =
      RefAnyCast<std::vector<SQL_TIMESTAMP_STRUCT> >(preparator_->at(pos));
  Utility::DateTimeSync(val, ds);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::vector<bool> >(
    size_t pos, std::vector<bool>& val) {
  size_t length = preparator_->GetLength();
  bool** p = AnyCast<bool*>(&preparator_->at(pos));
  val.Assign(*p, *p + length);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::deque<bool> >(
    size_t pos, std::deque<bool>& val) {
  size_t length = preparator_->GetLength();
  bool** p = AnyCast<bool*>(&preparator_->at(pos));
  val.Assign(*p, *p + length);
  return true;
}

template <>
bool Extractor::ExtractBoundImplContainer<std::list<bool> >(
    size_t pos, std::list<bool>& val) {
  size_t length = preparator_->GetLength();
  bool** p = AnyCast<bool*>(&preparator_->at(pos));
  val.Assign(*p, *p + length);
  return true;
}

template <typename T>
bool Extractor::extractManualStringImpl(size_t pos, T& val, SQLSMALLINT cType) {
  typedef typename T::value_type CharType;
  const size_t bytesPerChar = sizeof(CharType);
  const size_t charsPerChunk = CHUNK_SIZE;
  const size_t bytesPerChunk = charsPerChunk * bytesPerChar;
  SQLLEN totalBytes;
  SQLRETURN rc;
  CharType tmpBuf[charsPerChunk + 1];  // space for NUL terminator

  val.clear();
  ResizeLengths(pos);

  rc = SQLGetData(
      stmt_, (SQLUSMALLINT)pos + 1,
      cType,   // C data type
      tmpBuf,  // returned value
      bytesPerChunk +
          bytesPerChar,  // buffer length in bytes, including NUL terminator
      &totalBytes);      // size in bytes of this field, not including NULL
                         // terminator

  if (SQL_NO_DATA != rc && Utility::IsError(rc)) {
    throw StatementException(stmt_, "SQLGetData()");
  }

  if (SQL_NO_TOTAL == totalBytes) {  // unknown length, throw
    throw UnknownDataLengthException(
        "Could not determine returned data length.");
  }

  if (IsNullLengthIndicator(totalBytes)) {
    lengths_[pos] = totalBytes;
    return false;
  }

  if (SQL_NO_DATA == rc || !totalBytes) {
    return true;
  }

  lengths_[pos] = totalBytes;
  const size_t totalChars = totalBytes / bytesPerChar;

  if (totalBytes <= bytesPerChunk) {
    // There is no additional data; we've already got it all
    val.append(tmpBuf, totalChars);
  } else {
    // Reserve space in result string, append what we already have, resize and
    // insert the remaining data in-place.
    const size_t bytesRemaining = totalBytes - bytesPerChunk;
    SQLLEN fetchedBytes = 0;
    val.reserve(totalChars);
    val.append(tmpBuf, charsPerChunk);
    val.resize(totalChars);
    rc = SQLGetData(
        stmt_, (SQLUSMALLINT)pos + 1,
        cType,                // C data type
        &val[charsPerChunk],  // buffer to write to
        bytesRemaining +
            bytesPerChar,  // buffer length in bytes, including NUL terminator
        &fetchedBytes);    // number of bytes remaining, not including NULL
                           // terminator

    if (SQL_NO_DATA != rc && Utility::IsError(rc)) {
      throw StatementException(stmt_, "SQLGetData()");
    }

    if (SQL_NO_TOTAL == fetchedBytes) {  // unknown length, throw
      throw UnknownDataLengthException(
          "Could not determine returned data length.");
    }

    if (bytesRemaining != fetchedBytes) {  // unexpected number of bytes
      throw UnknownDataLengthException(
          "Unexpected number of bytes returned from second call to "
          "SQLGetData().");
    }
  }

  return true;
}

template <>
bool Extractor::extractManualImpl<String>(size_t pos, String& val,
                                          SQLSMALLINT cType) {
  return extractManualStringImpl(pos, val, cType);
}

template <>
bool Extractor::extractManualImpl<UString>(size_t pos, UString& val,
                                           SQLSMALLINT cType) {
  return extractManualStringImpl(pos, val, cType);
}

template <>
bool Extractor::extractManualImpl<fun::sql::CLOB>(size_t pos,
                                                  fun::sql::CLOB& val,
                                                  SQLSMALLINT cType) {
  return extractManualLOBImpl(pos, val, cType);
}

template <>
bool Extractor::extractManualImpl<fun::sql::BLOB>(size_t pos,
                                                  fun::sql::BLOB& val,
                                                  SQLSMALLINT cType) {
  return extractManualLOBImpl(pos, val, cType);
}

template <typename T>
bool Extractor::extractManualLOBImpl(size_t pos, fun::sql::LOB<T>& val,
                                     SQLSMALLINT cType) {
  const int bufSize = CHUNK_SIZE;
  size_t fetchedSize = bufSize;
  size_t totalSize = 0;

  SQLLEN len;

  fun::Buffer<T> apChar(bufSize);
  T* pChar = apChar.begin();
  SQLRETURN rc = 0;

  val.clear();
  ResizeLengths(pos);

  do {
    // clear out the latest data in the buffer
    if (fetchedSize > 0) UnsafeMemory::Memset(pChar, 0, fetchedSize);
    len = 0;
    rc = SQLGetData(stmt_, (SQLUSMALLINT)pos + 1,
                    cType,    // C data type
                    pChar,    // returned value
                    bufSize,  // buffer length
                    &len);    // length indicator

    lengths_[pos] += len;

    if (SQL_NO_DATA != rc && Utility::IsError(rc)) {
      throw StatementException(stmt_, "SQLGetData()");
    }

    if (SQL_NO_TOTAL == len) {  // unknown length, throw
      throw UnknownDataLengthException(
          "Could not determine returned data length.");
    }

    if (IsNullLengthIndicator(len)) {
      return false;
    }

    if (SQL_NO_DATA == rc || !len) {
      break;
    }

    fetchedSize = len > bufSize ? bufSize : len;
    totalSize += fetchedSize;
    val.AppendRaw(pChar, fetchedSize);
  } while (true);

  return true;
}

template <>
bool Extractor::extractManualImpl<fun::sql::Date>(size_t pos,
                                                  fun::sql::Date& val,
                                                  SQLSMALLINT cType) {
  SQL_DATE_STRUCT ds;
  ResizeLengths(pos);

  SQLRETURN rc = SQLGetData(stmt_, (SQLUSMALLINT)pos + 1,
                            cType,            // C data type
                            &ds,              // returned value
                            sizeof(ds),       // buffer length
                            &lengths_[pos]);  // length indicator

  if (Utility::IsError(rc)) {
    throw StatementException(stmt_, "SQLGetData()");
  }

  if (IsNullLengthIndicator(lengths_[pos])) {
    return false;
  } else {
    Utility::DateSync(val, ds);
  }

  return true;
}

template <>
bool Extractor::extractManualImpl<fun::sql::Time>(size_t pos,
                                                  fun::sql::Time& val,
                                                  SQLSMALLINT cType) {
  SQL_TIME_STRUCT ts;
  ResizeLengths(pos);

  SQLRETURN rc = SQLGetData(stmt_, (SQLUSMALLINT)pos + 1,
                            cType,            // C data type
                            &ts,              // returned value
                            sizeof(ts),       // buffer length
                            &lengths_[pos]);  // length indicator

  if (Utility::IsError(rc)) {
    throw StatementException(stmt_, "SQLGetData()");
  }

  if (IsNullLengthIndicator(lengths_[pos])) {
    return false;
  } else {
    Utility::TimeSync(val, ts);
  }

  return true;
}

template <>
bool Extractor::extractManualImpl<fun::DateTime>(size_t pos, fun::DateTime& val,
                                                 SQLSMALLINT cType) {
  SQL_TIMESTAMP_STRUCT ts;
  ResizeLengths(pos);

  SQLRETURN rc = SQLGetData(stmt_, (SQLUSMALLINT)pos + 1,
                            cType,            // C data type
                            &ts,              // returned value
                            sizeof(ts),       // buffer length
                            &lengths_[pos]);  // length indicator

  if (Utility::IsError(rc)) {
    throw StatementException(stmt_, "SQLGetData()");
  }

  if (IsNullLengthIndicator(lengths_[pos])) {
    return false;
  } else {
    Utility::DateTimeSync(val, ts);
  }

  return true;
}

bool Extractor::Extract(size_t pos, int32& val) {
  if (Preparator::DE_MANUAL == data_extraction_) {
    return extractManualImpl(pos, val, SQL_C_SLONG);
  } else {
    return ExtractBoundImpl(pos, val);
  }
}

bool Extractor::Extract(size_t pos, std::vector<int32>& val) {
  if (Preparator::DE_BOUND == data_extraction_) {
    return ExtractBoundImplContainer(pos, val);
  } else {
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
  }
}

bool Extractor::Extract(size_t pos, std::deque<int32>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<int32>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, int64& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_SBIGINT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<int64>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<int64>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<int64>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

#ifndef FUN_LONG_IS_64_BIT
bool Extractor::Extract(size_t pos, long& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_SLONG);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, unsigned long& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_SLONG);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<long>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<long>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<long>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}
#endif

bool Extractor::Extract(size_t pos, double& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_DOUBLE);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<double>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<double>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<double>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, String& val) {
  if (Preparator::DE_MANUAL == data_extraction_ ||
      preparator_->IsPotentiallyHuge(pos))
    return extractManualImpl(pos, val, SQL_C_CHAR);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<String>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<String>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<String>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, UString& val) {
  if (Preparator::DE_MANUAL == data_extraction_ ||
      preparator_->IsPotentiallyHuge(pos))
    return extractManualImpl(pos, val, SQL_C_WCHAR);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<UString>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<UString>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<UString>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, fun::sql::BLOB& val) {
  if (Preparator::DE_MANUAL == data_extraction_ ||
      preparator_->IsPotentiallyHuge(pos))
    return extractManualImpl(pos, val, SQL_C_BINARY);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, fun::sql::CLOB& val) {
  if (Preparator::DE_MANUAL == data_extraction_ ||
      preparator_->IsPotentiallyHuge(pos))
    return extractManualImpl(pos, val, SQL_C_BINARY);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<fun::sql::BLOB>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<fun::sql::BLOB>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<fun::sql::BLOB>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::vector<fun::sql::CLOB>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<fun::sql::CLOB>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<fun::sql::CLOB>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, fun::sql::Date& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_TYPE_DATE);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<fun::sql::Date>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<fun::sql::Date>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<fun::sql::Date>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, fun::sql::Time& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_TYPE_TIME);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<fun::sql::Time>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<fun::sql::Time>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<fun::sql::Time>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, fun::DateTime& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_TYPE_TIMESTAMP);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<fun::DateTime>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<fun::DateTime>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<fun::DateTime>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, int8& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_STINYINT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<int8>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<int8>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<int8>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, uint8& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_UTINYINT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<uint8>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<uint8>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<uint8>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, int16& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_SSHORT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<int16>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<int16>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<int16>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, uint16& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_USHORT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<uint16>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<uint16>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<uint16>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, uint32& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_ULONG);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<uint32>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<uint32>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<uint32>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, uint64& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_SBIGINT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<uint64>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<uint64>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<uint64>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, bool& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_BIT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<bool>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<bool>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<bool>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, float& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_FLOAT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<float>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<float>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<float>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, char& val) {
  if (Preparator::DE_MANUAL == data_extraction_)
    return extractManualImpl(pos, val, SQL_C_STINYINT);
  else
    return ExtractBoundImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<char>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<char>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<char>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImplContainer(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, fun::Any& val) {
  return ExtractImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<fun::Any>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImpl(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<fun::Any>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImpl(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<fun::Any>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImpl(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, fun::dynamicAny& val) {
  return ExtractImpl(pos, val);
}

bool Extractor::Extract(size_t pos, std::vector<fun::dynamicAny>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImpl(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::deque<fun::dynamicAny>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImpl(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::Extract(size_t pos, std::list<fun::dynamicAny>& val) {
  if (Preparator::DE_BOUND == data_extraction_)
    return ExtractBoundImpl(pos, val);
  else
    throw InvalidAccessException(
        "Direct container extraction only allowed for bound mode.");
}

bool Extractor::IsNull(size_t col, size_t row) {
  if (Preparator::DE_MANUAL == data_extraction_) {
    try {
      return IsNullLengthIndicator(lengths_.at(col));
    } catch (std::out_of_range& ex) {
      throw RangeException(ex.what());
    }
  } else {
    return SQL_NULL_DATA == preparator_->actualDataSize(col, row);
  }
}

void Extractor::CheckDataSize(size_t size) {
  size_t maxSize = preparator_->GetMaxFieldSize();
  if (size > maxSize) {
    throw SqlException(format(FLD_SIZE_EXCEEDED_FMT, size, maxSize));
  }
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
