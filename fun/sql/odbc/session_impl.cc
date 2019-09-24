#include "fun/sql/odbc/session_impl.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/odbc/odbc_statement_impl.h"
#include "fun/sql/odbc/error.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/session.h"
#include "fun/base/string.h"
#include <sqlext.h>

namespace fun {
namespace sql {
namespace odbc {

SessionImpl::SessionImpl(const String& connect,
                        size_t login_timeout,
                        size_t maxFieldSize,
                        bool autoBind,
                        bool autoExtract,
                        bool MakeExtractorsBeforeExecute)
  : fun::sql::SessionImplBase<SessionImpl>(connect, login_timeout),
    connector_(Connector::KEY),
    max_field_size_(maxFieldSize),
    auto_bind_(autoBind),
    auto_extract_(autoExtract),
    make_extractor_before_execute_(MakeExtractorsBeforeExecute),
    can_transact_(ODBC_TXN_CAPABILITY_UNKNOWN),
    in_transaction_(false),
    query_timeout_(-1) {
  Init();
}

SessionImpl::SessionImpl( const String& connect,
                          fun::Any maxFieldSize,
                          bool /*enforceCapability*/,
                          bool autoBind,
                          bool autoExtract,
                          bool MakeExtractorsBeforeExecute)
  : fun::sql::SessionImplBase<SessionImpl>(connect),
    connector_(Connector::KEY),
    max_field_size_(maxFieldSize),
    auto_bind_(autoBind),
    auto_extract_(autoExtract),
    make_extractor_before_execute_(MakeExtractorsBeforeExecute),
    can_transact_(ODBC_TXN_CAPABILITY_UNKNOWN),
    in_transaction_(false),
    query_timeout_(-1) {
  Init();
}

void SessionImpl::Init() {
  SetFeature("bulk", true);
  Open();
  SetProperty("handle", db_.GetHandle());
  SetProperty("handle", db_.GetHandle());
}

SessionImpl::~SessionImpl() {
  try {
    if (static_cast<bool>(db_) && IsInTransaction() && !GetFeature("autoCommit")) {
      try { Rollback(); }
      catch (...) { }
    }

    Close();
  } catch (...) {
    fun_unexpected();
  }
}

fun::sql::StatementImpl::Ptr SessionImpl::CreateStatementImpl() {
  return new OdbcStatementImpl(*this);
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

  SQLULEN tout = static_cast<SQLULEN>(GetLoginTimeout());
  if (Utility::IsError(fun::sql::odbc::SQLSetConnectAttr(db_, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)tout, 0))) {
    if (Utility::IsError(fun::sql::odbc::SQLGetConnectAttr(db_, SQL_ATTR_LOGIN_TIMEOUT, &tout, 0, 0)) ||
        GetLoginTimeout() != tout) {
      ConnectionException e(db_);
      throw ConnectionFailedException(e.errorString(), e);
    }
  }

  SQLCHAR connectOutput[512] = {0};
  SQLSMALLINT result;

  if (Utility::IsError(fun::sql::odbc::SQLDriverConnect(db_,
                NULL,
                SQLCHAR*) GetConnectionString().c_str(),
                SQLSMALLINT) SQL_NTS,
                connectOutput,
                sizeof(connectOutput),
                &result,
                SQL_DRIVER_NOPROMPT))) {
    ConnectionException e(db_);
    close();
    throw ConnectionFailedException(e.errorString(), e);
  }

  data_types_.fillTypeInfo(db_);

  AddProperty("dataTypeInfo",
    &SessionImpl::SetDataTypeInfo,
    &SessionImpl::dataTypeInfo);

  AddFeature("autoCommit",
    &SessionImpl::SetAutoCommit,
    &SessionImpl::IsAutoCommit);

  AddFeature("autoBind",
    &SessionImpl::SetAutoBind,
    &SessionImpl::IsAutoBind);

  AddFeature("autoExtract",
    &SessionImpl::SetAutoExtract,
    &SessionImpl::IsAutoExtract);

  AddFeature("MakeExtractorsBeforeExecute",
    &SessionImpl::MakeExtractorsBeforeExecute,
    &SessionImpl::IsMakeExtractorsBeforeExecute);

  AddProperty("maxFieldSize",
    &SessionImpl::SetMaxFieldSize,
    &SessionImpl::GetMaxFieldSize);

  AddProperty("queryTimeout",
    &SessionImpl::SetQueryTimeout,
    &SessionImpl::GetQueryTimeout);

