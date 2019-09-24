#include "fun/sql/mysql/binder.h"

namespace fun {
namespace sql {
namespace mysql {

Binder::Binder() {}

Binder::~Binder() {
  for (std::vector<MYSQL_TIME*>::iterator it = dates_.begin();
       it != dates_.end(); ++it) {
    delete *it;
    *it = 0;
  }
}

void Binder::Bind(size_t pos, const int8& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_TINY, &val, 0);
}

void Binder::Bind(size_t pos, const uint8& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_TINY, &val, 0, true);
}

void Binder::Bind(size_t pos, const int16& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_SHORT, &val, 0);
}

void Binder::Bind(size_t pos, const uint16& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_SHORT, &val, 0, true);
}

void Binder::Bind(size_t pos, const int32& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_LONG, &val, 0);
}

void Binder::Bind(size_t pos, const uint32& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_LONG, &val, 0, true);
}

void Binder::Bind(size_t pos, const int64& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_LONGLONG, &val, 0);
}

void Binder::Bind(size_t pos, const uint64& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_LONGLONG, &val, 0, true);
}

#ifndef FUN_LONG_IS_64_BIT

void Binder::Bind(size_t pos, const long& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_LONG, &val, 0);
}

void Binder::Bind(size_t pos, const unsigned long& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_LONG, &val, 0, true);
}

#endif  // FUN_LONG_IS_64_BIT

void Binder::Bind(size_t pos, const bool& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_TINY, &val, 0);
}

void Binder::Bind(size_t pos, const float& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_FLOAT, &val, 0);
}

void Binder::Bind(size_t pos, const double& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_DOUBLE, &val, 0);
}

void Binder::Bind(size_t pos, const char& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_TINY, &val, 0);
}

void Binder::Bind(size_t pos, const String& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_STRING, val.c_str(), static_cast<int>(val.length()));
}

void Binder::Bind(size_t pos, const fun::sql::BLOB& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_BLOB, val.GetRawContent(),
           static_cast<int>(val.size()));
}

void Binder::Bind(size_t pos, const fun::sql::CLOB& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_BLOB, val.GetRawContent(),
           static_cast<int>(val.size()));
}

void Binder::Bind(size_t pos, const DateTime& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  MYSQL_TIME mt = {0};

  mt.year = val.Year();
  mt.month = val.Month();
  mt.day = val.Day();
  mt.hour = val.Hour();
  mt.minute = val.Minute();
  mt.second = val.Second();
  mt.second_part = val.Millisecond() * 1000 + val.Microsecond();

  mt.time_type = MYSQL_TIMESTAMP_DATETIME;

  dates_.push_back(new MYSQL_TIME(mt));

  RealBind(pos, MYSQL_TYPE_DATETIME, dates_.back(), sizeof(MYSQL_TIME));
}

void Binder::Bind(size_t pos, const Date& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  MYSQL_TIME mt = {0};

  mt.year = val.Year();
  mt.month = val.Month();
  mt.day = val.Day();

  mt.time_type = MYSQL_TIMESTAMP_DATE;

  dates_.push_back(new MYSQL_TIME(mt));

  RealBind(pos, MYSQL_TYPE_DATE, dates_.back(), sizeof(MYSQL_TIME));
}

void Binder::Bind(size_t pos, const Time& val, Direction dir,
                  const WhenNullCb& null_cb) {
  fun_check(dir == PD_IN);
  MYSQL_TIME mt = {0};

  mt.hour = val.Hour();
  mt.minute = val.Minute();
  mt.second = val.Second();

  mt.time_type = MYSQL_TIMESTAMP_TIME;

  dates_.push_back(new MYSQL_TIME(mt));

  RealBind(pos, MYSQL_TYPE_TIME, dates_.back(), sizeof(MYSQL_TIME));
}

