#pragma once

#include <cstddef>
#include <deque>
#include <list>
#include <vector>
#include "fun/Nullable.h"
#include "fun/UTFString.h"
#include "fun/base/any.h"
#include "fun/base/date_time.h"
#include "fun/base/dynamic/var.h"
#include "fun/sql/date.h"
#include "fun/sql/lob.h"
#include "fun/sql/sql.h"
#include "fun/sql/time.h"

namespace fun {
namespace sql {

enum NullData {
  NULL_GENERIC = fun::NULL_GENERIC,
  DATA_NULL_INTEGER = 1,
  DATA_NULL_STRING = 2,
  DATA_NULL_DATE = 3,
  DATA_NULL_TIME = 4,
  DATA_NULL_DATETIME = 5,
  DATA_NULL_BLOB = 6,
  DATA_NULL_FLOAT = 7
};

struct NullValue {
  NullValue() {}

  template <typename T>
  operator fun::Nullable<T>() const {
    return fun::Nullable<T>();
  }

  template <typename T>
  static NullData NullCode() {
    return sql::NULL_GENERIC;
  }
};

namespace Keywords {

static const NullValue null;

}  // namespace Keywords

template <typename T>
inline bool operator==(const NullValue& nv, const Nullable<T>& n) {
  return n.IsNull();
}

template <typename T>
inline bool operator!=(const NullValue& nv, const Nullable<T>& n) {
  return !n.IsNull();
}

/**
 * Interface for Binding data types to placeholders.
 */
class FUN_SQL_API BinderBase {
 public:
  typedef SharedPtr<BinderBase> Ptr;

  struct WhenNullCb {
    WhenNullCb() : func_(NULL) {}

    inline bool IsDefined() const { return (func_ != NULL); }

    inline void OnNull() {
      if (func_) {
        func_(data_);
      }
    }

   protected:
    void* data_;
    void (*func_)(void*);
  };

  /**
   * Binding direction for a parameter.
   */
  enum Direction { PD_IN, PD_OUT, PD_IN_OUT };

  /**
   * Creates the BinderBase.
   */
  BinderBase();

  /**
   * Destroys the BinderBase.
   */
  virtual ~BinderBase();

  virtual void Bind(size_t pos, const int8& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<int8>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int8>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int8>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<int8> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<int8> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<int8> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const uint8& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<uint8>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint8>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint8>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<uint8> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<uint8> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<uint8> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const int16& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<int16>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int16>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int16>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<int16> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<int16> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<int16> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const uint16& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<uint16>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint16>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint16>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<uint16> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<uint16> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<uint16> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const int32& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<int32>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int32>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int32>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<int32> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<int32> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<int32> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const uint32& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<uint32>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint32>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint32>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<uint32> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<uint32> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<uint32> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const int64& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<int64>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int64>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int64>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<int64> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<int64> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<int64> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const uint64& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<uint64>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint64>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint64>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<uint64> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<uint64> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<uint64> >& val,
                    Direction dir = PD_IN);

#ifndef FUN_LONG_IS_64_BIT
  virtual void Bind(size_t pos, const long& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const unsigned long& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<long>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<long>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<long>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<long> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<long> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<long> >& val,
                    Direction dir = PD_IN);
#endif

  virtual void Bind(size_t pos, const bool& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<bool>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<bool>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<bool>& val,
                    Direction dir = PD_IN);

  virtual void Bind(size_t pos, const std::vector<Nullable<bool> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<bool> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<bool> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const float& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<float>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<float>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<float>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<float> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<float> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<float> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const double& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<double>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<double>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<double>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<double> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<double> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<double> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const char& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<char>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<char>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<char>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<char> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<char> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<char> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const char* const& pVal, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const String& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<String>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<String>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<String>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<String> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<String> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<String> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const UString& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb());
  virtual void Bind(size_t pos, const std::vector<UString>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<UString>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<UString>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<UString> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<UString> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<UString> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const BLOB& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const CLOB& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<BLOB>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<BLOB>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<BLOB>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<BLOB> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<BLOB> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<BLOB> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<CLOB>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<CLOB>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<CLOB>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<CLOB> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<CLOB> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<CLOB> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const DateTime& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<DateTime>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<DateTime>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<DateTime>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<DateTime> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<DateTime> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<DateTime> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const Date& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<Date>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Date>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Date>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<Date> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<Date> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<Date> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const Time& val, Direction dir = PD_IN,
                    const WhenNullCb& null_cb = WhenNullCb()) = 0;
  virtual void Bind(size_t pos, const std::vector<Time>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Time>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Time>& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Nullable<Time> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Nullable<Time> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Nullable<Time> >& val,
                    Direction dir = PD_IN);
  virtual void Bind(size_t pos, const NullData& val, Direction dir = PD_IN,
                    const std::type_info& bind_type = typeid(void)) = 0;
  virtual void Bind(size_t pos, const std::vector<NullData>& val,
                    Direction dir = PD_IN,
                    const std::type_info& bindElemType = typeid(void));
  virtual void Bind(size_t pos, const std::deque<NullData>& val,
                    Direction dir = PD_IN,
                    const std::type_info& bindElemType = typeid(void));
  virtual void Bind(size_t pos, const std::list<NullData>& val,
                    Direction dir = PD_IN,
                    const std::type_info& bindElemType = typeid(void));
  void Bind(size_t pos, const Any& val, Direction dir = PD_IN,
            const WhenNullCb& null_cb = WhenNullCb());
  void Bind(size_t pos, const fun::dynamic::Var& val, Direction dir = PD_IN,
            const WhenNullCb& null_cb = WhenNullCb());

  /**
   * Resets a binder. No-op by default. Implement for binders that cache data.
   */
  virtual void Reset();

  /**
   * Returns true if direction is out bound;
   */
  static bool IsOutBound(Direction dir);

  /**
   * Returns true if direction is in bound;
   */
  static bool IsInBound(Direction dir);

  /**
   * Returns true if direction is in and out bound;
   */
  static bool IsInOutBound(Direction dir);
};

//
// inlines
//

inline void BinderBase::Reset() {
  // NOOP
}

inline bool BinderBase::IsOutBound(Direction dir) {
  return PD_OUT == dir || PD_IN_OUT == dir;
}

inline bool BinderBase::IsInOutBound(Direction dir) { return PD_IN_OUT == dir; }

inline bool BinderBase::IsInBound(Direction dir) {
  return PD_IN == dir || PD_IN_OUT == dir;
}

}  // namespace sql
}  // namespace fun