  fun::sql::odbc::SQLSetConnectAttr(db_, SQL_ATTR_QUIET_MODE, 0, 0);

  if (!CanTransact()) {
    SetAAutoCommit("", true);
  }
}

bool SessionImpl::IsConnected() const {
  SQLULEN value = 0;

  if (!static_cast<bool>(db_) ||
      Utility::IsError(fun::sql::odbc::SQLGetConnectAttr(db_,
            SQL_ATTR_CONNECTION_DEAD,
            &value,
            0,
            0))) {
    return false;
  }

  return (SQL_CD_FALSE == value);
}

void SessionImpl::SetConnectionTimeout(size_t timeout) {
  SQLUINTEGER value = static_cast<SQLUINTEGER>(timeout);

  CheckError(fun::sql::odbc::SQLSetConnectAttr(db_,
    SQL_ATTR_CONNECTION_TIMEOUT,
    &value,
    SQL_IS_UINTEGER), "Failed to set connection timeout.");
}

size_t SessionImpl::GetConnectionTimeout() const {
  SQLULEN value = 0;

  CheckError(fun::sql::odbc::SQLGetConnectAttr(db_,
    SQL_ATTR_CONNECTION_TIMEOUT,
    &value,
    0,
    0), "Failed to Get connection timeout.");

  return value;
}

bool SessionImpl::CanTransact() const {
  if (ODBC_TXN_CAPABILITY_UNKNOWN == can_transact_) {
    SQLUSMALLINT ret;
    SQLRETURN res = fun::sql::odbc::SQLGetInfo(db_, SQL_TXN_CAPABLE, &ret, 0, 0);
    if (!Utility::IsError(res)) {
      can_transact_ = static_cast<char>((SQL_TC_NONE != ret) ? ODBC_TXN_CAPABILITY_TRUE : ODBC_TXN_CAPABILITY_FALSE);
    } else {
      Error<SQLHDBC, SQL_HANDLE_DBC> err(db_);
      can_transact_ = ODBC_TXN_CAPABILITY_FALSE;
    }
  }

  return ODBC_TXN_CAPABILITY_TRUE == can_transact_;
}

void SessionImpl::SetTransactionIsolationImpl(uint32 ti) const {
#if FUN_PTR_IS_64_BIT
  uint64 isolation = 0;
#else
  uint32 isolation = 0;
#endif

  if (ti & Session::TRANSACTION_READ_UNCOMMITTED) {
    isolation |= SQL_TXN_READ_UNCOMMITTED;
  }

  if (ti & Session::TRANSACTION_READ_COMMITTED) {
    isolation |= SQL_TXN_READ_COMMITTED;
  }

  if (ti & Session::TRANSACTION_REPEATABLE_READ) {
    isolation |= SQL_TXN_REPEATABLE_READ;
  }

  if (ti & Session::TRANSACTION_SERIALIZABLE) {
    isolation |= SQL_TXN_SERIALIZABLE;
  }

  CheckError(fun::sql::odbc::SQLSetConnectAttr(db_, SQL_ATTR_TXN_ISOLATION, (SQLPOINTER)isolation, 0));
}

uint32 SessionImpl::GetTransactionIsolation() const {
  SQLULEN isolation = 0;
  CheckError(fun::sql::odbc::SQLGetConnectAttr(db_, SQL_ATTR_TXN_ISOLATION,
    &isolation,
    0,
    0));

  return transactionIsolation(isolation);
}

bool SessionImpl::HasTransactionIsolation(uint32 ti) const {
  if (IsInTransaction()) {
    throw InvalidAccessException();
  }

  bool retval = true;
  uint32 old = GetTransactionIsolation();
  try { SetTransactionIsolationImpl(ti); }
  catch (fun::Exception&) { retval = false; }
  SetTransactionIsolationImpl(old);
  return retval;
}

uint32 SessionImpl::GetDefaultTransactionIsolation() const {
  SQLUINTEGER isolation = 0;
  CheckError(fun::sql::odbc::SQLGetInfo(db_, SQL_DEFAULT_TXN_ISOLATION,
    &isolation,
    0,
    0));

  return transactionIsolation(isolation);
}

