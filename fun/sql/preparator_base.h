#pragma once

#include "fun/base/utf_string.h"
#include "fun/ref_counted_object.h"
#include "fun/sql/lob.h"
#include "fun/sql/sql.h"

#include <cstddef>
#include <deque>
#include <list>
#include <vector>

namespace fun {

class DateTime;
class Any;

namespace Dynamic {
class Var;
}

namespace sql {

class Date;
class Time;

/**
 * Interface used for database preparation where we first have to register all
 * data types (and memory output locations) before extracting data, e.g. ODBC.
 * Extract works as two-phase extract: first we call Prepare once, then extract
 * n-times. There are cases (bulk operations using std::vector storage) when
 * extract is called only once. The value passed to a Prepare() call is not used
 * by the Prepare, serving only as an indication of the data type being
 * prepared, thus all values are passed as const references. Implementing this
 * interface is not mandatory for a connector. Connectors that only extract data
 * after sql execution (e.g. SQLite) do not need this functionality at all.
 */
class FUN_SQL_API PreparatorBase {
 public:
  typedef SharedPtr<PreparatorBase> Ptr;

  /**
   * Creates the PreparatorBase.
   */
  PreparatorBase(uint32 length = 1u);

  /**
   * Destroys the PreparatorBase.
   */
  virtual ~PreparatorBase();

  /**
   * Prepares an int8.
   */
  virtual void Prepare(size_t pos, const int8&) = 0;

  /**
   * Prepares an int8 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<int8>& val);

  /**
   * Prepares an int8 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<int8>& val);

  /**
   * Prepares an int8 list.
   */
  virtual void Prepare(size_t pos, const std::list<int8>& val);

  /**
   * Prepares an uint8.
   */
  virtual void Prepare(size_t pos, const uint8&) = 0;

  /**
   * Prepares an uint8 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<uint8>& val);

  /**
   * Prepares an uint8 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<uint8>& val);

  /**
   * Prepares an uint8 list.
   */
  virtual void Prepare(size_t pos, const std::list<uint8>& val);

  /**
   * Prepares an int16.
   */
  virtual void Prepare(size_t pos, const int16&) = 0;

  /**
   * Prepares an int16 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<int16>& val);

  /**
   * Prepares an int16 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<int16>& val);

  /**
   * Prepares an int16 list.
   */
  virtual void Prepare(size_t pos, const std::list<int16>& val);

  /**
   * Prepares an uint16.
   */
  virtual void Prepare(size_t pos, const uint16&) = 0;

  /**
   * Prepares an uint16 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<uint16>& val);

  /**
   * Prepares an uint16 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<uint16>& val);

  /**
   * Prepares an uint16 list.
   */
  virtual void Prepare(size_t pos, const std::list<uint16>& val);

  /**
   * Prepares an int32.
   */
  virtual void Prepare(size_t pos, const int32&) = 0;

  /**
   * Prepares an int32 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<int32>& val);

  /**
   * Prepares an int32 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<int32>& val);

  /**
   * Prepares an int32 list.
   */
  virtual void Prepare(size_t pos, const std::list<int32>& val);

  /**
   * Prepares an uint32.
   */
  virtual void Prepare(size_t pos, const uint32&) = 0;

  /**
   * Prepares an uint32 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<uint32>& val);

  /**
   * Prepares an uint32 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<uint32>& val);

  /**
   * Prepares an uint32 list.
   */
  virtual void Prepare(size_t pos, const std::list<uint32>& val);

  /**
   * Prepares an int64.
   */
  virtual void Prepare(size_t pos, const int64&) = 0;

  /**
   * Prepares an int64 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<int64>& val);

  /**
   * Prepares an int64 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<int64>& val);

  /**
   * Prepares an int64 list.
   */
  virtual void Prepare(size_t pos, const std::list<int64>& val);

  /**
   * Prepares an uint64.
   */
  virtual void Prepare(size_t pos, const uint64&) = 0;

  /**
   * Prepares an uint64 vector.
   */
  virtual void Prepare(size_t pos, const std::vector<uint64>& val);

