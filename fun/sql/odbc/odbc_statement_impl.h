#pragma once

#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/session_impl.h"
#include "fun/sql/odbc/binder.h"
#include "fun/sql/odbc/extractor.h"
#include "fun/sql/odbc/preparator.h"
#include "fun/sql/odbc/odbc_meta_column.h"
#include "fun/sql/statement_impl.h"
#include "fun/sql/column.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/format.h"
#include <sstream>

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqltypes.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * Implements statement functionality needed for ODBC
 */
class FUN_ODBC_API OdbcStatementImpl : public fun::sql::StatementImpl {
 public:
  /**
   * Creates the OdbcStatementImpl.
   */
  OdbcStatementImpl(SessionImpl& session);

  /**
   * Destroys the OdbcStatementImpl.
   */
  ~OdbcStatementImpl();

 protected:
  /**
   * Returns number of columns returned by query.
   */
  size_t ReturnedColumnCount() const;

  /**
   * Returns the number of affected rows.
   * Used to find out the number of rows affected by insert or update.
   */
  int AffectedRowCount() const;

  /**
   * Returns column meta data.
   */
  const MetaColumn& metaColumn(size_t pos, size_t data_set) const;

  /**
   * Returns true if a call to Next() will return data.
   */
  bool HasNext();

  /**
   * Retrieves the next row or set of rows from the resultset.
   * Returns the number of rows retrieved.
   * Will throw, if the resultset is empty.
   */
  size_t Next();

  /**
   * Returns true if a valid statement is set and we can bind.
   */
  bool CanBind() const;

  /**
   * Returns true if another compile is possible.
   */
  bool CanCompile() const;

  /**
   * Compiles the statement, doesn't bind yet.
   * Does nothing if the statement has already been compiled.
   */
  void CompileImpl();

  /**
   * Binds all parameters and executes the statement.
   */
  void BindImpl();

  /**
   * Returns true if extractors can be created.
   */
  virtual bool CanMakeExtractors();

  /**
   * Create extractors for the specified dataset
   */
  virtual void MakeExtractors(size_t count);

  /**
   * Returns the concrete extractor used by the statement.
   */
  ExtractionBase::ExtractorPtr GetExtractor();

  /**
   * Returns the concrete binder used by the statement.
   */
  BindingBase::BinderPtr GetBinder();

  /**
   * Returns the sql string as modified by the driver.
   */
  String GetNativeSQL();

 protected:
  virtual void InsertHint();

 private:
  typedef fun::sql::BindingBaseVec Bindings;
  typedef fun::SharedPtr<Binder> BinderPtr;
  typedef fun::sql::ExtractionBaseVec Extractions;
  typedef fun::SharedPtr<Preparator> PreparatorPtr;
  typedef std::vector<PreparatorPtr> PreparatorVec;
  typedef fun::SharedPtr<Extractor> ExtractorPtr;
  typedef std::vector<ExtractorPtr> ExtractorVec;
  typedef std::vector<OdbcMetaColumn*> ColumnPtrVec;
  typedef std::vector<ColumnPtrVec> ColumnPtrVecVec;

  static const String INVALID_CURSOR_STATE;

  /**
   * Closes the cursor and resets indicator variables.
   */
  void Clear();

  /**
   * Binds parameters.
   */
  void DoBind();

  /**
   * Returns true if sql is a stored procedure call.
   */
  bool IsStoredProcedure() const;

  /**
   * Prepares placeholders for data returned by statement.
   * It is called during statement compilation for sql statements
   * returning data. For stored procedures returning datasets,
   * it is called upon the first check for data availability
   * (see HasNext() function).
   */
  void DoPrepare();

  /**
   * Returns true if statement returns data.
   */
  bool HasData() const;

  /**
   * Fetches the next row of data.
   */
  void MakeStep();

  /**
   * Returns true if there is a row fetched but not yet extracted.
   */
  bool NextRowReady() const;

  /**
   * Called whenever SQLExecute returns SQL_NEED_DATA. This is expected
   * behavior for PB_AT_EXEC binding mode.
   */
  void PutData();

  void GetData();

  bool AddPreparator(bool add_always = true);
  void FillColumns(size_t data_set_pos);
  void CheckError(SQLRETURN rc, const String& msg="");
  bool NextResultSet();

  const SQLHDBC& connection_;
  const StatementHandle stmt_;
  PreparatorVec preparations_;
  BinderPtr binder_;
  ExtractorVec extractors_;
  bool step_called_;
  int next_response_;
  ColumnPtrVecVec column_ptrs_;
  bool prepared_;
  mutable size_t affected_row_count_;
  bool can_compile_;
  bool insert_hint_;
};


//
// inlines
//

inline ExtractionBase::ExtractorPtr OdbcStatementImpl::GetExtractor() {
  fun_check_dbg(GetCurrentDataSet() < extractors_.size());
  fun_check_dbg(extractors_[GetCurrentDataSet()]);
  return extractors_[GetCurrentDataSet()];
}

inline BindingBase::BinderPtr OdbcStatementImpl::GetBinder() {
  fun_check_dbg(!binder_.IsNull());
  return binder_;
}

inline size_t OdbcStatementImpl::ReturnedColumnCount() const {
  fun_check_dbg(GetCurrentDataSet() < preparations_.size());
  fun_check_dbg(preparations_[GetCurrentDataSet()]);
  return static_cast<size_t>(preparations_[GetCurrentDataSet()]->columns());
}

inline bool OdbcStatementImpl::HasData() const {
  return (ReturnedColumnCount() > 0);
}

inline bool OdbcStatementImpl::NextRowReady() const {
  return (!Utility::IsError(static_cast<SQLRETURN>(next_response_)));
}

inline bool OdbcStatementImpl::CanCompile() const {
  return can_compile_;
}

} // namespace odbc
} // namespace sql
} // namespace fun
