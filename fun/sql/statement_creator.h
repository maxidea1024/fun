#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/session_impl.h"
#include "fun/sql/statement.h"
#include "fun/ref_counted_ptr.h"

namespace fun {
namespace sql {

/**
 * A StatementCreator creates Statements.
 */
class FUN_SQL_API StatementCreator {
 public:
  /**
   * Creates an unitialized StatementCreator.
   */
  StatementCreator();

  /**
   * Creates a StatementCreator.
   */
  StatementCreator(SessionImpl::Ptr session_impl);

  /**
   * Creates a StatementCreator by copying another one.
   */
  StatementCreator(const StatementCreator& other);

  /**
   * Destroys the StatementCreator.
   */
  ~StatementCreator();

  /**
   * Assignment operator.
   */
  StatementCreator& operator = (const StatementCreator& other);

  /**
   * Assignment operator.
   */
  StatementCreator& operator = (fun::RefCountedPtr<SessionImpl> session_impl);

  /**
   * Swaps the StatementCreator with another one.
   */
  void Swap(StatementCreator& other);

  /**
   * Creates a Statement.
   */
  template <typename T>
  Statement operator << (const T& t) {
    if (!session_impl_->IsConnected()) {
      throw NotConnectedException(session_impl_->GetConnectionString());
    }

    Statement stmt(session_impl_->CreateStatementImpl());
    stmt << t;
    return stmt;
  }

 private:
  fun::RefCountedPtr<SessionImpl> session_impl_;
};

} // namespace sql
} // namespace fun
