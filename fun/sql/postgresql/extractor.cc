#include "fun/sql/postgresql/Extractor.h"
#include "fun/base/date_time_parser.h"
#include "fun/base/number_parser.h"
#include "fun/sql/date.h"
#include "fun/sql/time.h"

#include <limits>

namespace fun {
namespace sql {
namespace postgresql {

Extractor::Extractor(StatementExecutor& st /*, ResultMetadata& md */)
    : statement_executor_(st) {}

Extractor::~Extractor() {}

bool Extractor::Extract(size_t pos, int8& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  int tmp = 0;

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::TryParse(output_param.pData(), tmp)) {
    return false;
  }

  val = static_cast<int8>(tmp);

  return true;
}

bool Extractor::Extract(size_t pos, uint8& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  unsigned int tmp = 0;

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParseUnsigned(output_param.pData(), tmp)) {
    return false;
  }

  val = static_cast<int8>(tmp);

  return true;
}

bool Extractor::Extract(size_t pos, int16& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  int tmp = 0;

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::TryParse(output_param.pData(), tmp)) {
    return false;
  }

  val = static_cast<int8>(tmp);

  return true;
}

bool Extractor::Extract(size_t pos, uint16& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  unsigned int tmp = 0;

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParseUnsigned(output_param.pData(), tmp)) {
    return false;
  }

  val = static_cast<int8>(tmp);

  return true;
}

bool Extractor::Extract(size_t pos, int32& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::TryParse(output_param.pData(), val)) {
    return false;
  }

  return true;
}

bool Extractor::Extract(size_t pos, uint32& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParseUnsigned(output_param.pData(), val)) {
    return false;
  }

  return true;
}

bool Extractor::Extract(size_t pos, int64& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParse64(output_param.pData(), val)) {
    return false;
  }

  return true;
}

bool Extractor::Extract(size_t pos, uint64& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParseUnsigned64(output_param.pData(), val)) {
    return false;
  }

  return true;
}

#ifndef FUN_LONG_IS_64_BIT
bool Extractor::Extract(size_t pos, long& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  int64 tmp = 0;

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParse64(output_param.pData(), tmp)) {
    return false;
  }

  val = (long)tmp;

  return true;
}

bool Extractor::Extract(size_t pos, unsigned long& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  uint64 tmp = 0;

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParseUnsigned64(output_param.pData(), tmp)) {
    return false;
  }

  val = (unsigned long)tmp;

  return true;
}
#endif

bool Extractor::Extract(size_t pos, bool& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  if ('t' == *output_param.pData()) {
    val = true;
  } else {
    val = false;
  }

  return true;
}

bool Extractor::Extract(size_t pos, float& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  double tmp = 0.0;

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParseFloat(output_param.pData(), tmp)) {
    return false;
  }

  val = (float)tmp;

  return true;
}

bool Extractor::Extract(size_t pos, double& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param) ||
      !fun::NumberParser::tryParseFloat(output_param.pData(), val)) {
    return false;
  }

  return true;
}

bool Extractor::Extract(size_t pos, char& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  val = *output_param.pData();

  return true;
}

bool Extractor::Extract(size_t pos, String& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  val.Assign(output_param.pData(), output_param.size());

  return true;
}

bool Extractor::Extract(size_t pos, fun::sql::BLOB& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  // convert the PostgreSQL text format to binary and append to the BLOB
  // Format: \x10843029479abcf ...  two characters for every byte
  //
  //  The code below can be made more efficient by converting more than one byte
  //  at a time also if BLOB had a resize method it would be useful to allocate
  //  memory in one attempt.
  //

  const char* pBLOB = reinterpret_cast<const char*>(output_param.pData());
  size_t BLOBSize = output_param.size();

  if ('\\' == pBLOB[0] &&
      'x' == pBLOB[1]  // preamble to BYTEA data format in text form is \x
  ) {
    BLOBSize -= 2;  // lose the preamble
    BLOBSize /= 2;  // each byte is encoded as two text characters

    for (int i = 0; i < BLOBSize * 2; i += 2) {
      String buffer(&pBLOB[i + 2], 2);
      unsigned int binaryBuffer = 0;
      if (fun::NumberParser::tryParseHex(buffer, binaryBuffer)) {
        uint8 finalBinaryBuffer = static_cast<uint8>(binaryBuffer);  // downsize
        val.AppendRaw(&finalBinaryBuffer, 1);
      }
    }
  }
  return true;
}

