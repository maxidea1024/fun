#pragma once

#include "fun/sql/mysql/mysql.h"
#include "fun/sql/binder_base.h"
#include "fun/sql/lob.h"
#include "fun/sql/mysql/mysql_exception.h"
#include <mysql.h>

namespace fun {
namespace sql {
namespace mysql {

/**
 * Binds placeholders in the sql query to the provided values. Performs data types mapping.
 */
class FUN_MYSQL_API Binder : public fun::sql::BinderBase {
 public:
  typedef SharedPtr<Binder> Ptr;

  /**
   * Creates the Binder.
   */
  Binder();

  /**
   * Destroys the Binder.
   */
  virtual ~Binder();

  virtual void Bind(size_t pos, const int8& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const uint8& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const int16& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const uint16& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const int32& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const uint32& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const int64& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const uint64& val, Direction dir, const WhenNullCb& null_cb);

#ifndef FUN_LONG_IS_64_BIT
  virtual void Bind(size_t pos, const long& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const unsigned long& val, Direction dir, const WhenNullCb& null_cb);
#endif

  virtual void Bind(size_t pos, const bool& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const float& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const double& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const char& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const String& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const fun::sql::BLOB& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const fun::sql::CLOB& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const DateTime& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const Date& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const Time& val, Direction dir, const WhenNullCb& null_cb);
  virtual void Bind(size_t pos, const NullData& val, Direction dir, const std::type_info& bind_type);

  virtual void Bind(size_t pos, const std::vector<int8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint8>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<int16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint16>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<int32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint32>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<int64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<int64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<int64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<uint64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<uint64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<uint64>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<bool>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<bool>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<bool>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<float>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<float>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<float>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<double>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<double>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<double>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<char>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<char>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<char>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<BLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<BLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<BLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<CLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<CLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<CLOB>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<DateTime>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<DateTime>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<DateTime>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Date>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Date>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Date>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<Time>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<Time>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<Time>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::vector<NullData>& val, Direction dir, const std::type_info& bind_type);
  virtual void Bind(size_t pos, const std::deque<NullData>& val, Direction dir, const std::type_info& bind_type);
  virtual void Bind(size_t pos, const std::list<NullData>& val, Direction dir, const std::type_info& bind_type);
  virtual void Bind(size_t pos, const std::vector<String>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::deque<String>& val, Direction dir = PD_IN);
  virtual void Bind(size_t pos, const std::list<String>& val, Direction dir = PD_IN);

  /**
   * Return count of binded parameters
   */
  size_t Count() const;

  /**
   * Return array
   */
  MYSQL_BIND* GetBindArray() const;

  /**
   * Update linked times
   */
  //void UpdateDates();

 private:
  /**
   * Don't copy the binder
   */
  Binder(const Binder&);

  /**
   * Binds a const char ptr.
   * This is a private no-op in this implementation
   * due to security risk.
   */
  virtual void Bind(size_t, const char* const&, Direction, const WhenNullCb&) {}

  /**
   * Common Bind implementation
   */
  void RealBind(size_t pos, enum_field_types type, const void* buffer, int length, bool is_unsigned = false);

 private:
  std::vector<MYSQL_BIND> bind_array_;
  std::vector<MYSQL_TIME*> dates_;
};

} // namespace mysql
} // namespace sql
} // namespace fun
