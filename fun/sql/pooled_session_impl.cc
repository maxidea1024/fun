#include "fun/sql/pooled_session_impl.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/session_pool.h"

namespace fun {
namespace sql {

PooledSessionImpl::PooledSessionImpl(PooledSessionHolder::Ptr holder)
  : SessionImpl(holder->GetSession()->connection_string(),
    holder->GetSession()->GetLoginTimeout()),
  holder_(holder) {
}

PooledSessionImpl::~PooledSessionImpl() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

StatementImpl::Ptr PooledSessionImpl::CreateStatementImpl() {
  return Access()->CreateStatementImpl();
}

void PooledSessionImpl::Begin() {
  return Access()->Begin();
}

void PooledSessionImpl::Commit() {
  return Access()->Commit();
}

bool PooledSessionImpl::IsConnected() const {
  return Access()->IsConnected();
}

void PooledSessionImpl::SetConnectionTimeout(size_t timeout) {
  return Access()->SetConnectionTimeout(timeout);
}

size_t PooledSessionImpl::GetConnectionTimeout() const {
  return Access()->GetConnectionTimeout();
}

bool PooledSessionImpl::CanTransact() const {
  return Access()->CanTransact();
}

bool PooledSessionImpl::IsInTransaction() const {
  return Access()->IsInTransaction();
}

void PooledSessionImpl::SetTransactionIsolation(uint32 ti) {
  Access()->SetTransactionIsolation(ti);
}

uint32 PooledSessionImpl::GetTransactionIsolation() const {
  return Access()->GetTransactionIsolation();
}

bool PooledSessionImpl::HasTransactionIsolation(uint32 ti) const {
  return Access()->HasTransactionIsolation(ti);
}

bool PooledSessionImpl::IsTransactionIsolation(uint32 ti) const {
  return Access()->IsTransactionIsolation(ti);
}

void PooledSessionImpl::Rollback() {
  return Access()->Rollback();
}

void PooledSessionImpl::Open(const String& connect) {
  Access()->Open(connect);
}

void PooledSessionImpl::PutBack() {
  if (holder_) {
    holder_->GetOwner().PutBack(holder_);
    holder_ = nullptr;
  }
}

void PooledSessionImpl::Close() {
  if (holder_) {
    if (IsInTransaction()) {
      try {
        Rollback();
      } catch (...) {
        // Something's wrong with the session. Get rid of it.
        Access()->Close();
      }
    }
    PutBack();
  }
}

void PooledSessionImpl::Reset() {
  Access()->Reset();
}

const String& PooledSessionImpl::GetConnectorName() const {
  return Access()->GetConnectorName();
}

void PooledSessionImpl::SetFeature(const String& name, bool state) {
  Access()->SetFeature(name, state);
}

bool PooledSessionImpl::GetFeature(const String& name) const {
  return Access()->GetFeature(name);
}

void PooledSessionImpl::SetProperty(const String& name, const fun::Any& value) {
  Access()->SetProperty(name, value);
}

fun::Any PooledSessionImpl::GetProperty(const String& name) const {
  return Access()->GetProperty(name);
}

SessionImpl::Ptr PooledSessionImpl::Access() const {
  if (holder_) {
    holder_->Access();
    return GetImpl();
  } else {
    throw SessionUnavailableException();
  }
}

} // namespace sql
} // namespace fun
