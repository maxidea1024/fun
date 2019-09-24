#pragma once

#include "fun/base/mutex.h"
#include "fun/base/shared_ptr.h"
#include "fun/sql/session_impl_base.h"
#include "fun/sql/sqlite/binder.h"
#include "fun/sql/sqlite/connector.h"
#include "fun/sql/sqlite/sqlite.h"
#include "fun/sql/statement_impl.h"

extern "C" {
typedef struct sqlite3 sqlite3;
}

namespace fun {
namespace sql {
namespace sqlite {

/**
 * Implements SessionImpl interface.
 */
class FUN_SQLITE_API SessionImpl
    : public fun::sql::SessionImplBase<SessionImpl> {
 public:
  /**
   * Creates the SessionImpl. Opens a connection to the database.
   */
  SessionImpl(const String& filename,
              size_t login_timeout = LOGIN_TIMEOUT_DEFAULT);

  /**
   * Destroys the SessionImpl.
   */
  ~SessionImpl();

  /**
   * Returns an SQLite StatementImpl.
   */
  StatementImpl::Ptr CreateStatementImpl();

  /**
   * Opens a connection to the Database.
   *
   * An in-memory system database (sys), with a single table (dual)
   * containing single field (dummy) is attached to the database.
   * The in-memory system database is used to force change count
   * to be reset to zero on every new query (or batch of queries)
   * execution. Without this functionality, select statements
   * executions that do not return any rows return the count of
   * changes effected by the most recent insert, update or delete.
   * In-memory system database can be queried and updated but can not
   * be dropped. It may be used for other purposes
   * in the future.
   */
  void Open(const String& connect = "");

  /**
   * Closes the session.
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
   * Sets the session connection timeout value.
   * Timeout value is in seconds.
   * Throws RangeException if the timeout value is overflow.
   */
  void SetConnectionTimeout(size_t timeout);

  /**
   * Returns the session connection timeout value.
   * Timeout value is in seconds.
   */
  size_t GetConnectionTimeout() const;

  /**
   * Starts a transaction.
   */
  void Begin();

  /**
   * Commits and ends a transaction.
   */
  void Commit();

  /**
   * Aborts a transaction.
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
  void SetAutoCommit(const String&, bool val);

  /**
   * Returns autocommit property value.
   */
  bool IsAutoCommit(const String& name = "") const;

  /**
   * Returns the name of the connector.
   */
  const String& GetConnectorName() const;

 protected:
  void SetConnectionTimeout(const String& prop, const fun::Any& value);
  fun::Any GetConnectionTimeout(const String& prop) const;

 private:
  String connector_;
  sqlite3* db_;
  bool connected_;
  bool is_transaction_;
  int timeout_;
  mutable fun::Mutex mutex_;

  static const String DEFERRED_BEGIN_TRANSACTION;
  static const String COMMIT_TRANSACTION;
  static const String ABORT_TRANSACTION;
};

//
// inlines
//

inline bool SessionImpl::CanTransact() const { return true; }

inline bool SessionImpl::IsInTransaction() const { return is_transaction_; }

inline const String& SessionImpl::connectorName() const { return connector_; }

inline size_t SessionImpl::GetConnectionTimeout() const {
  return static_cast<size_t>(timeout_ / 1000);
}

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
