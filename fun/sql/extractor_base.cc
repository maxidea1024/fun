#include "fun/sql/extractor_base.h"
#include "fun/base/exception.h"

namespace fun {
namespace sql {

ExtractorBase::ExtractorBase() {}

ExtractorBase::~ExtractorBase() {}

bool ExtractorBase::Extract(size_t pos, std::vector<int8>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<int8>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<int8>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<uint8>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<uint8>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<uint8>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<int16>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<int16>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<int16>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<uint16>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<uint16>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<uint16>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<int32>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<int32>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<int32>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<uint32>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<uint32>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<uint32>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<int64>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<int64>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<int64>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<uint64>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<uint64>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<uint64>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

#ifndef FUN_LONG_IS_64_BIT
bool ExtractorBase::Extract(size_t pos, std::vector<long>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<long>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<long>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}
#endif

bool ExtractorBase::Extract(size_t pos, std::vector<bool>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<bool>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<bool>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<float>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<float>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<float>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<double>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<double>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<double>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<char>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<char>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<char>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<String>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<String>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<String>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, UString& val) {
  throw NotImplementedException("UString extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<UString>& val) {
  throw NotImplementedException("std::vector<UString> extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<UString>& val) {
  throw NotImplementedException("std::deque<UString> extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<UString>& val) {
  throw NotImplementedException("std::list<UString> extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<BLOB>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<BLOB>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<BLOB>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<CLOB>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<CLOB>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<CLOB>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<DateTime>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<DateTime>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<DateTime>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<Date>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<Date>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<Date>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<Time>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<Time>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<Time>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<Any>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<Any>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<Any>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::vector<fun::dynamic::Var>& val) {
  throw NotImplementedException("std::vector extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::deque<fun::dynamic::Var>& val) {
  throw NotImplementedException("std::deque extractor must be implemented.");
}

bool ExtractorBase::Extract(size_t pos, std::list<fun::dynamic::Var>& val) {
  throw NotImplementedException("std::list extractor must be implemented.");
}

} // namespace sql
} // namespace fun
