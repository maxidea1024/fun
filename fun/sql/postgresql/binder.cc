#include "fun/sql/postgresql/binder.h"
#include "fun/base/number_formatter.h"
#include "fun/base/date_time_format.h"

namespace fun {
namespace sql {
namespace postgresql {

Binder::Binder() {}

Binder::~Binder() {}

void Binder::Bind(size_t pos, const int8& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_INT8, & val, sizeof(int8));
}

void Binder::Bind(size_t pos, const uint8& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_UINT8, & val, sizeof(uint8));
}

void Binder::Bind(size_t pos, const int16& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_INT16, & val, sizeof(int16));
}

void Binder::Bind(size_t pos, const uint16& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_UINT16, & val, sizeof(uint16));
}

void Binder::Bind(size_t pos, const int32& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_INT32, & val, sizeof(int32));
}

void Binder::Bind(size_t pos, const uint32& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_UINT32, & val, sizeof(uint32));
}

void Binder::Bind(size_t pos, const int64& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_INT64, & val, sizeof(int64));
}

void Binder::Bind(size_t pos, const uint64& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_UINT64, & val, sizeof(uint64));
}

#ifndef FUN_LONG_IS_64_BIT

void Binder::Bind(size_t pos, const long& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_INT64, & val, sizeof(int64));
}

void Binder::Bind(size_t pos, const unsigned long& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_UINT64, & val, sizeof(uint64));
}

#endif // FUN_LONG_IS_64_BIT

void Binder::Bind(size_t pos, const bool& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_BOOL, & val, sizeof(bool));
}

void Binder::Bind(size_t pos, const float& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_FLOAT, & val, sizeof(float));
}

void Binder::Bind(size_t pos, const double& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_DOUBLE, & val, sizeof(double));
}

void Binder::Bind(size_t pos, const char& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  // USING UINT8 because fun::sql::MetaColumn does not have a single character type, just String
  RealBind(pos, fun::sql::MetaColumn::FDT_UINT8, & val, sizeof(char));
}

// complex types

void Binder::Bind(size_t pos, const String& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_STRING, & val, static_cast<int>(val.size()));
}

void Binder::Bind(size_t pos, const fun::sql::BLOB& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_BLOB, & val, static_cast<int>(val.size()));
}

void Binder::Bind(size_t pos, const fun::sql::CLOB& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_CLOB, & val, static_cast<int>(val.size()));
}

void Binder::Bind(size_t pos, const fun::DateTime& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_TIMESTAMP, & val, sizeof(fun::DateTime));
}

void Binder::Bind(size_t pos, const Date& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_DATE, & val, sizeof(Date));
}

void Binder::Bind(size_t pos, const Time& val, Direction dir, const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_TIME, & val, sizeof(Time));
}

void Binder::Bind(size_t pos, const NullData&, Direction dir, const std::type_info&) {
  fun_check(dir == PD_IN);
  RealBind(pos, fun::sql::MetaColumn::FDT_UNKNOWN, 0, 0);
}

size_t Binder::Count() const {
  return static_cast<size_t>(bind_vector_.size());
}

InputParameterVector Binder::BindVector() const {
  return bind_vector_;
}

