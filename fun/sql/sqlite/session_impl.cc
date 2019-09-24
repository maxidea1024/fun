#include "fun/sql/sqlite/session_impl.h"
#include "fun/sql/sqlite/utility.h"
#include "fun/sql/sqlite/sqlite_statement_impl.h"
#include "fun/sql/sqlite/sqlite-exception.h"
#include "fun/sql/session.h"
#include "fun/base/stopwatch.h"
#include "fun/base/string.h"
#include "fun/base/mutex.h"
#include "fun/sql/sql_exception.h"

#if defined(FUN_UNBUNDLED)
#include <sqlite3.h>
#else
#include "sqlite3.h"
#endif

#include <cstdlib>
#include <limits>

#ifndef SQLITE_OPEN_URI
#define SQLITE_OPEN_URI 0
#endif

namespace fun {
namespace sql {
namespace sqlite {

const String SessionImpl::DEFERRED_BEGIN_TRANSACTION("BEGIN DEFERRED");
const String SessionImpl::COMMIT_TRANSACTION("COMMIT");
const String SessionImpl::ABORT_TRANSACTION("ROLLBACK");

SessionImpl::SessionImpl(const String& filename, size_t login_timeout)
  : fun::sql::SessionImplBase<SessionImpl>(filename, login_timeout),
    connector_(Connector::KEY),
    db_(0),
    connected_(false),
    is_transaction_(false) {
  Open();
  SetConnectionTimeout(login_timeout);
  SetProperty("handle", db_);
  AddFeature("autoCommit",
    &SessionImpl::SetAutoCommit,
    &SessionImpl::IsAutoCommit);
  AddProperty("connectionTimeout", &SessionImpl::SetConnectionTimeout, &SessionImpl::GetConnectionTimeout);
}

SessionImpl::~SessionImpl() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

StatementImpl::Ptr SessionImpl::CreateStatementImpl() {
  fun_check_ptr(db_);
  return new SQLiteStatementImpl(*this, db_);
}

void SessionImpl::Begin() {
  fun::Mutex::ScopedLock l(mutex_);
  SQLiteStatementImpl tmp(*this, db_);
  tmp.Add(DEFERRED_BEGIN_TRANSACTION);
  tmp.Execute();
  is_transaction_ = true;
}

void SessionImpl::Commit() {
  fun::Mutex::ScopedLock l(mutex_);
  SQLiteStatementImpl tmp(*this, db_);
  tmp.Add(COMMIT_TRANSACTION);
  tmp.Execute();
  is_transaction_ = false;
}

void SessionImpl::Rollback() {
  fun::Mutex::ScopedLock l(mutex_);
  SQLiteStatementImpl tmp(*this, db_);
  tmp.Add(ABORT_TRANSACTION);
  tmp.Execute();
  is_transaction_ = false;
}

void SessionImpl::SetTransactionIsolation(uint32 ti) {
  if (ti != Session::TRANSACTION_READ_COMMITTED) {
    throw fun::InvalidArgumentException("SetTransactionIsolation()");
  }
}

uint32 SessionImpl::GetTransactionIsolation() const {
  return Session::TRANSACTION_READ_COMMITTED;
}

bool SessionImpl::HasTransactionIsolation(uint32 ti) const {
  if (ti == Session::TRANSACTION_READ_COMMITTED) {
    return true;
  }

  return false;
}

bool SessionImpl::IsTransactionIsolation(uint32 ti) const {
  if (ti == Session::TRANSACTION_READ_COMMITTED) {
    return true;
  }

  return false;
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

  try {
    int rc = 0;
    size_t tout = GetLoginTimeout();
    Stopwatch sw; sw.Start();
    while (true) {
      rc = sqlite3_open_v2(GetConnectionString().c_str(), &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, NULL);
      if (rc == SQLITE_OK) {
        break;
      }

      if (sw.ElapsedSeconds() >= tout) {
        close();
        fun_check(db_);
        Utility::ThrowException(db_, rc);
      } else {
        Thread::sleep(10);
      }
    }
  } catch (SQLiteException& e) {
    throw ConnectionFailedException(e.GetDisplayText());
  } catch (AssertionViolationException& e) {
    throw ConnectionFailedException(e.GetDisplayText());
  }

  connected_ = true;
}

void SessionImpl::Close() {
  if (db_) {
    int result = 0;
    int times = 10;
    do {
      result = sqlite3_close_v2(db_);
    } while (SQLITE_BUSY == result && --times > 0);

    if (SQLITE_BUSY == result && times == 0) {
      times = 10;
      sqlite3_stmt *stmt = NULL;
      do {
        stmt = sqlite3_next_stmt(db_, NULL);
        if (stmt && sqlite3_stmt_busy(stmt)) {
          sqlite3_finalize(stmt);
        }
      } while (stmt != NULL && --times > 0);
      sqlite3_close_v2(db_);
    }
    db_ = 0;
  }

  connected_ = false;
}

void SessionImpl::Reset() {
  // NOOP
}

bool SessionImpl::IsConnected() const {
  return connected_;
}

void SessionImpl::SetConnectionTimeout(size_t timeout) {
  if(timeout <= std::numeric_limits<int>::max()/1000) {
    int tout = 1000 * static_cast<int>(timeout);
    int rc = sqlite3_busy_timeout(db_, tout);
    if (rc != 0) Utility::ThrowException(db_, rc);
    timeout_ = tout;
  } else {
    throw RangeException("Occurred integer overflow because of timeout value.");
  }
}

void SessionImpl::SetConnectionTimeout(const String& /*prop*/, const fun::Any& value) {
  SetConnectionTimeout(fun::RefAnyCast<size_t>(value));
}

fun::Any SessionImpl::GetConnectionTimeout(const String& /*prop*/) const {
  return fun::Any(timeout_/1000);
}

void SessionImpl::SetAutoCommit(const String&, bool) {
  // The problem here is to decide whether to call commit or rollback
  // when autocommit is set to true. Hence, it is best not to implement
  // this explicit call and only implicitly support autocommit setting.
  throw NotImplementedException(
    "SQLite autocommit is implicit with begin/commit/rollback.");
}

bool SessionImpl::IsAutoCommit(const String&) const {
  fun::Mutex::ScopedLock l(mutex_);
  return (0 != sqlite3_get_autocommit(db_));
}

// NOTE: Utility::GetDbHandle() has been moved here from Utility.cpp
// as a workaround for a failing AnyCast with Clang.
// See <https://github.com/funproject/fun/issues/578>
// for a discussion.
sqlite3* Utility::GetDbHandle(const Session& session) {
  return AnyCast<sqlite3*>(session.GetProperty("handle"));
}

} // namespace sqlite
} // namespace sql
} // namespace fun
