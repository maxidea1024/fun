#pragma once

#include "fun/base/mutex.h"
#include "fun/sql/mysql/mysql.h"
#include "fun/sql/mysql/result_metadata.h"
#include "fun/sql/mysql/session_handle.h"
#include "fun/sql/mysql/statement_executor.h"
#include "fun/sql/sessionimpl_base.h"
#include "fun/sql/statement_impl.h"

namespace fun {
namespace sql {
namespace mysql {

/**
 * Implements SessionImpl interface
 */
class FUN_MYSQL_API SessionImpl
    : public fun::sql::SessionImplBase<SessionImpl> {
 public:
  static const String MYSQL_READ_UNCOMMITTED;
  static const String MYSQL_READ_COMMITTED;
  static const String MYSQL_REPEATABLE_READ;
  static const String MYSQL_SERIALIZABLE;

  /**
   * Creates the SessionImpl. Opens a connection to the database
   *
   * Connection string format:
   *     <str> == <assignment> | <assignment> ';' <str>
   *     <assignment> == <name> '=' <value>
   *     <name> == 'host' | 'port' | 'user' | 'password' | 'db' } 'compress' |
   * 'auto-Reconnect' <value> == [~;]*
   *
   * for compress and auto-Reconnect correct values are true/false
   * for port - numeric in decimal notation
   */
  SessionImpl(const String& connection_string,
              size_t login_timeout = LOGIN_TIMEOUT_DEFAULT);

  /**
   * Destroys the SessionImpl.
   */
  ~SessionImpl();

  /**
   * Returns an MySQL StatementImpl
   */
  StatementImpl::Ptr CreateStatementImpl();

  /**
   * Opens a connection to the database.
   */
  void Open(const String& connection = "");

  /**
   * Closes the connection.
   */
  void Close();

  /**
   * Reset connection with dababase and clears session state, but without
   * disconnecting
   */
  void Reset();

  /**
   * Returns true if connected, false otherwise.
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
  void SetAutoCommit(const String&, bool val);

  /**
   * Returns autocommit property value.
   */
  bool IsAutoCommit(const String& name = "") const;

  /**
   * Try to set insert id - do nothing.
   */
  void SetInsertId(const String&, const fun::Any&);

  /**
   * Get insert id
   */
  fun::Any GetInsertId(const String&) const;

  /**
   * Get handle
   */
  SessionHandle& GetHandle();

  /**
   * Returns the name of the connector.
   */
  const String& GetConnectorName() const;

 private:
  template <typename T>
  static inline T& GetValue(MYSQL_BIND* result, T& val) {
    return val = *((T*)result->buffer);
  }

  /**
   * Returns required setting.
   * Limited to one setting at a time.
   */
  template <typename T>
  T& GetSetting(const String& name, T& val) const {
    StatementExecutor ex(handle_);
    ResultMetadata metadata;
    metadata.Reset();
    ex.Prepare(fun::Format("SELECT @@%s", name));
    metadata.Init(ex);

    if (metadata.ReturnedColumnCount() > 0) {
      ex.BindResult(metadata.Row());
    } else {
      throw InvalidArgumentException("No data returned.");
    }

    ex.Execute();
    ex.Fetch();
    MYSQL_BIND* result = metadata.Row();
    return GetValue<T>(result, val);
  }

  String connector_;
  mutable SessionHandle handle_;
  bool connected_;
  bool in_transaction_;
  size_t timeout_;
  fun::FastMutex mutex_;
};

//
// inlines
//

inline bool SessionImpl::CanTransact() const { return true; }

inline void SessionImpl::SetInsertId(const String&, const fun::Any&) {}

inline fun::Any SessionImpl::GetInsertId(const String&) const {
  return fun::Any(uint64(mysql_insert_id(handle_)));
}

inline SessionHandle& SessionImpl::GetHandle() { return handle_; }

inline const String& SessionImpl::GetConnectorName() const {
  return connector_;
}

inline bool SessionImpl::IsInTransaction() const { return in_transaction_; }

inline bool SessionImpl::IsTransactionIsolation(uint32 ti) const {
  return GetTransactionIsolation() == ti;
}

inline bool SessionImpl::IsConnected() const { return connected_; }

inline size_t SessionImpl::GetConnectionTimeout() const { return timeout_; }

template <>
inline String& SessionImpl::GetValue(MYSQL_BIND* result, String& val) {
  val.Assign((char*)result->buffer, result->buffer_length);
  return val;
}

}  // namespace mysql
}  // namespace sql
}  // namespace fun
