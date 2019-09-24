#include "fun/sql/transaction.h"
#include "fun/base/exception.h"

namespace fun {
namespace sql {

Transaction::Transaction(fun::sql::Session& session, fun::Logger::Ptr logger)
  : session_(session), logger_(logger) {
  Begin();
}

Transaction::Transaction(fun::sql::Session& session, bool start)
  : session_(session), logger_(nullptr) {
  if (start) {
    Begin();
  }
}

Transaction::~Transaction() {
  try {
    if (session_.IsInTransaction()) {
      try {
        if (logger_) {
          logger_->LogDebug("Rolling back transaction.");
        }

        session_.Rollback();
      } catch (fun::Exception& e) {
        if (logger_) {
          logger_->LogError("Error while rolling back database transaction: %s", e.GetDisplayText());
        }
      } catch (...) {
        if (logger_) {
          logger_->LogError("Error while rolling back database transaction.");
        }
      }
    }
  } catch (...) {
    fun_unexpected();
  }
}

void Transaction::Begin() {
  if (!session_.IsInTransaction()) {
    session_.Begin();
  } else {
    throw InvalidAccessException("Transaction in progress.");
  }
}

void Transaction::Execute(const String& sql, bool do_commit) {
  if (!session_.IsInTransaction()) {
    session_.Begin();
  }

  session_ << sql, Keywords::now;

  if (do_commit) {
    Commit();
  }
}

void Transaction::Execute(const std::vector<String>& sql) {
  try {
    std::vector<String>::const_iterator it = sql.Begin();
    std::vector<String>::const_iterator end = sql.end();
    for (; it != end; ++it) {
      Execute(*it, it + 1 == end ? true : false);
    }
    return;
  } catch (Exception& e) {
    if (logger_) {
      logger_->Log(e);
    }
  }

  Rollback();
}

void Transaction::Commit() {
  if (logger_) {
    logger_->LogDebug("Committing transaction.");
  }

  session_.Commit();
}

void Transaction::Rollback() {
  if (logger_) {
    logger_->LogDebug("Rolling back transaction.");
  }

  session_.Rollback();
}

} // namespace sql
} // namespace fun
