#include "fun/sql/preparator_base.h"

namespace fun {
namespace sql {

PreparatorBase::PreparatorBase(uint32 length)
  : length_(length), bulk_(false) {}

PreparatorBase::~PreparatorBase() {}

void PreparatorBase::Prepare(size_t pos, const std::vector<int8>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<int8>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<int8>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<uint8>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<uint8>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<uint8>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<int16>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<int16>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<int16>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<uint16>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<uint16>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<uint16>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<int32>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<int32>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<int32>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<uint32>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<uint32>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<uint32>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<int64>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<int64>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<int64>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<uint64>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<uint64>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<uint64>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

#ifndef FUN_LONG_IS_64_BIT
void PreparatorBase::Prepare(size_t pos, const std::vector<long>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<long>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<long>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}
#endif

void PreparatorBase::Prepare(size_t pos, const std::vector<bool>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<bool>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<bool>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<float>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<float>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<float>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<double>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<double>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<double>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<char>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<char>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<char>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<String>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<String>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<String>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const UString& val) {
  throw NotImplementedException("UString preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<UString>& val) {
  throw NotImplementedException("std::vector<UString> preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<UString>& val) {
  throw NotImplementedException("std::deque<UString> preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<UString>& val) {
  throw NotImplementedException("std::list<UString> preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<BLOB>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<BLOB>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<BLOB>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<CLOB>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<CLOB>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<CLOB>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<DateTime>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<DateTime>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<DateTime>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<Date>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<Date>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<Date>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<Time>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<Time>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<Time>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<Any>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<Any>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<Any>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::vector<fun::dynamic::Var>& val) {
  throw NotImplementedException("std::vector preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::deque<fun::dynamic::Var>& val) {
  throw NotImplementedException("std::deque preparator must be implemented.");
}

void PreparatorBase::Prepare(size_t pos, const std::list<fun::dynamic::Var>& val) {
  throw NotImplementedException("std::list preparator must be implemented.");
}

} // namespace sql
} // namespace fun
