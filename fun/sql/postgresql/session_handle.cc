#include "fun/sql/postgresql/session_handle.h"
#include "fun/base/number_formatter.h"
#include "fun/sql/postgresql/postgresql_exception.h"
#include "fun/sql/postgresql/postgresql_types.h"
#include "fun/sql/session.h"

#define FUN_POSTGRESQL_VERSION_NUMBER                     \
  ((NDB_VERSION_MAJOR << 16) | (NDB_VERSION_MINOR << 8) | \
   (NDB_VERSION_BUILD & 0xFF))

namespace fun {
namespace sql {
namespace postgresql {

// const String SessionHandle::POSTGRESQL_READ_UNCOMMITTED = "READ UNCOMMITTED";
const String SessionHandle::POSTGRESQL_READ_COMMITTED = "READ COMMITTED";
const String SessionHandle::POSTGRESQL_REPEATABLE_READ = "REPEATABLE READ";
const String SessionHandle::POSTGRESQL_SERIALIZABLE = "SERIALIZABLE";

SessionHandle::SessionHandle()
    : connection_ptr_(0),
      in_transaction_(false),
      is_auto_commit_(true),
      is_asynchronous_commit_(false),
      transaction_isolation_level_(Session::TRANSACTION_READ_COMMITTED) {}

SessionHandle::~SessionHandle() {
  try {
    Disconnect();
  } catch (...) {
  }
}

bool SessionHandle::IsConnected() const {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  return IsConnectedNoLock();
}

bool SessionHandle::IsConnectedNoLock() const {
  // DO NOT ACQUIRE THE MUTEX IN PRIVATE METHODS

  if (connection_ptr_ && PQstatus(connection_ptr_) == CONNECTION_OK) {
    return true;
  }

  return false;
}

void SessionHandle::Connect(const String& connection_string) {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (IsConnectedNoLock()) {
    throw ConnectionFailedException("Already Connected");
  }

  connection_ptr_ = PQconnectdb(connection_string.c_str());

  if (!IsConnectedNoLock()) {
    throw ConnectionFailedException(String("Connection Error: ") +
                                    GetLastErrorNoLock());
  }

  connection_string_ = connection_string;
}

void SessionHandle::Connect(const char* connection_string) {
  Connect(String(connection_string));
}

void SessionHandle::Connect(const char* host, const char* user,
                            const char* password, const char* database,
                            unsigned short port,
                            unsigned int connection_timeout) {
  String connection_string;

  connection_string.Append("host=");
  connection_string.Append(host);
  connection_string.Append(" ");

  connection_string.Append("user=");
  connection_string.Append(user);
  connection_string.Append(" ");

  connection_string.Append("password=");
  connection_string.Append(password);
  connection_string.Append(" ");

  connection_string.Append("dbname=");
  connection_string.Append(database);
  connection_string.Append(" ");

  connection_string.Append("port=");
  fun::NumberFormatter::Append(connection_string, port);
  connection_string.Append(" ");

  connection_string.Append("connect_timeout=");
  fun::NumberFormatter::Append(connection_string, connection_timeout);

  Connect(connection_string);
}

void SessionHandle::Disconnect() {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (IsConnectedNoLock()) {
    PQfinish(connection_ptr_);

    connection_ptr_ = nullptr;

    connection_string_ = String();
    in_transaction_ = false;
    is_auto_commit_ = true;
    is_asynchronous_commit_ = false;
    transaction_isolation_level_ = Session::TRANSACTION_READ_COMMITTED;
  }
}

// TODO: Figure out what happens if a connection is reset with a pending
// transaction
bool SessionHandle::Reset() {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (connection_ptr_) {
    PQreset(connection_ptr_);
  }

  if (IsConnectedNoLock()) {
    return true;
  }

  return false;
}

String SessionHandle::GetLastError() const {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    return String();
  }