bool Extractor::Extract(size_t pos, fun::sql::CLOB& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  val.AssignRaw(output_param.pData(), output_param.size());

  return true;
}

bool Extractor::Extract(size_t pos, DateTime& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  int tzd = -1;
  DateTime datetime;

  if (!DateTimeParser::TryParse(output_param.pData(), datetime, tzd)) {
    return false;
  }

  datetime.makeUTC(tzd);

  val = datetime;

  return true;
}

bool Extractor::Extract(size_t pos, Date& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  int tzd = -1;
  DateTime datetime;

  if (!DateTimeParser::TryParse(output_param.pData(), datetime, tzd)) {
    return false;
  }

  datetime.makeUTC(tzd);

  val.Assign(datetime.Year(), datetime.Month(), datetime.Day());

  return true;
}

bool Extractor::Extract(size_t pos, Time& val) {
  OutputParameter output_param = ExtractPreamble(pos);

  if (IsColumnNull(output_param)) {
    return false;
  }

  int tzd = -1;
  DateTime datetime;

  if (!DateTimeParser::TryParse("%H:%M:%s%z", output_param.pData(), datetime,
                                tzd)) {
    return false;
  }

  // datetime.makeUTC(tzd); // TODO
  // Note: fun::sql::Time should be extended to support the fractional
  // components of fun::DateTime

  val.Assign(datetime.Hour(), datetime.Minute(), datetime.Second());

  return true;
}

bool Extractor::Extract(size_t pos, Any& val) {
  return ExtractStringImpl(pos, val);
}

bool Extractor::Extract(size_t pos, Dynamic::Var& val) {
  return ExtractStringImpl(pos, val);
}

bool Extractor::IsNull(size_t col, size_t /*row*/) {
  OutputParameter output_param = ExtractPreamble(col);

  if (IsColumnNull(output_param)) {
    return true;
  }

  return false;
}

void Extractor::Reset() { ExtractorBase::Reset(); }

const OutputParameter& Extractor::ExtractPreamble(size_t position) const {
  if (statement_executor_.ReturnedColumnCount() <= position) {
    throw PostgreSqlException(
        "Extractor: attempt to Extract more parameters than query result "
        "contains");
  }

  return statement_executor_.resultColumn(position);
}

bool Extractor::IsColumnNull(const OutputParameter& output_param) const {
  return output_param.IsNull() || 0 == output_param.pData();
}

//
// Not implemented
//

bool Extractor::Extract(size_t, std::vector<int8>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int8>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int8>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint8>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint8>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint8>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<int16>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int16>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int16>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint16>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint16>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint16>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<int32>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int32>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int32>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint32>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint32>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint32>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<int64>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<int64>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<int64>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<uint64>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<uint64>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<uint64>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

#ifndef FUN_LONG_IS_64_BIT
bool Extractor::Extract(size_t, std::vector<long>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<long>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<long>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}
#endif

bool Extractor::Extract(size_t, std::vector<bool>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<bool>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<bool>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<float>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<float>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<float>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<double>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<double>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<double>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<char>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<char>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<char>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<String>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<String>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<String>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<BLOB>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<BLOB>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<BLOB>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<CLOB>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<CLOB>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<CLOB>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<DateTime>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<DateTime>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<DateTime>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Date>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Date>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Date>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Time>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Time>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Time>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Any>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Any>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Any>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::vector<Dynamic::Var>&) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::deque<Dynamic::Var>&) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool Extractor::Extract(size_t, std::list<Dynamic::Var>&) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
