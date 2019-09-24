#pragma once

#include "fun/sql/postgresql/postgresql.h"
#include "fun/sql/postgresql/session_handle.h"
#include "fun/sql/session_impl_base.h"
#include "fun/sql/statement_impl.h"

#include <string>

namespace fun {
namespace sql {
namespace postgresql {

/**
 * Implements SessionImpl interface
 */
class FUN_POSTGRESQL_API SessionImpl
    : public fun::sql::SessionImplBase<SessionImpl> {
 public:
  /**
   * Creates the SessionImpl. Opens a connection to the database
   *
   * Connection string format:
   * <str> == <assignment> | <assignment> ' ' <str>
   * <assignment> == <name> '=' <value>
   * <name> == 'host' | 'port' | 'user' | 'password' | 'dbname' |
   * 'connect_timeout' <value> == [~;]*
   *
   * consult postgres documentation for other parameters
   */
  SessionImpl(const String& connection_string,
              size_t login_timeout = LOGIN_TIMEOUT_DEFAULT);

  /**
   * Destroys the SessionImpl.
   */
  ~SessionImpl();

  /**
   * Sets the session connection timeout value.
   */
  void SetConnectionTimeout(size_t timeout);

  /**
   * Returns the session connection timeout value.
   */
  size_t GetConnectionTimeout() const;

  /**
   * Opens a connection to the database.
   */
  void Open(const String& connection_string = String());

  /**
   * Closes the connection.
   */
  void Close();

  /**
   * Do nothing
   */
  void Reset();

  /**
   * Returns true if connected, false otherwise.
   */
  bool IsConnected() const;

  /**
   * Returns an PostgreSQL StatementImpl
   */
  fun::sql::StatementImpl::Ptr CreateStatementImpl();

  /**
   * Starts a transaction
   */
  void Begin();

  /**
   * Commits and ends a transaction
   */
  void Commit();

  /**
   * Aborts a transaction
   */
  void Rollback();

  /**
   * Returns true if session has transaction capabilities.
   */
  bool CanTransact() const;

  /**
   * Returns true if a transaction is a transaction is in progress, false
   * otherwise.
   */
  bool IsInTransaction() const;

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
  bool HasTransactionIsolation(uint32 ti) const;

  /**
   * Returns true if the transaction isolation level corresponds
   * to the supplied bitmask.
   */
  bool IsTransactionIsolation(uint32 ti) const;

  /**
   * Sets autocommit property for the session.
   */
  void SetAutoCommit(const String&, bool value);

  /**
   * Returns autocommit property value.
   */
  bool IsAutoCommit(const String& name = String()) const;

  /**
   * Sets asynchronousCommit property for the session.
   */
  void SetAsynchronousCommit(const String&, bool value);

  /**
   * is the connection in Asynchronous commit mode?
   */
  bool IsAsynchronousCommit(const String& name = String()) const;

  /**
   * Get handle
   */
  SessionHandle& GetHandle();

  /**
   * Returns the name of the connector.
   */
  const String& GetConnectorName() const;

 private:
  String connector_name_;
  mutable SessionHandle session_handle_;
  size_t timeout_;
};

//
// inlines
//

inline bool SessionImpl::CanTransact() const { return true; }

inline SessionHandle& SessionImpl::GetHandle() { return session_handle_; }

inline const String& SessionImpl::GetConnectorName() const {
  return connector_name_;
}

inline bool SessionImpl::IsTransactionIsolation(uint32 ti) const {
  return GetTransactionIsolation() == ti;
}

inline size_t SessionImpl::GetConnectionTimeout() const { return timeout_; }

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
