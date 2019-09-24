#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/constants.h"
#include "fun/sql/lob.h"
#include "fun/base/utf_string.h"
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <cstddef>

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
 * Interface used to Extract data from a single result row.
 * If an extractor receives null it is not allowed to change val!
 */
class FUN_SQL_API ExtractorBase {
 public:
  typedef SharedPtr<ExtractorBase> Ptr;

  /**
   * Creates the ExtractorBase.
   */
  ExtractorBase();

  /**
   * Destroys the ExtractorBase.
   */
  virtual ~ExtractorBase();

  /**
   * Extracts an int8. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, int8& val) = 0;

  /**
   * Extracts an int8 vector.
   */
  virtual bool Extract(size_t pos, std::vector<int8>& val);

  /**
   * Extracts an int8 deque.
   */
  virtual bool Extract(size_t pos, std::deque<int8>& val);

  /**
   * Extracts an int8 list.
   */
  virtual bool Extract(size_t pos, std::list<int8>& val);

  /**
   * Extracts an uint8. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, uint8& val) = 0;

  /**
   * Extracts an uint8 vector.
   */
  virtual bool Extract(size_t pos, std::vector<uint8>& val);

  /**
   * Extracts an uint8 deque.
   */
  virtual bool Extract(size_t pos, std::deque<uint8>& val);

  /**
   * Extracts an uint8 list.
   */
  virtual bool Extract(size_t pos, std::list<uint8>& val);

  /**
   * Extracts an int16. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, int16& val) = 0;

  /**
   * Extracts an int16 vector.
   */
  virtual bool Extract(size_t pos, std::vector<int16>& val);

  /**
   * Extracts an int16 deque.
   */
  virtual bool Extract(size_t pos, std::deque<int16>& val);

  /**
   * Extracts an int16 list.
   */
  virtual bool Extract(size_t pos, std::list<int16>& val);

  /**
   * Extracts an uint16. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, uint16& val) = 0;

  /**
   * Extracts an uint16 vector.
   */
  virtual bool Extract(size_t pos, std::vector<uint16>& val);

  /**
   * Extracts an uint16 deque.
   */
  virtual bool Extract(size_t pos, std::deque<uint16>& val);

  /**
   * Extracts an uint16 list.
   */
  virtual bool Extract(size_t pos, std::list<uint16>& val);

  /**
   * Extracts an int32. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, int32& val) = 0;

  /**
   * Extracts an int32 vector.
   */
  virtual bool Extract(size_t pos, std::vector<int32>& val);

  /**
   * Extracts an int32 deque.
   */
  virtual bool Extract(size_t pos, std::deque<int32>& val);

  /**
   * Extracts an int32 list.
   */
  virtual bool Extract(size_t pos, std::list<int32>& val);

  /**
   * Extracts an uint32. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, uint32& val) = 0;

  /**
   * Extracts an uint32 vector.
   */
  virtual bool Extract(size_t pos, std::vector<uint32>& val);

  /**
   * Extracts an uint32 deque.
   */
  virtual bool Extract(size_t pos, std::deque<uint32>& val);

  /**
   * Extracts an uint32 list.
   */
  virtual bool Extract(size_t pos, std::list<uint32>& val);

  /**
   * Extracts an int64. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, int64& val) = 0;

  /**
   * Extracts an int64 vector.
   */
  virtual bool Extract(size_t pos, std::vector<int64>& val);

  /**
   * Extracts an int64 deque.
   */
  virtual bool Extract(size_t pos, std::deque<int64>& val);

  /**
   * Extracts an int64 list.
   */
  virtual bool Extract(size_t pos, std::list<int64>& val);

  /**
   * Extracts an uint64. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, uint64& val) = 0;

  /**
   * Extracts an uint64 vector.
   */
  virtual bool Extract(size_t pos, std::vector<uint64>& val);

  /**
   * Extracts an uint64 deque.
   */
  virtual bool Extract(size_t pos, std::deque<uint64>& val);

  /**
   * Extracts an uint64 list.
   */
  virtual bool Extract(size_t pos, std::list<uint64>& val);

#ifndef FUN_LONG_IS_64_BIT
  /**
   * Extracts a long. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, long& val) = 0;

  /**
   * Extracts an unsigned long. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, unsigned long& val) = 0;

  /**
   * Extracts a long vector.
   */
  virtual bool Extract(size_t pos, std::vector<long>& val);

  /**
   * Extracts a long deque.
   */
  virtual bool Extract(size_t pos, std::deque<long>& val);

  /**
   * Extracts a long list.
   */
  virtual bool Extract(size_t pos, std::list<long>& val);
#endif

  /**
   * Extracts a boolean. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, bool& val) = 0;

  /**
   * Extracts a boolean vector.
   */
  virtual bool Extract(size_t pos, std::vector<bool>& val);

  /**
   * Extracts a boolean deque.
   */
  virtual bool Extract(size_t pos, std::deque<bool>& val);

  /**
   * Extracts a boolean list.
   */
  virtual bool Extract(size_t pos, std::list<bool>& val);

  /**
   * Extracts a float. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, float& val) = 0;

  /**
   * Extracts a float vector.
   */
  virtual bool Extract(size_t pos, std::vector<float>& val);

  /**
   * Extracts a float deque.
   */
  virtual bool Extract(size_t pos, std::deque<float>& val);

