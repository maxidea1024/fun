#pragma once

#include "fun/base/shared_ptr.h"
#include "fun/sql/meta_column.h"
#include "fun/sql/sqlite/binder.h"
#include "fun/sql/sqlite/extractor.h"
#include "fun/sql/sqlite/sqlite.h"
#include "fun/sql/statement_impl.h"

extern "C" {
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
}

namespace fun {
namespace sql {
namespace sqlite {

/**
 * Implements statement functionality needed for SQLite
 */
class FUN_SQLITE_API SQLiteStatementImpl : public fun::sql::StatementImpl {
 public:
  /**
   * Creates the SQLiteStatementImpl.
   */
  SQLiteStatementImpl(fun::sql::SessionImpl& session, sqlite3* db);

  /**
   * Destroys the SQLiteStatementImpl.
   */
  ~SQLiteStatementImpl();

 protected:
  /**
   * Returns number of columns returned by query.
   */
  size_t ReturnedColumnCount() const;

  /**
   * Returns the number of affected rows.
   * Used to find out the number of rows affected by insert, delete or update.
   * All changes are counted, even if they are later undone by a ROLLBACK or
   * ABORT. Changes associated with creating and dropping tables are not
   * counted.
   */
  int AffectedRowCount() const;

  /**
   * Returns column meta data.
   */
  const MetaColumn& MetaColumnAt(size_t pos, size_t data_set) const;

  /**
   * Returns true if a call to next() will return data.
   */
  bool HasNext();

  /**
   * Retrieves the next row from the resultset and returns 1.
   * Will throw, if the resultset is empty.
   */
  size_t Next();

  /**
   * Returns true if a valid statement is set and we can bind.
   */
  bool CanBind() const;

  /**
   * Returns true if statement can compile.
   */
  bool CanCompile() const;

  /**
   * Compiles the statement, doesn't bind yet.
   * Returns true if the statement was successfully compiled.
   * The way SQLite handles batches of statements is by compiling
   * one at a time and returning a pointer to the next one.
   * The remainder of the statement is kept in a string
   * buffer pointed to by leftover_ member.
   */
  void CompileImpl();

  /**
   * Binds parameters
   */
  void BindImpl();

  /**
   * Returns the concrete extractor used by the statement.
   */
  ExtractionBase::ExtractorPtr GetExtractor();

  /**
   * Returns the concrete binder used by the statement.
   */
  BindingBase::BinderPtr GetBinder();

 private:
  /**
   * Removes the stmt_
   */
  void Clear();

  typedef fun::SharedPtr<Binder> BinderPtr;
  typedef fun::SharedPtr<Extractor> ExtractorPtr;
  typedef fun::sql::BindingBaseVec Bindings;
  typedef fun::sql::ExtractionBaseVec Extractions;
  typedef std::vector<fun::sql::MetaColumn> MetaColumnVec;
  typedef std::vector<MetaColumnVec> MetaColumnVecVec;
  typedef fun::SharedPtr<String> StrPtr;
  typedef Bindings::iterator BindIt;

  sqlite3* db_;
  sqlite3_stmt* stmt_;
  bool step_called_;
  int next_response_;
  BinderPtr binder_;
  ExtractorPtr extractor_;
  MetaColumnVecVec columns_;
  int affected_row_count_;
  StrPtr leftover_;
  BindIt bind_begin_;
  bool can_bind_;
  bool is_extracted_;
  bool can_compile_;

  static const int FUN_SQLITE_INV_ROW_CNT;
};

//
// inlines
//

inline ExtractionBase::ExtractorPtr SQLiteStatementImpl::GetExtractor() {
  return extractor_;
}

inline BindingBase::BinderPtr SQLiteStatementImpl::GetBinder() {
  return binder_;
}

inline bool SQLiteStatementImpl::CanBind() const { return can_bind_; }

inline bool SQLiteStatementImpl::CanCompile() const { return can_compile_; }

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