void Binder::UpdateBindVectorToCurrentValues() {
  InputParameterVector::iterator itr  = bind_vector_.begin();
  InputParameterVector::iterator itrEnd = bind_vector_.end();

  for (; itr != itrEnd; ++itr) {
  switch (itr->fieldType()) {
    case fun::sql::MetaColumn::FDT_INT8:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const int8*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_UINT8:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const uint8*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_INT16:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const int16*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_UINT16:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const uint16*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_INT32:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const int32*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_UINT32:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const uint32*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_INT64:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const int64*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_UINT64:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const uint64*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_BOOL: {
      const bool currentBoolValue = * static_cast<const bool*>(itr->pData());
      itr->SetStringVersionRepresentation(currentBoolValue ? "TRUE" : "FALSE");
      }
      break;

    case fun::sql::MetaColumn::FDT_FLOAT:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const float*>(itr->pData())));
      break;

    case fun::sql::MetaColumn::FDT_DOUBLE:
      itr->SetStringVersionRepresentation(fun::NumberFormatter::Format(* static_cast<const double*>(itr->pData())));
      break;

  //  case fun::sql::MetaColumn::FDT_CHAR:
  //    itr->SetStringVersionRepresentation(String(static_cast<const char*>(itr->pData()), 1));  // single character string
  //    break;

    case fun::sql::MetaColumn::FDT_STRING:
      itr->SetStringVersionRepresentation(* static_cast<const String*>(itr->pData()));
      break;

    case fun::sql::MetaColumn::FDT_TIMESTAMP: {
      const fun::DateTime& datetime = * static_cast<const fun::DateTime*>(itr->pData());
      itr->SetStringVersionRepresentation(DateTimeFormatter::Format(datetime, fun::DateTimeFormat::ISO8601_FRAC_FORMAT));
      }
      break;

    case fun::sql::MetaColumn::FDT_DATE: {
      const fun::sql::Date& date = * static_cast<const fun::sql::Date*>(itr->pData());
      itr->SetStringVersionRepresentation(DateTimeFormatter::Format(fun::DateTime(date.Year(), date.Month(), date.Day()), "%Y-%m-%d"));
      }
      break;

    case fun::sql::MetaColumn::FDT_TIME: {
      const fun::sql::Time& time = * static_cast<const fun::sql::Time*>(itr->pData());
      itr->SetStringVersionRepresentation(DateTimeFormatter::Format(fun::DateTime(0, 1, 1, time.Hour(), time.Minute(), time.Second()), "%H:%M:%s%z"));
      }
      break;

    case fun::sql::MetaColumn::FDT_BLOB: {
      const fun::sql::BLOB& blob = * static_cast<const fun::sql::BLOB*>(itr->pData());
      itr->SetNonStringVersionRepresentation(static_cast<const void*> (blob.GetRawContent()), blob.size());
      }
      break;

    case fun::sql::MetaColumn::FDT_CLOB: {
      const fun::sql::CLOB& clob = * static_cast<const fun::sql::CLOB*>(itr->pData());
      itr->SetNonStringVersionRepresentation(static_cast<const void*> (clob.GetRawContent()), clob.size());
      }

    case fun::sql::MetaColumn::FDT_UNKNOWN:
    default:
      break;
    }
  }
}


//
// Private
//

void Binder::RealBind(size_t position, fun::sql::MetaColumn::ColumnDataType field_type, const void* buffer, size_t length) {
  try {
    if (position >= bind_vector_.size()) {
      bind_vector_.resize(position + 1);
    }

    InputParameter input_param(field_type, buffer, length);

    bind_vector_[position] = input_param;
  } catch (std::bad_alloc&) {
    PostgreSqlException("Memory allocation error while binding");
  }
}

