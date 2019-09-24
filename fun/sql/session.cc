#include "fun/sql/session.h"
#include <algorithm>
#include "fun/base/string.h"
#include "fun/sql/session_factory.h"
#include "fun/uri.h"

namespace fun {
namespace sql {

Session::Session(fun::RefCountedPtr<SessionImpl> impl)
    : impl_(impl), statement_creator_(impl) {
  fun_check_ptr(impl.Get());
}

Session::Session(const String& connector, const String& connection_string,
                 size_t timeout) {
  operator=(
      SessionFactory::instance().create(connector, connection_string, timeout));
}

Session::Session(const String& connection, size_t timeout) {
  operator=(SessionFactory::Instance().Create(connection, timeout));
}

Session::Session(const Session& other)
    : impl_(other.impl_), statement_creator_(other.impl_) {}

Session::Session(Session&& other)
    : impl_(other.impl_), statement_creator_(other.impl_) {
  other.impl_ = nullptr;
}

Session::~Session() {
  if (impl_) {
    impl_->PutBack();
  }
}

Session& Session::operator=(const Session& other) {
  if (&other != this) {
    Session tmp(other);
    Swap(tmp);
  }
  return *this;
}

Session& Session::operator=(Session&& other) {
  if (&other != this) {
    impl_ = other.impl_;
    statement_creator_ = impl_;
    other.impl_ = nullptr;
  }
  return *this;
}

void Session::Swap(Session& other) {
  Swap(statement_creator_, other.statement_creator_);
  Swap(impl_, other.impl_);
}

}  // namespace sql
}  // namespace fun