  /**
   * Extracts a float list.
   */
  virtual bool Extract(size_t pos, std::list<float>& val);

  /**
   * Extracts a double. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, double& val) = 0;

  /**
   * Extracts a double vector.
   */
  virtual bool Extract(size_t pos, std::vector<double>& val);

  /**
   * Extracts a double deque.
   */
  virtual bool Extract(size_t pos, std::deque<double>& val);

  /**
   * Extracts a double list.
   */
  virtual bool Extract(size_t pos, std::list<double>& val);

  /**
   * Extracts a single character. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, char& val) = 0;

  /**
   * Extracts a character vector.
   */
  virtual bool Extract(size_t pos, std::vector<char>& val);

  /**
   * Extracts a character deque.
   */
  virtual bool Extract(size_t pos, std::deque<char>& val);

  /**
   * Extracts a character list.
   */
  virtual bool Extract(size_t pos, std::list<char>& val);

  /**
   * Extracts a string. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, String& val) = 0;

  /**
   * Extracts a string vector.
   */
  virtual bool Extract(size_t pos, std::vector<String>& val);

  /**
   * Extracts a string deque.
   */
  virtual bool Extract(size_t pos, std::deque<String>& val);

  /**
   * Extracts a string list.
   */
  virtual bool Extract(size_t pos, std::list<String>& val);

  /**
   * Extracts a UString. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, UString& val);

  /**
   * Extracts a UString vector.
   */
  virtual bool Extract(size_t pos, std::vector<UString>& val);

  /**
   * Extracts a UString deque.
   */
  virtual bool Extract(size_t pos, std::deque<UString>& val);

  /**
   * Extracts a UString list.
   */
  virtual bool Extract(size_t pos, std::list<UString>& val);

  /**
   * Extracts a BLOB. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, BLOB& val) = 0;

  /**
   * Extracts a CLOB. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, CLOB& val) = 0;

  /**
   * Extracts a BLOB vector.
   */
  virtual bool Extract(size_t pos, std::vector<BLOB>& val);

  /**
   * Extracts a BLOB deque.
   */
  virtual bool Extract(size_t pos, std::deque<BLOB>& val);

  /**
   * Extracts a BLOB list.
   */
  virtual bool Extract(size_t pos, std::list<BLOB>& val);

  /**
   * Extracts a CLOB vector.
   */
  virtual bool Extract(size_t pos, std::vector<CLOB>& val);

  /**
   * Extracts a CLOB deque.
   */
  virtual bool Extract(size_t pos, std::deque<CLOB>& val);

  /**
   * Extracts a CLOB list.
   */
  virtual bool Extract(size_t pos, std::list<CLOB>& val);

  /**
   * Extracts a DateTime. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, DateTime& val) = 0;

  /**
   * Extracts a DateTime vector.
   */
  virtual bool Extract(size_t pos, std::vector<DateTime>& val);

  /**
   * Extracts a DateTime deque.
   */
  virtual bool Extract(size_t pos, std::deque<DateTime>& val);

  /**
   * Extracts a DateTime list.
   */
  virtual bool Extract(size_t pos, std::list<DateTime>& val);

  /**
   * Extracts a Date. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, Date& val) = 0;

  /**
   * Extracts a Date vector.
   */
  virtual bool Extract(size_t pos, std::vector<Date>& val);

  /**
   * Extracts a Date deque.
   */
  virtual bool Extract(size_t pos, std::deque<Date>& val);

  /**
   * Extracts a Date list.
   */
  virtual bool Extract(size_t pos, std::list<Date>& val);

  /**
   * Extracts a Time. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, Time& val) = 0;

  /**
   * Extracts a Time vector.
   */
  virtual bool Extract(size_t pos, std::vector<Time>& val);

  /**
   * Extracts a Time deque.
   */
  virtual bool Extract(size_t pos, std::deque<Time>& val);

  /**
   * Extracts a Time list.
   */
  virtual bool Extract(size_t pos, std::list<Time>& val);

  /**
   * Extracts an Any. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, Any& val) = 0;

  /**
   * Extracts an Any vector.
   */
  virtual bool Extract(size_t pos, std::vector<Any>& val);

  /**
   * Extracts an Any deque.
   */
  virtual bool Extract(size_t pos, std::deque<Any>& val);

  /**
   * Extracts an Any list.
   */
  virtual bool Extract(size_t pos, std::list<Any>& val);

  /**
   * Extracts a Var. Returns false if null was received.
   */
  virtual bool Extract(size_t pos, fun::dynamic::Var& val) = 0;

  /**
   * Extracts a Var vector.
   */
  virtual bool Extract(size_t pos, std::vector<fun::dynamic::Var>& val);

  /**
   * Extracts a Var deque.
   */
  virtual bool Extract(size_t pos, std::deque<fun::dynamic::Var>& val);

  /**
   * Extracts a Var list.
   */
  virtual bool Extract(size_t pos, std::list<fun::dynamic::Var>& val);


  /**
   * Returns true if the value at [col,row] position is null.
   */
  virtual bool IsNull(size_t col, size_t row = FUN_DATA_INVALID_ROW) = 0;

  /**
   * Resets any information internally cached by the extractor.
   */
  virtual void Reset();
};


//
// inlines
//

inline void ExtractorBase::Reset() {
  // NOOP BY DEFAULT
}

} // namespace sql
} // namespace fun
