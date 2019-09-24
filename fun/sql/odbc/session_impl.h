#pragma once

#include "fun/base/mutex.h"
#include "fun/base/shared_ptr.h"
#include "fun/sql/odbc/binder.h"
#include "fun/sql/odbc/connector.h"
#include "fun/sql/odbc/handle.h"
#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/odbc/type_info.h"
#include "fun/sql/session_impl_base.h"
#include "fun/sql/statement_impl.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqltypes.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * Implements SessionImpl interface
 */
class FUN_ODBC_API SessionImpl : public fun::sql::SessionImplBase<SessionImpl> {
 public:
  static const size_t ODBC_MAX_FIELD_SIZE = 1024u;

  enum TransactionCapability {
    ODBC_TXN_CAPABILITY_UNKNOWN = -1,
    ODBC_TXN_CAPABILITY_FALSE = 0,
    ODBC_TXN_CAPABILITY_TRUE = 1
  };

  /**
   * Creates the SessionImpl. Opens a connection to the database.
   * Throws NotConnectedException if connection was not successful.
   */
  SessionImpl(const String& connect, size_t login_timeout,
              size_t maxFieldSize = ODBC_MAX_FIELD_SIZE, bool autoBind = true,
              bool autoExtract = true,
              bool MakeExtractorsBeforeExecute = false);

  //@ deprecated
  /**
   * Creates the SessionImpl. Opens a connection to the database.
   */
  SessionImpl(const String& connect,
              fun::Any maxFieldSize = ODBC_MAX_FIELD_SIZE,
              bool enforceCapability = false, bool autoBind = true,
              bool autoExtract = true,
              bool MakeExtractorsBeforeExecute = false);

  /**
   * Destroys the SessionImpl.
   */
  ~SessionImpl();

  /**
   * Returns an ODBC StatementImpl
   */
  StatementImpl::Ptr CreateStatementImpl();

  /**
   * Opens a connection to the Database
   */
  void Open(const String& connect = "");

  /**
   * Closes the connection
   * Throws OdbcException if close was not successful.
   */
  void Close();

  /**
   * Returns true if session is connected
   */
  bool IsConnected() const;

  /**
   * Sets the session connection timeout value.
   */
  void SetConnectionTimeout(size_t timeout);

  /**
   * Returns the session connection timeout value.
   */
  size_t GetConnectionTimeout() const;

  /**
   * Starts a transaction
   */
  void begin();

  /**
   * Commits and ends a transaction
   */
  void Commit();

  /**
   * Aborts a transaction
   */
  void Rollback();

  /**
   * Do nothing
   */
  void reset();

  /**
   * Returns true if a transaction is in progress.
   */
  bool IsInTransaction() const;

  /**
   * Returns the name of the connector.
   */
  const String& GetConnectorName() const;

  /**
   * Returns true if connection is transaction-capable.
   */
  bool CanTransact() const;

  /**
   * Sets the transaction isolation level.
   */
  void SetTransactionIsolation(uint32 ti);

  /**
   * Returns the transaction isolation level.
   */
  uint32 GetTransactionIsolation() const;

  /**
   * Returns true if the transaction isolation level corresponding
   * to the supplied bitmask is supported.
   */
  bool HasTransactionIsolation(uint32) const;

  /**
   * Returns true if the transaction isolation level corresponds
   * to the supplied bitmask.
   */
  bool IsTransactionIsolation(uint32) const;

  /**
   * Sets autocommit property for the session.
   */
  void SetAutoCommit(const String&, bool val);

  /**
   * Returns autocommit property value.
   */
  bool IsAutoCommit(const String& name = "") const;

  /**
   * Sets automatic binding for the session.
   */
  void SetAutoBind(const String&, bool val);

  /**
   * Returns true if binding is automatic for this session.
   */
  bool IsAutoBind(const String& name = "") const;

  /**
   * Sets automatic extraction for the session.
   */
  void SetAutoExtract(const String&, bool val);

  /**
   * Returns true if extraction is automatic for this session.
   */
  bool IsAutoExtract(const String& name = "") const;

  /**
   * Sets internal extractor creation flag for the session.
   */
  void MakeExtractorsBeforeExecute(const String&, bool val);

  /**
   * Returns true if internal extractor creator is done before execution of
   * statement.
   */
  bool IsMakeExtractorsBeforeExecute(const String& name = "") const;

