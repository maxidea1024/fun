#include "fun/sql/session_impl.h"
#include "fun/base/exception.h"

namespace fun {
namespace sql {

SessionImpl::SessionImpl(const String& connection_string, size_t timeout)
  : connection_string_(connection_string),
    login_timeout_(timeout) {}

SessionImpl::~SessionImpl() {}

void SessionImpl::Reconnect() {
  Close();

  Open();
}

void SessionImpl::SetConnectionString(const String& connection_string) {
  if (IsConnected()) {
    throw fun::InvalidAccessException("Can not change connection string on connected session."
                                      " Close the session first.");
  }

  connection_string_ = connection_string;
}

} // namespace sql
} // namespace fun
