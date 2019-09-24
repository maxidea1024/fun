#include "fun/sql/postgresql/session_impl.h"
#include "fun/sql/postgresql/postgresql_exception.h"
#include "fun/sql/postgresql/postgresql_statement_impl.h"
#include "fun/sql/postgresql/postgresql_types.h"
#include "fun/sql/session.h"
#include "fun/base/number_parser.h"
#include "fun/base/string.h"

#include <map>

namespace {

String CopyStripped(String::const_iterator aFromStringCItr, String::const_iterator aToStringCItr) {
  // skip leading spaces
  while ((aFromStringCItr != aToStringCItr) && isspace(*aFromStringCItr)) aFromStringCItr++;
  // skip trailing spaces
  while ((aFromStringCItr != aToStringCItr) && isspace(*(aToStringCItr - 1))) aToStringCItr--;

  return String(aFromStringCItr, aToStringCItr);
}

String CreateConnectionStringFromOptionsMap(const std::map <String, String> options_map) {
  String connection_string;

  for (std::map<String, String>::const_iterator citr = options_map.begin(); citr != options_map.end(); ++citr) {
    connection_string.Append(citr->first);
    connection_string.Append("=");
    connection_string.Append(citr->second);
    connection_string.Append(" ");
  }

  return connection_string;
}

} // namespace

namespace fun {
namespace sql {
namespace postgresql {

SessionImpl::SessionImpl(const String& connection_string, size_t login_timeout)
  : fun::sql::SessionImplBase<SessionImpl>(connection_string, login_timeout) {
  SetFeature("bulk", true);
  SetProperty("handle", static_cast<SessionHandle*>(&session_handle_));
  SetConnectionTimeout(CONNECTION_TIMEOUT_DEFAULT);
  Open();
}

SessionImpl::~SessionImpl() {
  try {
    Close();
  } catch (...) {}
}

void SessionImpl::SetConnectionTimeout(size_t timeout) {
  timeout_ = timeout;
}

void SessionImpl::Open(const String& connection_string) {
  if (GetConnectionString() != connection_string) {
    if (IsConnected()) {
      throw ConnectionException("Session already connected");
    }

    if (!connection_string.IsEmpty()) {
      SetConnectionString(connection_string);
    }
  }

  fun_check_dbg(!GetConnectionString().IsEmpty());

  unsigned int timeout = static_cast<unsigned int>(GetLoginTimeout());

  // PostgreSQL connections can use environment variables for connection parameters.
  // As such it is not an error if they are not part of the connection string

  std::map <String, String> options_map;

  // Default values
  options_map["connect_timeout"] = fun::NumberFormatter::Format(timeout);

  const String& conn_string = GetConnectionString();

  for (String::const_iterator start = conn_string.begin();;) {
    String::const_iterator finish = std::find(start, conn_string.end(), ' '); // space is the separator between keyword=value pairs
    String::const_iterator middle = std::find(start, finish, '=');

    if (middle == finish) {
      throw PostgreSqlException("create session: bad connection string format, cannot find '='");
    }

    options_map[CopyStripped(start, middle)] = CopyStripped(middle + 1, finish);

    if ((finish == conn_string.end()) || (finish + 1 == conn_string.end())) {
      break;
    }

    start = finish + 1;
  }

  // Real connect
  session_handle_.Connect(CreateConnectionStringFromOptionsMap(options_map));

  AddFeature("autoCommit",
    &SessionImpl::SetAutoCommit,
    &SessionImpl::IsAutoCommit);

  AddFeature("asynchronousCommit",
    &SessionImpl::SetAutoCommit,
    &SessionImpl::IsAutoCommit);
}

void SessionImpl::Close() {
  if (IsConnected()) {
    session_handle_.Disconnect();
  }
}

void SessionImpl::Reset() {
}

bool SessionImpl::IsConnected() const {
  return session_handle_.IsConnected();
}

StatementImpl::Ptr SessionImpl::CreateStatementImpl() {
  return new PostgreSqlStatementImpl(*this);
}

bool SessionImpl::IsInTransaction() const {
  return session_handle_.IsInTransaction();
}

void SessionImpl::Begin() {
  if (IsInTransaction()) {
    throw fun::InvalidAccessException("Already in transaction.");
  }

  session_handle_.StartTransaction();
}

void SessionImpl::Commit() {
  // Not an error to issue a COMMIT without a preceding BEGIN
  session_handle_.Commit();
}

void SessionImpl::Rollback() {
  // Not an error to issue a ROLLBACK without a preceding BEGIN
  session_handle_.Rollback();
}

void SessionImpl::SetAutoCommit(const String&, bool value) {
  session_handle_.SetAutoCommit(value);
}

bool SessionImpl::IsAutoCommit(const String&) const {
  return session_handle_.IsAutoCommit();
}

void SessionImpl::SetAsynchronousCommit(const String&,  bool value) {
  session_handle_.SetAsynchronousCommit(value);
}

bool SessionImpl::IsAsynchronousCommit(const String&) const {
  return session_handle_.IsAsynchronousCommit();
}

void SessionImpl::SetTransactionIsolation(uint32 ti) {
  return session_handle_.SetTransactionIsolation(ti);
}

uint32 SessionImpl::GetTransactionIsolation() const {
  return session_handle_.transactionIsolation();
}

bool SessionImpl::HasTransactionIsolation(uint32 ti) const {
  return session_handle_.HasTransactionIsolation(ti);
}

} // namespace postgresql
} // namespace sql
} // namespace fun