  /**
   * Prepares an uint64 deque.
   */
  virtual void Prepare(size_t pos, const std::deque<uint64>& val);

  /**
   * Prepares an uint64 list.
   */
  virtual void Prepare(size_t pos, const std::list<uint64>& val);

#ifndef FUN_LONG_IS_64_BIT
  /**
   * Prepares a long.
   */
  virtual void Prepare(size_t pos, const long&) = 0;

  /**
   * Prepares an unsigned long.
   */
  virtual void Prepare(size_t pos, const unsigned long&) = 0;

  /**
   * Prepares a long vector.
   */
  virtual void Prepare(size_t pos, const std::vector<long>& val);

  /**
   * Prepares a long deque.
   */
  virtual void Prepare(size_t pos, const std::deque<long>& val);

  /**
   * Prepares a long list.
   */
  virtual void Prepare(size_t pos, const std::list<long>& val);
#endif

  /**
   * Prepares a boolean.
   */
  virtual void Prepare(size_t pos, const bool&) = 0;

  /**
   * Prepares a boolean vector.
   */
  virtual void Prepare(size_t pos, const std::vector<bool>& val);

  /**
   * Prepares a boolean deque.
   */
  virtual void Prepare(size_t pos, const std::deque<bool>& val);

  /**
   * Prepares a boolean list.
   */
  virtual void Prepare(size_t pos, const std::list<bool>& val);

  /**
   * Prepares a float.
   */
  virtual void Prepare(size_t pos, const float&) = 0;

  /**
   * Prepares a float vector.
   */
  virtual void Prepare(size_t pos, const std::vector<float>& val);

  /**
   * Prepares a float deque.
   */
  virtual void Prepare(size_t pos, const std::deque<float>& val);

  /**
   * Prepares a float list.
   */
  virtual void Prepare(size_t pos, const std::list<float>& val);

  /**
   * Prepares a double.
   */
  virtual void Prepare(size_t pos, const double&) = 0;

  /**
   * Prepares a double vector.
   */
  virtual void Prepare(size_t pos, const std::vector<double>& val);

  /**
   * Prepares a double deque.
   */
  virtual void Prepare(size_t pos, const std::deque<double>& val);

  /**
   * Prepares a double list.
   */
  virtual void Prepare(size_t pos, const std::list<double>& val);

  /**
   * Prepares a single character.
   */
  virtual void Prepare(size_t pos, const char&) = 0;

  /**
   * Prepares a character vector.
   */
  virtual void Prepare(size_t pos, const std::vector<char>& val);

  /**
   * Prepares a character deque.
   */
  virtual void Prepare(size_t pos, const std::deque<char>& val);

  /**
   * Prepares a character list.
   */
  virtual void Prepare(size_t pos, const std::list<char>& val);

  /**
   * Prepares a string.
   */
  virtual void Prepare(size_t pos, const String&) = 0;

  /**
   * Prepares a string vector.
   */
  virtual void Prepare(size_t pos, const std::vector<String>& val);

  /**
   * Prepares a string deque.
   */
  virtual void Prepare(size_t pos, const std::deque<String>& val);

  /**
   * Prepares a character list.
   */
  virtual void Prepare(size_t pos, const std::list<String>& val);

  /**
   * Prepares a UString.
   */
  virtual void Prepare(size_t pos, const UString&);

  /**
   * Prepares a UString vector.
   */
  virtual void Prepare(size_t pos, const std::vector<UString>& val);

  /**
   * Prepares a UString deque.
   */
  virtual void Prepare(size_t pos, const std::deque<UString>& val);

  /**
   * Prepares a UString list.
   */
  virtual void Prepare(size_t pos, const std::list<UString>& val);

  /**
   * Prepares a BLOB.
   */
  virtual void Prepare(size_t pos, const BLOB&) = 0;

  /**
   * Prepares a CLOB.
   */
  virtual void Prepare(size_t pos, const CLOB&) = 0;

  /**
   * Prepares a BLOB vector.
   */
  virtual void Prepare(size_t pos, const std::vector<BLOB>& val);

