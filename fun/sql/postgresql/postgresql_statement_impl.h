#pragma once

#include "fun/base/format.h"
#include "fun/base/shared_ptr.h"
#include "fun/sql/postgresql/binder.h"
#include "fun/sql/postgresql/extractor.h"
#include "fun/sql/postgresql/postgresql.h"
#include "fun/sql/postgresql/session_impl.h"
#include "fun/sql/postgresql/statement_executor.h"
#include "fun/sql/statement_impl.h"

namespace fun {
namespace sql {
namespace postgresql {

/**
 * Implements statement functionality needed for PostgreSQL
 */
class FUN_POSTGRESQL_API PostgreSqlStatementImpl
    : public fun::sql::StatementImpl {
 public:
  /**
   * Creates the PostgreSqlStatementImpl.
   */
  PostgreSqlStatementImpl(SessionImpl& session_impl);

  /**
   * Destroys the PostgreSqlStatementImpl.
   */
  ~PostgreSqlStatementImpl();

 protected:
  /**
   * Returns number of columns returned by query.
   */
  virtual size_t ReturnedColumnCount() const;

  /**
   * Returns the number of affected rows.
   * Used to find out the number of rows affected by insert, delete or update.
   */
  virtual int AffectedRowCount() const;

  /**
   * Returns column meta data.
   */
  virtual const MetaColumn& MetaColumnAt(size_t position,
                                         size_t data_set) const;

  /**
   * Returns true if a call to next() will return data.
   */
  virtual bool HasNext();

  /**
   * Retrieves the next row from the resultset.
   * Will throw, if the resultset is empty.
   */
  virtual size_t Next();

  /**
   * Returns true if a valid statement is set and we can bind.
   */
  virtual bool CanBind() const;

  /**
   * Returns true if another compile is possible.
   */
  virtual bool CanCompile() const;

  /**
   * Compiles the statement, doesn't bind yet
   */
  virtual void CompileImpl();

  /**
   * Binds parameters
   */
  virtual void BindImpl();

  /**
   * Returns the concrete extractor used by the statement.
   */
  virtual fun::sql::ExtractorBase::Ptr GetExtractor();

  /**
   * Returns the concrete binder used by the statement.
   */
  virtual fun::sql::BinderBase::Ptr GetBinder();

 private:
  enum NextState { NEXT_DONTKNOW, NEXT_TRUE, NEXT_FALSE };

  StatementExecutor statement_executor_;
  Binder::Ptr binder_;
  // Need for COPY queries
  Binder::Ptr bulk_binder_;

  Extractor::Ptr extractor_;
  NextState has_next_;
};

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
