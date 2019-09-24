#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/session_impl.h"
#include "fun/sql/statement_impl.h"
#include "fun/sql/pooled_session_holder.h"
#include "fun/ref_counted_ptr.h"

namespace fun {
namespace sql {

class SessionPool;

/**
 * PooledSessionImpl is a decorator created by
 * SessionPool that adds session pool
 * management to SessionImpl objects.
 */
class FUN_SQL_API PooledSessionImpl : public SessionImpl {
 public:
  /**
   * Creates the PooledSessionImpl.
   */
  PooledSessionImpl(PooledSessionHolder::Ptr holder);

  /**
   * Destroys the PooledSessionImpl.
   */
  ~PooledSessionImpl();

  // SessionImpl
  StatementImpl::Ptr CreateStatementImpl();
  void Begin();
  void Commit();
  void Rollback();
  void Open(const String& connect = "");
  void Close();
  void Reset();
  bool IsConnected() const;
  void SetConnectionTimeout(size_t timeout);
  size_t GetConnectionTimeout() const;
  bool CanTransact() const;
  bool IsInTransaction() const;
  void SetTransactionIsolation(uint32);
  uint32 GetTransactionIsolation() const;
  bool HasTransactionIsolation(uint32) const;
  bool IsTransactionIsolation(uint32) const;
  const String& GetConnectorName() const;
  void SetFeature(const String& name, bool state);
  bool GetFeature(const String& name) const;
  void SetProperty(const String& name, const fun::Any& value);
  fun::Any GetProperty(const String& name) const;

  virtual void PutBack();

 protected:
  /**
   * Updates the last access timestamp,
   * verifies validity of the session
   * and returns the session if it is valid.
   *
   * Throws an SessionUnavailableException if the
   * session is no longer valid.
   */
  SessionImpl::Ptr Access() const;

  /**
   * Returns a pointer to the SessionImpl.
   */
  SessionImpl::Ptr GetImpl() const;

 private:
  mutable PooledSessionHolder::Ptr holder_;
};


//
// inlines
//

inline SessionImpl::Ptr PooledSessionImpl::GetImpl() const {
  return holder_->GetSession();
}

} // namespace sql
} // namespace fun
