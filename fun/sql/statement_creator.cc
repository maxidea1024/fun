#include "fun/sql/statement_creator.h"
#include <algorithm>

namespace fun {
namespace sql {

StatementCreator::StatementCreator() {}

StatementCreator::StatementCreator(SessionImpl::Ptr session_impl)
    : session_impl_(session_impl) {}

StatementCreator::StatementCreator(const StatementCreator& other)
    : session_impl_(other.session_impl_) {}

StatementCreator& StatementCreator::operator=(const StatementCreator& other) {
  if (&other != this) {
    StatementCreator tmp(other);
    Swap(tmp);
  }
  return *this;
}

StatementCreator& StatementCreator::operator=(
    fun::RefCountedPtr<SessionImpl> session_impl) {
  session_impl_ = session_impl;
  return *this;
}

void StatementCreator::Swap(StatementCreator& other) {
  fun::Swap(session_impl_, other.session_impl_);
}

StatementCreator::~StatementCreator() {}

}  // namespace sql
}  // namespace fun