void Binder::Bind(size_t pos, const std::vector<int8>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<int8>::iterator first = const_cast<std::vector<int8> &>(val).begin();
  std::vector<int8>::iterator last = const_cast<std::vector<int8> &>(val).end();
  RealContainerBind<std::vector<int8>::iterator, int8>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<int8>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<int8>::iterator first = const_cast<std::deque<int8> &>(val).begin();
  std::deque<int8>::iterator last = const_cast<std::deque<int8> &>(val).end();
  RealContainerBind<std::deque<int8>::iterator, int8>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<int8>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<int8>::iterator first = const_cast<std::list<int8> &>(val).begin();
  std::list<int8>::iterator last = const_cast<std::list<int8> &>(val).end();
  RealContainerBind<std::list<int8>::iterator, int8>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<uint8>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<uint8>::iterator first = const_cast<std::vector<uint8> &>(val).begin();
  std::vector<uint8>::iterator last = const_cast<std::vector<uint8> &>(val).end();
  RealContainerBind<std::vector<uint8>::iterator, uint8>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<uint8>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<uint8>::iterator first = const_cast<std::deque<uint8> &>(val).begin();
  std::deque<uint8>::iterator last = const_cast<std::deque<uint8> &>(val).end();
  RealContainerBind<std::deque<uint8>::iterator, uint8>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<uint8>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<uint8>::iterator first = const_cast<std::list<uint8> &>(val).begin();
  std::list<uint8>::iterator last = const_cast<std::list<uint8> &>(val).end();
  RealContainerBind<std::list<uint8>::iterator, uint8>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<int16>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<int16>::iterator first = const_cast<std::vector<int16> &>(val).begin();
  std::vector<int16>::iterator last = const_cast<std::vector<int16> &>(val).end();
  RealContainerBind<std::vector<int16>::iterator, int16>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<int16>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<int16>::iterator first = const_cast<std::deque<int16> &>(val).begin();
  std::deque<int16>::iterator last = const_cast<std::deque<int16> &>(val).end();
  RealContainerBind<std::deque<int16>::iterator, int16>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<int16>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<int16>::iterator first = const_cast<std::list<int16> &>(val).begin();
  std::list<int16>::iterator last = const_cast<std::list<int16> &>(val).end();
  RealContainerBind<std::list<int16>::iterator, int16>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<uint16>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<uint16>::iterator first = const_cast<std::vector<uint16> &>(val).begin();
  std::vector<uint16>::iterator last = const_cast<std::vector<uint16> &>(val).end();
  RealContainerBind<std::vector<uint16>::iterator, uint16>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<uint16>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<uint16>::iterator first = const_cast<std::deque<uint16> &>(val).begin();
  std::deque<uint16>::iterator last = const_cast<std::deque<uint16> &>(val).end();
  RealContainerBind<std::deque<uint16>::iterator, uint16>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<uint16>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<uint16>::iterator first = const_cast<std::list<uint16> &>(val).begin();
  std::list<uint16>::iterator last = const_cast<std::list<uint16> &>(val).end();
  RealContainerBind<std::list<uint16>::iterator, uint16>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<int32>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<int32>::iterator first = const_cast<std::vector<int32> &>(val).begin();
  std::vector<int32>::iterator last = const_cast<std::vector<int32> &>(val).end();
  RealContainerBind<std::vector<int32>::iterator, int32>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<int32>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<int32>::iterator first = const_cast<std::deque<int32> &>(val).begin();
  std::deque<int32>::iterator last = const_cast<std::deque<int32> &>(val).end();
  RealContainerBind<std::deque<int32>::iterator, int32>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<int32>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<int32>::iterator first = const_cast<std::list<int32> &>(val).begin();
  std::list<int32>::iterator last = const_cast<std::list<int32> &>(val).end();
  RealContainerBind<std::list<int32>::iterator, int32>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<uint32>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<uint32>::iterator first = const_cast<std::vector<uint32> &>(val).begin();
  std::vector<uint32>::iterator last = const_cast<std::vector<uint32> &>(val).end();
  RealContainerBind<std::vector<uint32>::iterator, uint32>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<uint32>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<uint32>::iterator first = const_cast<std::deque<uint32> &>(val).begin();
  std::deque<uint32>::iterator last = const_cast<std::deque<uint32> &>(val).end();
  RealContainerBind<std::deque<uint32>::iterator, uint32>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<uint32>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<uint32>::iterator first = const_cast<std::list<uint32> &>(val).begin();
  std::list<uint32>::iterator last = const_cast<std::list<uint32> &>(val).end();
  RealContainerBind<std::list<uint32>::iterator, uint32>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<int64>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<int64>::iterator first = const_cast<std::vector<int64> &>(val).begin();
  std::vector<int64>::iterator last = const_cast<std::vector<int64> &>(val).end();
  RealContainerBind<std::vector<int64>::iterator, int64>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<int64>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<int64>::iterator first = const_cast<std::deque<int64> &>(val).begin();
  std::deque<int64>::iterator last = const_cast<std::deque<int64> &>(val).end();
  RealContainerBind<std::deque<int64>::iterator, int64>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<int64>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<int64>::iterator first = const_cast<std::list<int64> &>(val).begin();
  std::list<int64>::iterator last = const_cast<std::list<int64> &>(val).end();
  RealContainerBind<std::list<int64>::iterator, int64>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<uint64>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<uint64>::iterator first = const_cast<std::vector<uint64> &>(val).begin();
  std::vector<uint64>::iterator last = const_cast<std::vector<uint64> &>(val).end();
  RealContainerBind<std::vector<uint64>::iterator, uint64>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<uint64>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<uint64>::iterator first = const_cast<std::deque<uint64> &>(val).begin();
  std::deque<uint64>::iterator last = const_cast<std::deque<uint64> &>(val).end();
  RealContainerBind<std::deque<uint64>::iterator, uint64>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<uint64>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<uint64>::iterator first = const_cast<std::list<uint64> &>(val).begin();
  std::list<uint64>::iterator last = const_cast<std::list<uint64> &>(val).end();
  RealContainerBind<std::list<uint64>::iterator, uint64>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<bool>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<bool>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<bool>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<float>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<float>::iterator first = const_cast<std::vector<float> &>(val).begin();
  std::vector<float>::iterator last = const_cast<std::vector<float> &>(val).end();
  RealContainerBind<std::vector<float>::iterator, float>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<float>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<float>::iterator first = const_cast<std::deque<float> &>(val).begin();
  std::deque<float>::iterator last = const_cast<std::deque<float> &>(val).end();
  RealContainerBind<std::deque<float>::iterator, float>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<float>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<float>::iterator first = const_cast<std::list<float> &>(val).begin();
  std::list<float>::iterator last = const_cast<std::list<float> &>(val).end();
  RealContainerBind<std::list<float>::iterator, float>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<double>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<double>::iterator first = const_cast<std::vector<double> &>(val).begin();
  std::vector<double>::iterator last = const_cast<std::vector<double> &>(val).end();
  RealContainerBind<std::vector<double>::iterator, double>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<double>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<double>::iterator first = const_cast<std::deque<double> &>(val).begin();
  std::deque<double>::iterator last = const_cast<std::deque<double> &>(val).end();
  RealContainerBind<std::deque<double>::iterator, double>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<double>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<double>::iterator first = const_cast<std::list<double> &>(val).begin();
  std::list<double>::iterator last = const_cast<std::list<double> &>(val).end();
  RealContainerBind<std::list<double>::iterator, double>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<char>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<char>::iterator first = const_cast<std::vector<char> &>(val).begin();
  std::vector<char>::iterator last = const_cast<std::vector<char> &>(val).end();
  RealContainerBind<std::vector<char>::iterator, char>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<char>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<char>::iterator first = const_cast<std::deque<char> &>(val).begin();
  std::deque<char>::iterator last = const_cast<std::deque<char> &>(val).end();
  RealContainerBind<std::deque<char>::iterator, char>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<char>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<char>::iterator first = const_cast<std::list<char> &>(val).begin();
  std::list<char>::iterator last = const_cast<std::list<char> &>(val).end();
  RealContainerBind<std::list<char>::iterator, char>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::BLOB>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<fun::sql::BLOB>::iterator first = const_cast<std::vector<fun::sql::BLOB> &>(val).begin();
  std::vector<fun::sql::BLOB>::iterator last = const_cast<std::vector<fun::sql::BLOB> &>(val).end();
  RealContainerBind<std::vector<fun::sql::BLOB>::iterator, fun::sql::BLOB>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::BLOB>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<fun::sql::BLOB>::iterator first = const_cast<std::deque<fun::sql::BLOB> &>(val).begin();
  std::deque<fun::sql::BLOB>::iterator last = const_cast<std::deque<fun::sql::BLOB> &>(val).end();
  RealContainerBind<std::deque<fun::sql::BLOB>::iterator, fun::sql::BLOB>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<fun::sql::BLOB>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<fun::sql::BLOB>::iterator first = const_cast<std::list<fun::sql::BLOB> &>(val).begin();
  std::list<fun::sql::BLOB>::iterator last = const_cast<std::list<fun::sql::BLOB> &>(val).end();
  RealContainerBind<std::list<fun::sql::BLOB>::iterator, fun::sql::BLOB>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::CLOB>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<fun::sql::CLOB>::iterator first = const_cast<std::vector<fun::sql::CLOB> &>(val).begin();
  std::vector<fun::sql::CLOB>::iterator last = const_cast<std::vector<fun::sql::CLOB> &>(val).end();
  RealContainerBind<std::vector<fun::sql::CLOB>::iterator, fun::sql::CLOB>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::CLOB>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<fun::sql::CLOB>::iterator first = const_cast<std::deque<fun::sql::CLOB> &>(val).begin();
  std::deque<fun::sql::CLOB>::iterator last = const_cast<std::deque<fun::sql::CLOB> &>(val).end();
  RealContainerBind<std::deque<fun::sql::CLOB>::iterator, fun::sql::CLOB>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<fun::sql::CLOB>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<fun::sql::CLOB>::iterator first = const_cast<std::list<fun::sql::CLOB> &>(val).begin();
  std::list<fun::sql::CLOB>::iterator last = const_cast<std::list<fun::sql::CLOB> &>(val).end();
  RealContainerBind<std::list<fun::sql::CLOB>::iterator, fun::sql::CLOB>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<fun::DateTime>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<fun::DateTime>::iterator first = const_cast<std::vector<fun::DateTime> &>(val).begin();
  std::vector<fun::DateTime>::iterator last = const_cast<std::vector<fun::DateTime> &>(val).end();
  RealContainerBind<std::vector<fun::DateTime>::iterator, fun::DateTime>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<fun::DateTime>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<fun::DateTime>::iterator first = const_cast<std::deque<fun::DateTime> &>(val).begin();
  std::deque<fun::DateTime>::iterator last = const_cast<std::deque<fun::DateTime> &>(val).end();
  RealContainerBind<std::deque<fun::DateTime>::iterator, fun::DateTime>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<fun::DateTime>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<fun::DateTime>::iterator first = const_cast<std::list<fun::DateTime> &>(val).begin();
  std::list<fun::DateTime>::iterator last = const_cast<std::list<fun::DateTime> &>(val).end();
  RealContainerBind<std::list<fun::DateTime>::iterator, fun::DateTime>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::Date>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<fun::sql::Date>::iterator first = const_cast<std::vector<fun::sql::Date> &>(val).begin();
  std::vector<fun::sql::Date>::iterator last = const_cast<std::vector<fun::sql::Date> &>(val).end();
  RealContainerBind<std::vector<fun::sql::Date>::iterator, fun::sql::Date>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::Date>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<fun::sql::Date>::iterator first = const_cast<std::deque<fun::sql::Date> &>(val).begin();
  std::deque<fun::sql::Date>::iterator last = const_cast<std::deque<fun::sql::Date> &>(val).end();
  RealContainerBind<std::deque<fun::sql::Date>::iterator, fun::sql::Date>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<fun::sql::Date>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<fun::sql::Date>::iterator first = const_cast<std::list<fun::sql::Date> &>(val).begin();
  std::list<fun::sql::Date>::iterator last = const_cast<std::list<fun::sql::Date> &>(val).end();
  RealContainerBind<std::list<fun::sql::Date>::iterator, fun::sql::Date>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::Time>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<fun::sql::Time>::iterator first = const_cast<std::vector<fun::sql::Time> &>(val).begin();
  std::vector<fun::sql::Time>::iterator last = const_cast<std::vector<fun::sql::Time> &>(val).end();
  RealContainerBind<std::vector<fun::sql::Time>::iterator, fun::sql::Time>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::Time>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<fun::sql::Time>::iterator first = const_cast<std::deque<fun::sql::Time> &>(val).begin();
  std::deque<fun::sql::Time>::iterator last = const_cast<std::deque<fun::sql::Time> &>(val).end();
  RealContainerBind<std::deque<fun::sql::Time>::iterator, fun::sql::Time>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<fun::sql::Time>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<fun::sql::Time>::iterator first = const_cast<std::list<fun::sql::Time> &>(val).begin();
  std::list<fun::sql::Time>::iterator last = const_cast<std::list<fun::sql::Time> &>(val).end();
  RealContainerBind<std::list<fun::sql::Time>::iterator, fun::sql::Time>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::NullData>& val, Direction dir, const std::type_info&) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::NullData>& val, Direction dir, const std::type_info&) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<fun::sql::NullData>& val, Direction dir, const std::type_info&) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<String>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::vector<String>::iterator first = const_cast<std::vector<String> &>(val).begin();
  std::vector<String>::iterator last = const_cast<std::vector<String> &>(val).end();
  RealContainerBind<std::vector<String>::iterator, String>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::deque<String>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::deque<String>::iterator first = const_cast<std::deque<String> &>(val).begin();
  std::deque<String>::iterator last = const_cast<std::deque<String> &>(val).end();
  RealContainerBind<std::deque<String>::iterator, String>(pos, first, last);
}

void Binder::Bind(size_t pos, const std::list<String>& val, Direction dir) {
  fun_check(dir == PD_IN);

  std::list<String>::iterator first = const_cast<std::list<String> &>(val).begin();
  std::list<String>::iterator last = const_cast<std::list<String> &>(val).end();
  RealContainerBind<std::list<String>::iterator, String>(pos, first, last);
}

} // namespace postgresql
} // namespace sql
} // namespace fun