  return GetLastErrorNoLock();
}

String SessionHandle::GetLastErrorNoLock() const {
  // DO NOT ACQUIRE THE MUTEX IN PRIVATE METHODS
  String lastErrorString(0 != connection_ptr_ ? PQerrorMessage(connection_ptr_)
                                              : "not connected");

  return lastErrorString;
}

void SessionHandle::StartTransaction() {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  if (in_transaction_) {
    return;  // NO-OP
  }

  PGresult* pq_result = PQexec(connection_ptr_, "BEGIN");

  PQResultClear result_clearer(pq_result);

  if (PQresultStatus(pq_result) != PGRES_COMMAND_OK) {
    throw StatementException(String("BEGIN statement failed:: ") +
                             GetLastErrorNoLock());
  }

  in_transaction_ = true;
}

void SessionHandle::Commit() {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  PGresult* pq_result = PQexec(connection_ptr_, "COMMIT");

  PQResultClear result_clearer(pq_result);

  if (PQresultStatus(pq_result) != PGRES_COMMAND_OK) {
    throw StatementException(String("COMMIT statement failed:: ") +
                             GetLastErrorNoLock());
  }

  in_transaction_ = false;

  DeallocateStoredPreparedStatements();
}

void SessionHandle::Rollback() {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  PGresult* pq_result = PQexec(connection_ptr_, "ROLLBACK");

  PQResultClear result_clearer(pq_result);

  if (PQresultStatus(pq_result) != PGRES_COMMAND_OK) {
    throw StatementException(String("ROLLBACK statement failed:: ") +
                             GetLastErrorNoLock());
  }

  in_transaction_ = false;

  DeallocateStoredPreparedStatements();
}

void SessionHandle::SetAutoCommit(bool aShouldAutoCommit) {
  if (aShouldAutoCommit == is_auto_commit_) {
    return;
  }

  if (aShouldAutoCommit) {
    Commit();  // end any in process transaction
  } else {
    StartTransaction();  // start a new transaction
  }

  is_auto_commit_ = aShouldAutoCommit;
}

void SessionHandle::SetAsynchronousCommit(bool aShouldAsynchronousCommit) {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  if (aShouldAsynchronousCommit == is_asynchronous_commit_) {
    return;
  }

  PGresult* pq_result =
      PQexec(connection_ptr_, aShouldAsynchronousCommit
                                  ? "SET SYNCHRONOUS COMMIT TO OFF"
                                  : "SET SYNCHRONOUS COMMIT TO ON");

  PQResultClear result_clearer(pq_result);

  if (PQresultStatus(pq_result) != PGRES_COMMAND_OK) {
    throw StatementException(
        String("SET SYNCHRONUS COMMIT statement failed:: ") +
        GetLastErrorNoLock());
  }

  is_asynchronous_commit_ = aShouldAsynchronousCommit;
}

void SessionHandle::Cancel() {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  PGcancel* pq_cancel = PQgetCancel(connection_ptr_);

  PGCancelFree cancel_freer(pq_cancel);

  PQcancel(pq_cancel, 0, 0);  // no error buffer
}

void SessionHandle::SetTransactionIsolation(uint32 ti) {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  if (ti == transaction_isolation_level_) {
    return;
  }

  if (!HasTransactionIsolation(ti)) {
    throw fun::InvalidArgumentException("SetTransactionIsolation()");
  }

  String isolation_level;

  switch (ti) {
    case Session::TRANSACTION_READ_COMMITTED:
      isolation_level = POSTGRESQL_READ_COMMITTED;
      break;
    case Session::TRANSACTION_REPEATABLE_READ:
      isolation_level = POSTGRESQL_REPEATABLE_READ;
      break;
    case Session::TRANSACTION_SERIALIZABLE:
      isolation_level = POSTGRESQL_SERIALIZABLE;
      break;
  }

  PGresult* pq_result = PQexec(
      connection_ptr_,
      fun::Format(
          "SET SESSION CHARACTERISTICS AS TRANSACTION ISOLATION LEVEL %s",
          isolation_level)
          .c_str());

  PQResultClear result_clearer(pq_result);

  if (PQresultStatus(pq_result) != PGRES_COMMAND_OK) {
    throw StatementException(
        String("set transaction isolation statement failed: ") +
        GetLastErrorNoLock());
  }

  transaction_isolation_level_ = ti;
}

uint32 SessionHandle::transactionIsolation() {
  return transaction_isolation_level_;
}

bool SessionHandle::HasTransactionIsolation(uint32 ti) {
  return Session::TRANSACTION_READ_COMMITTED == ti ||
         Session::TRANSACTION_REPEATABLE_READ == ti ||
         Session::TRANSACTION_SERIALIZABLE == ti;
}

void SessionHandle::DeallocatePreparedStatement(
    const String& aPreparedStatementToDeAllocate) {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  if (!in_transaction_) {
    DeallocatePreparedStatementNoLock(aPreparedStatementToDeAllocate);
  } else {
    try {
      prepared_statements_to_be_deallocated_.push_back(
          aPreparedStatementToDeAllocate);
    } catch (std::bad_alloc&) {
    }
  }
}

void SessionHandle::DeallocatePreparedStatementNoLock(
    const String& aPreparedStatementToDeAllocate) {
  PGresult* pq_result =
      PQexec(connection_ptr_,
             (String("DEALLOCATE ") + aPreparedStatementToDeAllocate).c_str());

  PQResultClear result_clearer(pq_result);

  if (PQresultStatus(pq_result) != PGRES_COMMAND_OK) {
    throw StatementException(String("DEALLOCATE statement failed: ") +
                             GetLastErrorNoLock());
  }
}

void SessionHandle::DeallocateStoredPreparedStatements() {
  // DO NOT ACQUIRE THE MUTEX IN PRIVATE METHODS
  while (!prepared_statements_to_be_deallocated_.IsEmpty()) {
    DeallocatePreparedStatementNoLock(
        prepared_statements_to_be_deallocated_.back());

    prepared_statements_to_be_deallocated_.pop_back();
  }
}

int SessionHandle::GetServerVersion() const {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  return PQserverVersion(connection_ptr_);
}

int SessionHandle::GetServerProcessId() const {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  return PQbackendPID(connection_ptr_);
}

int SessionHandle::GetProtocoVersion() const {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  return PQprotocolVersion(connection_ptr_);
}

String SessionHandle::GetClientEncoding() const {
  fun::FastMutex::ScopedLock guard(session_mutex_);

  if (!IsConnectedNoLock()) {
    throw NotConnectedException();
  }

  return pg_encoding_to_char(PQclientEncoding(connection_ptr_));
}

int SessionHandle::GetLibpqVersion() const { return PQlibVersion(); }

SessionParametersMap SessionHandle::SetConnectionInfoParameters(
    PQconninfoOption* pConnInfOpt) {
  SessionParametersMap session_parameter_map;

  while (0 != pConnInfOpt->keyword) {
    try {
      String keyword = pConnInfOpt->keyword ? pConnInfOpt->keyword : String();
      String environmentVariableVersion =
          pConnInfOpt->envvar ? pConnInfOpt->envvar : String();
      String compiledVersion =
          pConnInfOpt->compiled ? pConnInfOpt->compiled : String();
      String currentValue = pConnInfOpt->val ? pConnInfOpt->val : String();
      String dialogLabel = pConnInfOpt->label ? pConnInfOpt->label : String();
      String dialogDisplayCharacter =
          pConnInfOpt->dispchar ? pConnInfOpt->dispchar : String();
      int dialogDisplaysize = pConnInfOpt->dispsize;

      SessionParameters connParams(keyword, environmentVariableVersion,
                                   compiledVersion, currentValue, dialogLabel,
                                   dialogDisplayCharacter, dialogDisplaysize);

      session_parameter_map.insert(
          SessionParametersMap::value_type(connParams.keyword(), connParams));
    } catch (std::bad_alloc&) {
    }

    ++pConnInfOpt;
  }

  return session_parameter_map;
}

SessionParametersMap SessionHandle::GetConnectionDefaultParameters() {
  PQconninfoOption* ptrConnInfoOptions = PQconndefaults();

  PQConnectionInfoOptionsFree connectionOptionsFreeer(ptrConnInfoOptions);

  return SetConnectionInfoParameters(ptrConnInfoOptions);
}

SessionParametersMap SessionHandle::GetConnectionParameters() const {
  if (!IsConnected()) {
    throw NotConnectedException();
  }

  PQconninfoOption* ptrConnInfoOptions = nullptr;
  {
    fun::FastMutex::ScopedLock guard(session_mutex_);

    ptrConnInfoOptions = PQconninfo(connection_ptr_);
  }

  PQConnectionInfoOptionsFree connectionOptionsFreeer(ptrConnInfoOptions);

  return SetConnectionInfoParameters(ptrConnInfoOptions);
}

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