  /**
   * Sets the max field size (the default used when column size is unknown).
   */
  void SetMaxFieldSize(const String& name, const fun::Any& value);

  /**
   * Returns the max field size (the default used when column size is unknown).
   */
  fun::Any GetMaxFieldSize(const String& name = "") const;

  /**
   * Returns maximum length of sql statement allowed by driver.
   */
  int maxStatementLength() const;

  /**
   * Sets the timeout (in seconds) for queries.
   * Value must be of type int.
   */
  void SetQueryTimeout(const String&, const fun::Any& value);

  /**
   * Returns the timeout (in seconds) for queries,
   * or -1 if no timeout has been set.
   */
  fun::Any GetQueryTimeout(const String&) const;

  /**
   * Returns the timeout (in seconds) for queries,
   * or -1 if no timeout has been set.
   */
  int queryTimeout() const;

  /**
   * Returns the connection handle.
   */
  const ConnectionHandle& dbc() const;

  /**
   * Returns the data types information.
   */
  fun::Any dataTypeInfo(const String& name = "") const;

 private:
  /**
   * No-op. Throws InvalidAccessException.
   */
  void SetDataTypeInfo(const String& name, const fun::Any& value);

  void init();

  static const int FUNCTIONS = SQL_API_ODBC3_ALL_FUNCTIONS_SIZE;

  void CheckError(SQLRETURN rc, const String& msg = "") const;

  uint32 GetDefaultTransactionIsolation() const;

  uint32 transactionIsolation(SQLULEN isolation) const;

  void SetTransactionIsolationImpl(uint32 ti) const;

  String connector_;
  const ConnectionHandle db_;
  fun::Any max_field_size_;
  bool auto_bind_;
  bool auto_extract_;
  bool make_extractor_before_execute_;
  TypeInfo data_types_;
  mutable char can_transact_;
  bool in_transaction_;
  int query_timeout_;
  fun::FastMutex mutex_;
};

//
// inlines
//

inline void SessionImpl::CheckError(SQLRETURN rc, const String& msg) const {
  if (Utility::IsError(rc)) {
    throw ConnectionException(db_, msg);
  }
}

inline const ConnectionHandle& SessionImpl::dbc() const { return db_; }

inline void SessionImpl::SetMaxFieldSize(const String& /*name*/,
                                         const fun::Any& value) {
  max_field_size_ = value;
}

inline fun::Any SessionImpl::GetMaxFieldSize(const String& /*name*/) const {
  return max_field_size_;
}

inline void SessionImpl::SetDataTypeInfo(const String& /*name*/,
                                         const fun::Any& /*value*/) {
  throw InvalidAccessException();
}

inline fun::Any SessionImpl::dataTypeInfo(const String& /*name*/) const {
  return const_cast<TypeInfo*>(&data_types_);
}

inline void SessionImpl::SetAutoBind(const String&, bool val) {
  auto_bind_ = val;
}

inline bool SessionImpl::IsAutoBind(const String& /*name*/) const {
  return auto_bind_;
}

inline void SessionImpl::SetAutoExtract(const String&, bool val) {
  auto_extract_ = val;
}

inline bool SessionImpl::IsAutoExtract(const String& /*name*/) const {
  return auto_extract_;
}

inline void SessionImpl::MakeExtractorsBeforeExecute(const String&, bool val) {
  make_extractor_before_execute_ = val;
}

inline bool SessionImpl::IsMakeExtractorsBeforeExecute(
    const String& name) const {
  return make_extractor_before_execute_;
}

inline const String& SessionImpl::GetConnectorName() const {
  return connector_;
}

inline void SessionImpl::SetTransactionIsolation(uint32 ti) {
  SetTransactionIsolationImpl(ti);
}

inline bool SessionImpl::IsTransactionIsolation(uint32 ti) const {
  return 0 != (ti & GetTransactionIsolation());
}

inline void SessionImpl::SetQueryTimeout(const String&, const fun::Any& value) {
  query_timeout_ = fun::AnyCast<int>(value);
}

inline fun::Any SessionImpl::GetQueryTimeout(const String&) const {
  return query_timeout_;
}

inline int SessionImpl::queryTimeout() const { return query_timeout_; }

}  // namespace odbc
}  // namespace sql
}  // namespace fun
