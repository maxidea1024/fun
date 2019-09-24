#include "fun/base/any.h"
#include "fun/base/date_time.h"
#include "fun/base/dynamic/var.h"
#include "fun/sql/binder_base.h"
#include "fun/sql/date.h"
#include "fun/sql/lob.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/time.h"

namespace fun {
namespace sql {

BinderBase::BinderBase() {}

BinderBase::~BinderBase() {}

void BinderBase::Bind(size_t pos, const std::vector<int8>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<int8>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<int8>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<int8> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<int8> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<int8> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<uint8>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<uint8>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<uint8>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<uint8> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<uint8> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<uint8> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<int16>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<int16>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<int16>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<int16> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<int16> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<int16> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<uint16>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<uint16>& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<uint16>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<uint16> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<uint16> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<uint16> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<int32>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<int32>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<int32>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<int32> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<int32> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<int32> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<uint32>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<uint32>& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<uint32>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<uint32> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<uint32> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<uint32> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<int64>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<int64>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<int64>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<int64> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<int64> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<int64> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<uint64>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<uint64>& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<uint64>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<uint64> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<uint64> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<uint64> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

#ifndef FUN_LONG_IS_64_BIT
void BinderBase::Bind(size_t pos, const std::vector<long>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<long>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<long>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<long> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<long> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<long> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}
#endif

void BinderBase::Bind(size_t pos, const std::vector<bool>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<bool>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<bool>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<bool> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<bool> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<bool> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<float>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<float>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<float>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<float> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<float> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<float> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<double>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<double>& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<double>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<double> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<double> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<double> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<char>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<char>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<char>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<char> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<char> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<char> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<String>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<String>& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<String>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<String> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<String> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<String> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const UString& val, Direction dir,
                      const WhenNullCb& null_cb) {
  throw NotImplementedException("UString binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<UString>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<UString>& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<UString>& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<UString> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<UString> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<UString> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<BLOB>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<BLOB>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<BLOB>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<BLOB> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<BLOB> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<BLOB> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<CLOB>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<CLOB>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<CLOB>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<CLOB> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<CLOB> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<CLOB> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<DateTime>& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<DateTime>& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<DateTime>& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<DateTime> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<DateTime> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<DateTime> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Date>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Date>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Date>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<Date> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<Date> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<Date> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Time>& val, Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Time>& val, Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Time>& val, Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<Nullable<Time> >& val,
                      Direction dir) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<Nullable<Time> >& val,
                      Direction dir) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<Nullable<Time> >& val,
                      Direction dir) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::vector<NullData>& val,
                      Direction dir, const std::type_info& bind_element_type) {
  throw NotImplementedException("std::vector binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::deque<NullData>& val,
                      Direction dir, const std::type_info& bind_element_type) {
  throw NotImplementedException("std::deque binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const std::list<NullData>& val, Direction dir,
                      const std::type_info& bind_element_type) {
  throw NotImplementedException("std::list binder must be implemented.");
}

void BinderBase::Bind(size_t pos, const Any& val, Direction dir,
                      const WhenNullCb& null_cb) {
  const std::type_info& type = val.Type();

  //자주 액세스되는 타입부터 체크해야함.
  if (type == typeid(int32)) {
    Bind(pos, RefAnyCast<int32>(val), dir, null_cb);
  } else if (type == typeid(String)) {
    Bind(pos, RefAnyCast<String>(val), dir, null_cb);
  } else if (type == typeid(fun::UString)) {
    Bind(pos, RefAnyCast<fun::UString>(val), dir, null_cb);
  } else if (type == typeid(bool)) {
    Bind(pos, RefAnyCast<bool>(val), dir, null_cb);
  } else if (type == typeid(char)) {
    Bind(pos, RefAnyCast<char>(val), dir, null_cb);
  } else if (type == typeid(int8)) {
    Bind(pos, RefAnyCast<int8>(val), dir, null_cb);
  } else if (type == typeid(uint8)) {
    Bind(pos, RefAnyCast<uint8>(val), dir, null_cb);
  } else if (type == typeid(int16)) {
    Bind(pos, RefAnyCast<int16>(val), dir, null_cb);
  } else if (type == typeid(uint16)) {
    Bind(pos, RefAnyCast<uint16>(val), dir, null_cb);
  } else if (type == typeid(uint32)) {
    Bind(pos, RefAnyCast<uint32>(val), dir, null_cb);
  } else if (type == typeid(int64)) {
    Bind(pos, RefAnyCast<int64>(val), dir, null_cb);
  } else if (type == typeid(uint64)) {
    Bind(pos, RefAnyCast<uint64>(val), dir, null_cb);
  } else if (type == typeid(float)) {
    Bind(pos, RefAnyCast<float>(val), dir, null_cb);
  } else if (type == typeid(double)) {
    Bind(pos, RefAnyCast<double>(val), dir, null_cb);
  } else if (type == typeid(DateTime)) {
    Bind(pos, RefAnyCast<DateTime>(val), dir, null_cb);
  } else if (type == typeid(Date)) {
    Bind(pos, RefAnyCast<Date>(val), dir, null_cb);
  } else if (type == typeid(Time)) {
    Bind(pos, RefAnyCast<Time>(val), dir, null_cb);
  } else if (type == typeid(BLOB)) {
    Bind(pos, RefAnyCast<BLOB>(val), dir, null_cb);
  } else if (type == typeid(void)) {
    Bind(pos, NULL_GENERIC, dir, type);
  }
#ifndef FUN_LONG_IS_64_BIT
  else if (type == typeid(long)) {
    Bind(pos, RefAnyCast<long>(val), dir, null_cb);
  }
#endif
  else {
    throw UnknownTypeException(String(val.type().name()));
  }
}

void BinderBase::Bind(size_t pos, const fun::dynamic::Var& val, Direction dir,
                      const WhenNullCb& null_cb) {
  const std::type_info& type = val.Type();

  //자주 액세스되는 타입부터 체크해야함.
  if (type == typeid(int32)) {
    Bind(pos, val.Extract<int32>(), dir, null_cb);
  } else if (type == typeid(String)) {
    Bind(pos, val.Extract<String>(), dir, null_cb);
  } else if (type == typeid(fun::UString)) {
    Bind(pos, val.Extract<fun::UString>(), dir, null_cb);
  } else if (type == typeid(bool)) {
    Bind(pos, val.Extract<bool>(), dir, null_cb);
  } else if (type == typeid(char)) {
    Bind(pos, val.Extract<char>(), dir, null_cb);
  } else if (type == typeid(int8)) {
    Bind(pos, val.Extract<int8>(), dir, null_cb);
  } else if (type == typeid(uint8)) {
    Bind(pos, val.Extract<uint8>(), dir, null_cb);
  } else if (type == typeid(int16)) {
    Bind(pos, val.Extract<int16>(), dir, null_cb);
  } else if (type == typeid(uint16)) {
    Bind(pos, val.Extract<uint16>(), dir, null_cb);
  } else if (type == typeid(uint32)) {
    Bind(pos, val.Extract<uint32>(), dir, null_cb);
  } else if (type == typeid(int64)) {
    Bind(pos, val.Extract<int64>(), dir, null_cb);
  } else if (type == typeid(uint64)) {
    Bind(pos, val.Extract<uint64>(), dir, null_cb);
  } else if (type == typeid(float)) {
    Bind(pos, val.Extract<float>(), dir, null_cb);
  } else if (type == typeid(double)) {
    Bind(pos, val.Extract<double>(), dir, null_cb);
  } else if (type == typeid(DateTime)) {
    Bind(pos, val.Extract<DateTime>(), dir, null_cb);
  } else if (type == typeid(Date)) {
    Bind(pos, val.Extract<Date>(), dir, null_cb);
  } else if (type == typeid(Time)) {
    Bind(pos, val.Extract<Time>(), dir, null_cb);
  } else if (type == typeid(BLOB)) {
    Bind(pos, val.Extract<BLOB>(), dir, null_cb);
  } else if (type == typeid(void)) {
    Bind(pos, NULL_GENERIC, dir, type);
  } else if (type == typeid(NullData)) {
    Bind(pos, val.Extract<NullData>(), dir, type);
  } else if (type == typeid(NullType)) {
    Bind(pos, static_cast<NullData>(val.Extract<NullType>()), dir, type);
  }
#ifndef FUN_LONG_IS_64_BIT
  else if (type == typeid(long)) {
    Bind(pos, val.Extract<long>(), dir, null_cb);
  }
#endif
  else {
    throw UnknownTypeException(String(val.type().name()));
  }
}

}  // namespace sql
}  // namespace fun
