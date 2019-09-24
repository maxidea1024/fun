#pragma once

#include "fun/sql/postgresql/postgresql.h"
#include "fun/sql/postgresql/postgresql_types.h"
#include "fun/sql/postgresql/statement_executor.h"

#include "fun/sql/extractor_base.h"
#include "fun/sql/lob.h"

#include "fun/base/any.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/dynamic_any.h"
#include "fun/base/types.h"

namespace fun {

// namespace Dynamic {
//  class Var;
//}

namespace sql {
namespace postgresql {

/**
 * Extracts and converts data values from the result row returned by PostgreSQL.
 * If NULL is received, the incoming val value is not changed and false is
 * returned
 */
class FUN_POSTGRESQL_API Extractor : public fun::sql::ExtractorBase {
 public:
  typedef SharedPtr<Extractor> Ptr;

  /**
   * Creates the Extractor.
   */
  Extractor(StatementExecutor& executor);

  /**
   * Destroys the Extractor.
   */
  virtual ~Extractor();

  virtual bool Extract(size_t pos, int8& val);
  virtual bool Extract(size_t pos, uint8& val);
  virtual bool Extract(size_t pos, int16& val);
  virtual bool Extract(size_t pos, uint16& val);
  virtual bool Extract(size_t pos, int32& val);
  virtual bool Extract(size_t pos, uint32& val);
  virtual bool Extract(size_t pos, int64& val);
  virtual bool Extract(size_t pos, uint64& val);

#ifndef FUN_LONG_IS_64_BIT
  virtual bool Extract(size_t pos, long& val);
  virtual bool Extract(size_t pos, unsigned long& val);
#endif

  virtual bool Extract(size_t pos, bool& val);
  virtual bool Extract(size_t pos, float& val);
  virtual bool Extract(size_t pos, double& val);
  virtual bool Extract(size_t pos, char& val);
  virtual bool Extract(size_t pos, String& val);
  virtual bool Extract(size_t pos, fun::sql::BLOB& val);
  virtual bool Extract(size_t pos, fun::sql::CLOB& val);
  virtual bool Extract(size_t pos, DateTime& val);
  virtual bool Extract(size_t pos, Date& val);
  virtual bool Extract(size_t pos, Time& val);
  virtual bool Extract(size_t pos, Any& val);
  virtual bool Extract(size_t pos, Dynamic::Var& val);

  /**
   * Returns true if the value at [col,row] position is null.
   */
  virtual bool IsNull(size_t col, size_t row);

  /**
   * Resets any information internally cached by the extractor.
   */
  virtual void Reset();

  //
  // Not implemented Extract functions
  //

  virtual bool Extract(size_t pos, std::vector<int8>& val);
  virtual bool Extract(size_t pos, std::deque<int8>& val);
  virtual bool Extract(size_t pos, std::list<int8>& val);
  virtual bool Extract(size_t pos, std::vector<uint8>& val);
  virtual bool Extract(size_t pos, std::deque<uint8>& val);
  virtual bool Extract(size_t pos, std::list<uint8>& val);
  virtual bool Extract(size_t pos, std::vector<int16>& val);
  virtual bool Extract(size_t pos, std::deque<int16>& val);
  virtual bool Extract(size_t pos, std::list<int16>& val);
  virtual bool Extract(size_t pos, std::vector<uint16>& val);
  virtual bool Extract(size_t pos, std::deque<uint16>& val);
  virtual bool Extract(size_t pos, std::list<uint16>& val);
  virtual bool Extract(size_t pos, std::vector<int32>& val);
  virtual bool Extract(size_t pos, std::deque<int32>& val);
  virtual bool Extract(size_t pos, std::list<int32>& val);
  virtual bool Extract(size_t pos, std::vector<uint32>& val);
  virtual bool Extract(size_t pos, std::deque<uint32>& val);
  virtual bool Extract(size_t pos, std::list<uint32>& val);
  virtual bool Extract(size_t pos, std::vector<int64>& val);
  virtual bool Extract(size_t pos, std::deque<int64>& val);
  virtual bool Extract(size_t pos, std::list<int64>& val);
  virtual bool Extract(size_t pos, std::vector<uint64>& val);
  virtual bool Extract(size_t pos, std::deque<uint64>& val);
  virtual bool Extract(size_t pos, std::list<uint64>& val);

#ifndef FUN_LONG_IS_64_BIT
  virtual bool Extract(size_t pos, std::vector<long>& val);
  virtual bool Extract(size_t pos, std::deque<long>& val);
  virtual bool Extract(size_t pos, std::list<long>& val);
#endif