uint32 SessionImpl::transactionIsolation(SQLULEN isolation) const {
  if (0 == isolation) {
    throw InvalidArgumentException("transactionIsolation(SQLUINTEGER)");
  }

  uint32 ret = 0;

  if (isolation & SQL_TXN_READ_UNCOMMITTED) {
    ret |= Session::TRANSACTION_READ_UNCOMMITTED;
  }

  if (isolation & SQL_TXN_READ_COMMITTED) {
    ret |= Session::TRANSACTION_READ_COMMITTED;
  }

  if (isolation & SQL_TXN_REPEATABLE_READ) {
    ret |= Session::TRANSACTION_REPEATABLE_READ;
  }

  if (isolation & SQL_TXN_SERIALIZABLE) {
    ret |= Session::TRANSACTION_SERIALIZABLE;
  }

  if (0 == ret) {
    throw InvalidArgumentException("transactionIsolation(SQLUINTEGER)");
  }

  return ret;
}

void SessionImpl::SetAutoCommit(const String&, bool val) {
  CheckError(fun::sql::odbc::SQLSetConnectAttr(db_,
    SQL_ATTR_AUTOCOMMIT,
    val ? (SQLPOINTER) SQL_AUTOCOMMIT_ON :
      (SQLPOINTER) SQL_AUTOCOMMIT_OFF,
    SQL_IS_UINTEGER), "Failed to set automatic commit.");
}

bool SessionImpl::IsAutoCommit(const String&) const {
  SQLULEN value = 0;

  CheckError(fun::sql::odbc::SQLGetConnectAttr(db_,
    SQL_ATTR_AUTOCOMMIT,
    &value,
    0,
    0));

  return (0 != value);
}

bool SessionImpl::IsInTransaction() const {
  if (!CanTransact()) {
    return false;
  }

  SQLULEN value = 0;
  CheckError(fun::sql::odbc::SQLGetConnectAttr(db_,
    SQL_ATTR_AUTOCOMMIT,
    &value,
    0,
    0));

  if (0 == value) {
    return in_transaction_;
  } else {
    return false;
  }
}

void SessionImpl::Begin() {
  if (IsAutoCommit()) {
    throw InvalidAccessException("Session in auto commit mode.");
  }
 {
    fun::FastMutex::ScopedLock l(mutex_);

    if (in_transaction_) {
      throw InvalidAccessException("Transaction in progress.");
    }

    in_transaction_ = true;
  }
}

void SessionImpl::Commit() {
  if (!IsAutoCommit()) {
    CheckError(SQLEndTran(SQL_HANDLE_DBC, db_, SQL_COMMIT));
  }

  in_transaction_ = false;
}

void SessionImpl::Rollback() {
  if (!IsAutoCommit()) {
    CheckError(SQLEndTran(SQL_HANDLE_DBC, db_, SQL_ROLLBACK));
  }

  in_transaction_ = false;
}

void SessionImpl::Reset() {
}

void SessionImpl::Close() {
  if (!IsConnected()) {
    return;
  }

  try {
    Commit();
  } catch (ConnectionException&) {}

  SQLCHAR sql_state[5+1];
  SQLCHAR sql_error_message[SQL_MAX_MESSAGE_LENGTH];
  SQLINTEGER native_error_code;
  SQLSMALLINT sql_error_message_length;
  unsigned int retry_count = 10;
  do {
    SQLRETURN rc = SQLDisconnect(db_);
    if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc) {
      SQLGetDiagRec(SQL_HANDLE_DBC, db_,
              1,
              sql_state,
              &native_error_code,
              sql_error_message, SQL_MAX_MESSAGE_LENGTH, &sql_error_message_length);
      if (String(reinterpret_cast<const char *>(sql_state)) == "25000"
        || String(reinterpret_cast<const char *>(sql_state)) == "25501") {
        --retry_count;
        fun::Thread::sleep(100);
      } else {
        break;
      }
    } else {
      return;
    }
  } while(retry_count > 0);

  throw OdbcException
    (String(reinterpret_cast<const char *>(sql_error_message)), native_error_code);
}

int SessionImpl::maxStatementLength() const {
  SQLUINTEGER info;
  SQLRETURN rc = 0;
  if (Utility::IsError(rc = fun::sql::odbc::SQLGetInfo(db_,
    SQL_MAXIMUM_STATEMENT_LENGTH,
    (SQLPOINTER) &info,
    0,
    0))) {
    throw ConnectionException(db_, "SQLGetInfo(SQL_MAXIMUM_STATEMENT_LENGTH)");
  }

  return info;
}

} // namespace odbc
} // namespace sql
} // namespace fun
