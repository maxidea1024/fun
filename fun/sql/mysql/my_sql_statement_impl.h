#pragma once

#include "fun/sql/mysql/mysql.h"
#include "fun/sql/mysql/session_impl.h"
#include "fun/sql/mysql/binder.h"
#include "fun/sql/mysql/extractor.h"
#include "fun/sql/mysql/statement_executor.h"
#include "fun/sql/mysql/result_metadata.h"
#include "fun/sql/statement_impl.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/format.h"

namespace fun {
namespace sql {
namespace mysql {

/**
 * Implements statement functionality needed for MySQL
 */
class FUN_MYSQL_API MySqlStatementImpl : public fun::sql::StatementImpl {
 public:
  /**
   * Creates the MySqlStatementImpl.
   */
  MySqlStatementImpl(SessionImpl& s);

  /**
   * Destroys the MySqlStatementImpl.
   */
  ~MySqlStatementImpl();

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
  virtual const MetaColumn& MetaColumnAt(size_t pos, size_t data_set) const;

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
  enum {
    NEXT_DONTKNOW,
    NEXT_TRUE,
    NEXT_FALSE
  };

  StatementExecutor stmt_;
  ResultMetadata metadata_;
  Binder::Ptr binder_;
  Extractor::Ptr extractor_;
  int32 has_next_;
};

} // namespace mysql
} // namespace sql
} // namespace fun