  virtual bool Extract(size_t pos, std::vector<bool>& val);
  virtual bool Extract(size_t pos, std::deque<bool>& val);
  virtual bool Extract(size_t pos, std::list<bool>& val);
  virtual bool Extract(size_t pos, std::vector<float>& val);
  virtual bool Extract(size_t pos, std::deque<float>& val);
  virtual bool Extract(size_t pos, std::list<float>& val);
  virtual bool Extract(size_t pos, std::vector<double>& val);
  virtual bool Extract(size_t pos, std::deque<double>& val);
  virtual bool Extract(size_t pos, std::list<double>& val);
  virtual bool Extract(size_t pos, std::vector<char>& val);
  virtual bool Extract(size_t pos, std::deque<char>& val);
  virtual bool Extract(size_t pos, std::list<char>& val);
  virtual bool Extract(size_t pos, std::vector<String>& val);
  virtual bool Extract(size_t pos, std::deque<String>& val);
  virtual bool Extract(size_t pos, std::list<String>& val);
  virtual bool Extract(size_t pos, std::vector<BLOB>& val);
  virtual bool Extract(size_t pos, std::deque<BLOB>& val);
  virtual bool Extract(size_t pos, std::list<BLOB>& val);
  virtual bool Extract(size_t pos, std::vector<CLOB>& val);
  virtual bool Extract(size_t pos, std::deque<CLOB>& val);
  virtual bool Extract(size_t pos, std::list<CLOB>& val);
  virtual bool Extract(size_t pos, std::vector<DateTime>& val);
  virtual bool Extract(size_t pos, std::deque<DateTime>& val);
  virtual bool Extract(size_t pos, std::list<DateTime>& val);
  virtual bool Extract(size_t pos, std::vector<Date>& val);
  virtual bool Extract(size_t pos, std::deque<Date>& val);
  virtual bool Extract(size_t pos, std::list<Date>& val);
  virtual bool Extract(size_t pos, std::vector<Time>& val);
  virtual bool Extract(size_t pos, std::deque<Time>& val);
  virtual bool Extract(size_t pos, std::list<Time>& val);
  virtual bool Extract(size_t pos, std::vector<Any>& val);
  virtual bool Extract(size_t pos, std::deque<Any>& val);
  virtual bool Extract(size_t pos, std::list<Any>& val);
  virtual bool Extract(size_t pos, std::vector<Dynamic::Var>& val);
  virtual bool Extract(size_t pos, std::deque<Dynamic::Var>& val);
  virtual bool Extract(size_t pos, std::list<Dynamic::Var>& val);

 private:
  const OutputParameter& ExtractPreamble(size_t position) const;

  bool IsColumnNull(const OutputParameter& output_param) const;

  /**
   * Utility function for extraction of Any and DynamicAny.
   */
  template <typename T>
  bool ExtractStringImpl(size_t pos, T& val) {
    OutputParameter output_param = ExtractPreamble(pos);

    if (IsColumnNull(output_param)) {
      return false;
    }

    String tmp;  // since the postgreSQL API in use is all about strings...
    bool ret = Extract(pos, tmp);
    if (ret) {
      val = tmp;
    }

    return ret;
  }

  // Prevent VC8 warning "operator= could not be generated"
  Extractor& operator=(const Extractor&);

 private:
  StatementExecutor& statement_executor_;
};

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