  /**
   * Prepares a BLOB deque.
   */
  virtual void Prepare(size_t pos, const std::deque<BLOB>& val);

  /**
   * Prepares a BLOB list.
   */
  virtual void Prepare(size_t pos, const std::list<BLOB>& val);

  /**
   * Prepares a CLOB vector.
   */
  virtual void Prepare(size_t pos, const std::vector<CLOB>& val);

  /**
   * Prepares a CLOB deque.
   */
  virtual void Prepare(size_t pos, const std::deque<CLOB>& val);

  /**
   * Prepares a CLOB list.
   */
  virtual void Prepare(size_t pos, const std::list<CLOB>& val);

  /**
   * Prepares a DateTime.
   */
  virtual void Prepare(size_t pos, const DateTime&) = 0;

  /**
   * Prepares a DateTime vector.
   */
  virtual void Prepare(size_t pos, const std::vector<DateTime>& val);

  /**
   * Prepares a DateTime deque.
   */
  virtual void Prepare(size_t pos, const std::deque<DateTime>& val);

  /**
   * Prepares a DateTime list.
   */
  virtual void Prepare(size_t pos, const std::list<DateTime>& val);

  /**
   * Prepares a Date.
   */
  virtual void Prepare(size_t pos, const Date&) = 0;

  /**
   * Prepares a Date vector.
   */
  virtual void Prepare(size_t pos, const std::vector<Date>& val);

  /**
   * Prepares a Date deque.
   */
  virtual void Prepare(size_t pos, const std::deque<Date>& val);

  /**
   * Prepares a Date list.
   */
  virtual void Prepare(size_t pos, const std::list<Date>& val);

  /**
   * Prepares a Time.
   */
  virtual void Prepare(size_t pos, const Time&) = 0;

  /**
   * Prepares a Time vector.
   */
  virtual void Prepare(size_t pos, const std::vector<Time>& val);

  /**
   * Prepares a Time deque.
   */
  virtual void Prepare(size_t pos, const std::deque<Time>& val);

  /**
   * Prepares a Time list.
   */
  virtual void Prepare(size_t pos, const std::list<Time>& val);

  /**
   * Prepares an Any.
   */
  virtual void Prepare(size_t pos, const Any&) = 0;

  /**
   * Prepares an Any vector.
   */
  virtual void Prepare(size_t pos, const std::vector<Any>& val);

  /**
   * Prepares an Any deque.
   */
  virtual void Prepare(size_t pos, const std::deque<Any>& val);

  /**
   * Prepares an Any list.
   */
  virtual void Prepare(size_t pos, const std::list<Any>& val);

  /**
   * Prepares a Var.
   */
  virtual void Prepare(size_t pos, const fun::dynamic::Var&) = 0;

  /**
   * Prepares a Var vector.
   */
  virtual void Prepare(size_t pos, const std::vector<fun::dynamic::Var>& val);

  /**
   * Prepares a Var deque.
   */
  virtual void Prepare(size_t pos, const std::deque<fun::dynamic::Var>& val);

  /**
   * Prepares a Var list.
   */
  virtual void Prepare(size_t pos, const std::list<fun::dynamic::Var>& val);

  /**
   * Sets the length of prepared data.
   * Needed only for data lengths greater than 1 (i.e. for
   * bulk operations).
   */
  void SetLength(uint32 length);

  /**
   * Returns the length of prepared data. Defaults to 1.
   * The length is greater than one for bulk operations.
   */
  uint32 GetLength() const;

  /**
   * Sets bulk operation flag (always false at object creation time)
   */
  void SetBulk(bool flag = true);

  /**
   * Returns bulk operation flag.
   */
  bool IsBulk() const;

 private:
  uint32 length_;
  bool bulk_;
};

//
// inlines
//

inline void PreparatorBase::SetLength(uint32 length) { length_ = length; }

inline uint32 PreparatorBase::GetLength() const { return length_; }

inline void PreparatorBase::SetBulk(bool flag) { bulk_ = flag; }

inline bool PreparatorBase::IsBulk() const { return bulk_; }

}  // namespace sql
}  // namespace fun
