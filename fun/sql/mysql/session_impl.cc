#include "fun/sql/mysql/session_impl.h"
#include "fun/base/number_parser.h"
#include "fun/base/string.h"
#include "fun/sql/mysql/mysql_statement_impl.h"
#include "fun/sql/session.h"

namespace {

String CopyStripped(String::const_iterator from, String::const_iterator to) {
  // skip leading spaces
  while ((from != to) && isspace(*from)) from++;
  // skip trailing spaces
  while ((from != to) && isspace(*(to - 1))) to--;

  return String(from, to);
}

}  // namespace

namespace fun {
namespace sql {
namespace mysql {

const String SessionImpl::MYSQL_READ_UNCOMMITTED = "READ UNCOMMITTED";
const String SessionImpl::MYSQL_READ_COMMITTED = "READ COMMITTED";
const String SessionImpl::MYSQL_REPEATABLE_READ = "REPEATABLE READ";
const String SessionImpl::MYSQL_SERIALIZABLE = "SERIALIZABLE";

SessionImpl::SessionImpl(const String& connection_string, size_t login_timeout)
    : fun::sql::SessionImplBase<SessionImpl>(connection_string, login_timeout),
      connector_("MySQL"),
      handle_(0),
      connected_(false),
      in_transaction_(false) {
  AddProperty("insertId", &SessionImpl::SetInsertId, &SessionImpl::GetInsertId);
  SetProperty("handle", static_cast<MYSQL*>(handle_));
  Open();
}

void SessionImpl::Open(const String& connect) {
  if (connect != GetConnectionString()) {
    if (IsConnected()) {
      throw InvalidAccessException("Session already connected");
    }

    if (!connect.IsEmpty()) {
      SetConnectionString(connect);
    }
  }

  fun_check_dbg(!GetConnectionString().IsEmpty());

  handle_.Init();

  unsigned int timeout = static_cast<unsigned int>(GetLoginTimeout());
  handle_.options(MYSQL_OPT_CONNECT_TIMEOUT, timeout);

  std::map<String, String> options;

  // Default values
  options["host"] = "localhost";
  options["port"] = "3306";
  options["user"] = "";
  options["password"] = "";
  options["db"] = "";
  options["compress"] = "";
  options["auto-Reconnect"] = "";
  options["secure-auth"] = "";
  options["character-set"] = "utf8";

  const String& conn_string = GetConnectionString();
  for (String::const_iterator start = conn_string.begin();;) {
    String::const_iterator finish = std::find(start, conn_string.end(), ';');
    String::const_iterator middle = std::find(start, finish, '=');

    if (middle == finish) {
      throw MySqlException(
          "create session: bad connection string format, can not find '='");
    }

    options[CopyStripped(start, middle)] = CopyStripped(middle + 1, finish);

    if ((finish == conn_string.end()) || (finish + 1 == conn_string.end())) {
      break;
    }

    start = finish + 1;
  }

  if (options["user"].IsEmpty()) {
    throw MySqlException("create session: specify user name");
  }

  const char* db = nullptr;
  if (!options["db"].IsEmpty()) {
    db = options["db"].c_str();
  }

  unsigned int port = 0;
  if (!NumberParser::tryParseUnsigned(options["port"], port) || 0 == port ||
      port > 65535) {
    throw MySqlException(
        "create session: specify correct port (numeric in decimal notation)");
  }

  if (options["compress"] == "true") {
    handle_.options(MYSQL_OPT_COMPRESS);
  } else if (options["compress"] == "false") {
    //
  } else if (!options["compress"].IsEmpty()) {
    throw MySqlException(
        "create session: specify correct compress option (true or false) or "
        "skip it");
  }

  if (options["auto-Reconnect"] == "true") {
    handle_.options(MYSQL_OPT_RECONNECT, true);
  } else if (options["auto-Reconnect"] == "false") {
    handle_.options(MYSQL_OPT_RECONNECT, false);
  } else if (!options["auto-Reconnect"].IsEmpty()) {
    throw MySqlException(
        "create session: specify correct auto-Reconnect option (true or false) "
        "or skip it");
  }

  if (options["secure-auth"] == "true") {
    handle_.options(MYSQL_SECURE_AUTH, true);
  } else if (options["secure-auth"] == "false") {
    handle_.options(MYSQL_SECURE_AUTH, false);
  } else if (!options["secure-auth"].IsEmpty()) {
    throw MySqlException(
        "create session: specify correct secure-auth option (true or false) or "
        "skip it");
  }

  if (!options["character-set"].IsEmpty()) {
    handle_.options(MYSQL_SET_CHARSET_NAME, options["character-set"].c_str());
  }

  // Real connect
  handle_.Connect(options["host"].c_str(), options["user"].c_str(),
                  options["password"].c_str(), db, port);

  AddFeature("autoCommit", &SessionImpl::SetAutoCommit,
             &SessionImpl::IsAutoCommit);

  connected_ = true;
}

SessionImpl::~SessionImpl() { close(); }

StatementImpl::Ptr SessionImpl::CreateStatementImpl() {
  return new MySqlStatementImpl(*this);
}

void SessionImpl::Begin() {
  fun::FastMutex::ScopedLock l(mutex_);

  if (in_transaction_) {
    throw fun::InvalidAccessException("Already in transaction.");
  }

  handle_.StartTransaction();
  in_transaction_ = true;
}

void SessionImpl::Commit() {
  handle_.Commit();
  in_transaction_ = false;
}

void SessionImpl::Rollback() {
  handle_.Rollback();
  in_transaction_ = false;
}

void SessionImpl::SetAutoCommit(const String&, bool val) {
  StatementExecutor ex(handle_);
  ex.Prepare(fun::Format("SET autocommit=%d", val ? 1 : 0));
  ex.Execute();
}

bool SessionImpl::IsAutoCommit(const String&) const {
  int ac = 0;
  return 1 == GetSetting("autocommit", ac);
}

void SessionImpl::SetTransactionIsolation(uint32 ti) {
  String isolation;
  switch (ti) {
    case Session::TRANSACTION_READ_UNCOMMITTED:
      isolation = MYSQL_READ_UNCOMMITTED;
      break;
    case Session::TRANSACTION_READ_COMMITTED:
      isolation = MYSQL_READ_COMMITTED;
      break;
    case Session::TRANSACTION_REPEATABLE_READ:
      isolation = MYSQL_REPEATABLE_READ;
      break;
    case Session::TRANSACTION_SERIALIZABLE:
      isolation = MYSQL_SERIALIZABLE;
      break;
    default:
      throw fun::InvalidArgumentException("SetTransactionIsolation()");
  }

  StatementExecutor ex(handle_);
  ex.Prepare(
      fun::Format("SET SESSION TRANSACTION ISOLATION LEVEL %s", isolation));
  ex.Execute();
}

uint32 SessionImpl::GetTransactionIsolation() const {
  String isolation;
  GetSetting("tx_isolation", isolation);
  fun::replaceInPlace(isolation, "-", " ");
  if (MYSQL_READ_UNCOMMITTED == isolation) {
    return Session::TRANSACTION_READ_UNCOMMITTED;
  } else if (MYSQL_READ_COMMITTED == isolation) {
    return Session::TRANSACTION_READ_COMMITTED;
  } else if (MYSQL_REPEATABLE_READ == isolation) {
    return Session::TRANSACTION_REPEATABLE_READ;
  } else if (MYSQL_SERIALIZABLE == isolation) {
    return Session::TRANSACTION_SERIALIZABLE;
  }

  throw InvalidArgumentException("GetTransactionIsolation()");
}

bool SessionImpl::HasTransactionIsolation(uint32 ti) const {
  return Session::TRANSACTION_READ_UNCOMMITTED == ti ||
         Session::TRANSACTION_READ_COMMITTED == ti ||
         Session::TRANSACTION_REPEATABLE_READ == ti ||
         Session::TRANSACTION_SERIALIZABLE == ti;
}

void SessionImpl::Close() {
  if (connected_) {
    handle_.Close();
    connected_ = false;
  }
}

void SessionImpl::Reset() {
  if (connected_) {
    handle_.Reset();
  }
}

void SessionImpl::SetConnectionTimeout(size_t timeout) {
  handle_.SetOptions(MYSQL_OPT_READ_TIMEOUT,
                     static_cast<unsigned int>(timeout));
  handle_.SetOptions(MYSQL_OPT_WRITE_TIMEOUT,
                     static_cast<unsigned int>(timeout));
  timeout_ = timeout;
}

}  // namespace mysql
}  // namespace sql
}  // namespace fun