void Binder::Bind(size_t pos, const NullData&, Direction dir,
                  const std::type_info& /*bind_type*/) {
  fun_check(dir == PD_IN);
  RealBind(pos, MYSQL_TYPE_NULL, 0, 0);
}

size_t Binder::Count() const { return static_cast<size_t>(bind_array_.size()); }

MYSQL_BIND* Binder::GetBindArray() const {
  if (bind_array_.size() == 0) {
    return 0;
  }

  return const_cast<MYSQL_BIND*>(&bind_array_[0]);
}

/*void Binder::UpdateDates() {
  for (size_t i = 0; i < dates_.size(); i++) {
    switch (dates_[i].mt.time_type) {
      case MYSQL_TIMESTAMP_DATE:
        dates_[i].mt.year = dates_[i].link.date->Year();
        dates_[i].mt.month = dates_[i].link.date->Month();
        dates_[i].mt.day = dates_[i].link.date->Day();
        break;
      case MYSQL_TIMESTAMP_DATETIME:
        dates_[i].mt.year = dates_[i].link.datetime->Year();
        dates_[i].mt.month = dates_[i].link.datetime->Month();
        dates_[i].mt.day = dates_[i].link.datetime->Day();
        dates_[i].mt.hour = dates_[i].link.datetime->Hour();
        dates_[i].mt.minute = dates_[i].link.datetime->Minute();
        dates_[i].mt.second = dates_[i].link.datetime->Second();
        dates_[i].mt.second_part = dates_[i].link.datetime->Millisecond();
        break;
      case MYSQL_TIMESTAMP_TIME:
        dates_[i].mt.hour   = dates_[i].link.time->Hour();
        dates_[i].mt.minute = dates_[i].link.time->Minute();
        dates_[i].mt.second = dates_[i].link.time->Second();
        break;
    }
  }
}*/

///////////////////
//
// Private
//
////////////////////

void Binder::RealBind(size_t pos, enum_field_types type, const void* buffer,
                      int length, bool is_unsigned) {
  if (pos >= bind_array_.size()) {
    size_t s = static_cast<size_t>(bind_array_.size());
    bind_array_.resize(pos + 1);

    UnsafeMemory::Memset(&bind_array_[s], 0,
                         sizeof(MYSQL_BIND) * (bind_array_.size() - s));
  }

  MYSQL_BIND b = {0};

  b.buffer_type = type;
  b.buffer = const_cast<void*>(buffer);
  b.buffer_length = length;
  b.is_unsigned = is_unsigned;

  bind_array_[pos] = b;
}

void Binder::Bind(size_t pos, const std::vector<int8>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<int8>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<int8>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<uint8>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<uint8>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<uint8>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<int16>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<int16>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<int16>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<uint16>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<uint16>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<uint16>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<int32>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<int32>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<int32>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<uint32>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<uint32>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<uint32>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<int64>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<int64>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<int64>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<uint64>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<uint64>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<uint64>& val, Direction dir) {
  throw NotImplementedException();
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
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<float>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<float>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<double>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<double>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<double>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<char>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<char>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<char>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::BLOB>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::BLOB>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<fun::sql::BLOB>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::CLOB>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::CLOB>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<fun::sql::CLOB>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<fun::DateTime>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<fun::DateTime>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<fun::DateTime>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::Date>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::Date>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<fun::sql::Date>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::Time>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::Time>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<fun::sql::Time>& val,
                  Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<fun::sql::NullData>& val,
                  Direction dir, const std::type_info& /*bind_type*/) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<fun::sql::NullData>& val,
                  Direction dir, const std::type_info& /*bind_type*/) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<fun::sql::NullData>& val,
                  Direction dir, const std::type_info& /*bind_type*/) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::vector<String>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::deque<String>& val, Direction dir) {
  throw NotImplementedException();
}

void Binder::Bind(size_t pos, const std::list<String>& val, Direction dir) {
  throw NotImplementedException();
}

}  // namespace mysql
}  // namespace sql
}  // namespace fun
