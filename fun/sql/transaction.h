#pragma once

#include "fun/logger.h"
#include "fun/sql/session.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * Transaction helps with transactions in domain logic.
 * When an Transaction object is created, it first checks whether a
 * transaction is in progress. If not, a new transaction is created.
 * When the Transaction is destroyed, and Commit() has been called,
 * nothing is done. Otherwise, the current transaction is rolled back.
 * See Transaction for more details and purpose of this template.
 */
class FUN_SQL_API Transaction {
 public:
  /**
   * Creates the Transaction and starts it, using the given database session and
   * logger.
   */
  Transaction(fun::sql::Session& session, fun::Logger::Ptr logger = nullptr);

  /**
   * Creates the Transaction, using the given database session.
   * If start is true, transaction is started, otherwise begin() must be called
   * to start the transaction.
   */
  Transaction(fun::sql::Session& session, bool start);

  /**
   * Creates the Transaction, using the given database session, transactor and
   * logger. The transactor type must provide operator () overload taking
   * non-const Session reference as an argument.
   *
   * When transaction is created using this constructor, it is executed and
   * committed automatically. If no error occurs, Rollback is disabled and does
   * not occur at destruction time. If an error occurs resulting in exception
   * being thrown, the transaction is rolled back and exception propagated to
   * calling code.
   *
   * Example usage:
   *
   * struct Transactor {
   *    void operator () (Session& session) const {
   *      // do something ...
   *    }
   * };
   *
   * Transactor tr;
   * Transaction tn(session, tr);
   */
  template <typename T>
  Transaction(fun::sql::Session& session, T& t,
              fun::Logger::Ptr logger = nullptr)
      : session_(session), logger_(logger) {
    try {
      Transact(t);
    } catch (...) {
      if (logger_) {
        logger_->error("Error executing transaction.");
      }
      Rollback();
      throw;
    }
  }

  /**
   * Destroys the Transaction.
   * Rolls back the current database transaction if it has not been committed
   * (by calling Commit()), or rolled back (by calling Rollback()).
   *
   * If an exception is thrown during Rollback, the exception is logged
   * and no further action is taken.
   */
  ~Transaction();

  /**
   * Sets the transaction isolation level.
   */
  void SetIsolation(uint32 ti);

  /**
   * Returns the transaction isolation level.
   */
  uint32 GetIsolation();

  /**
   * Returns true if the transaction isolation level corresponding
   * to the supplied bitmask is supported.
   */
  bool HasIsolation(uint32 ti);

  /**
   * Returns true if the transaction isolation level corresponds
   * to the supplied bitmask.
   */
  bool IsIsolation(uint32 ti);

  /**
   * Executes and, if do_commit is true, commits the transaction.
   * Passing true value for Commit disables Rollback during destruction
   * of this Transaction object.
   */
  void Execute(const String& sql, bool do_commit = true);

  /**
   * Executes all the sql statements supplied in the vector and, after the last
   * one is successfully executed, commits the transaction.
   * If an error occurs during execution, transaction is rolled back.
   * Passing true value for Commit disables Rollback during destruction
   * of this Transaction object.
   */
  void Execute(const std::vector<String>& sql);

  /**
   * Executes the transactor and, unless transactor throws an exception,
   * commits the transaction.
   */
  template <typename T>
  void Transact(const T& t) {
    if (!IsActive()) {
      begin();
    }
    t(session_);
    Commit();
  }

  /**
   * Commits the current transaction.
   */
  void Commit();

  /**
   * Rolls back the current transaction.
   */
  void Rollback();

  /**
   * Returns false after the transaction has been committed or rolled back,
   * true if the transaction is ongoing.
   */
  bool IsActive();

  /**
   * Sets the logger for this transaction.
   * Transaction does not take the ownership of the pointer.
   */
  void SetLogger(fun::Logger::Ptr logger);

 private:
  /**
   * Begins the transaction if the session is already not in transaction.
   * Otherwise does nothing.
   */
  void begin();

  Session session_;
  Logger::Ptr logger_;

 public:
  Transaction() = delete;
  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;
};

//
// inlines
//

inline bool Transaction::IsActive() { return session_.IsInTransaction(); }

inline void Transaction::SetIsolation(uint32 ti) {
  session_.SetTransactionIsolation(ti);
}

inline uint32 Transaction::GetIsolation() {
  return session_.GetTransactionIsolation();
}

inline bool Transaction::HasIsolation(uint32 ti) {
  return session_.IsTransactionIsolation(ti);
}

inline bool Transaction::IsIsolation(uint32 ti) {
  return session_.IsTransactionIsolation(ti);
}

inline void Transaction::SetLogger(fun::Logger::Ptr logger) {
  logger_ = logger;
}

}  // namespace sql
}  // namespace fun
