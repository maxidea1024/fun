#pragma once

#include <algorithm>
#include "fun/async_method.h"
#include "fun/async_result.h"
#include "fun/base/format.h"
#include "fun/base/mutex.h"
#include "fun/base/shared_ptr.h"
#include "fun/sql/binding.h"
#include "fun/sql/bulk.h"
#include "fun/sql/range.h"
#include "fun/sql/row.h"
#include "fun/sql/simple_row_formatter.h"
#include "fun/sql/sql.h"
#include "fun/sql/statement_impl.h"

namespace fun {
namespace sql {

class BindingBase;
class ExtractionBase;
class Session;
class Limit;

/**
 * A Statement is used to execute sql statements.
 * It does not contain code of its own.
 * Its main purpose is to forward calls to the concrete StatementImpl stored
 * inside. Statement execution can be synchronous or asynchronous. Synchronous
 * execution is achieved through Execute() call, while asynchronous is achieved
 * through ExecuteAsync() method call. An asynchronously executing statement
 * should not be copied during the execution.
 *
 * Note:
 *
 * Once set as asynchronous through 'async' manipulator, statement remains
 * asynchronous for all subsequent execution calls, both Execute() and
 * ExecuteAsync(). However, calling ExecutAsync() on a synchronous statement
 * shall execute asynchronously but without altering the underlying statement's
 * synchronous nature.
 *
 * Once asynchronous, a statement can be reverted back to synchronous state in
 * two ways:
 *
 *   1) By calling SetAsync(false)
 *   2) By means of 'sync' or 'reset' manipulators
 *
 * See individual functions documentation for more details.
 *
 * Statement owns the RowFormatter, which can be provided externally through
 * SetFormatter() member function. If no formatter is externally supplied to the
 * statement, the SimpleRowFormatter is lazy created and used.
 */
class FUN_SQL_API Statement {
 public:
  typedef void (*Manipulator)(Statement&);

  typedef ActiveResult<size_t> Result;
  typedef SharedPtr<Result> ResultPtr;
  typedef ActiveMethod<size_t, bool, StatementImpl> AsyncExecMethod;
  typedef SharedPtr<AsyncExecMethod> AsyncExecMethodPtr;

  static const int WAIT_FOREVER = -1;

  enum Storage {
    STORAGE_DEQUE = StatementImpl::STORAGE_DEQUE_IMPL,
    STORAGE_VECTOR = StatementImpl::STORAGE_VECTOR_IMPL,
    STORAGE_LIST = StatementImpl::STORAGE_LIST_IMPL,
    STORAGE_UNKNOWN = StatementImpl::STORAGE_UNKNOWN_IMPL
  };

  /**
   * Creates the Statement.
   */
  Statement(StatementImpl::Ptr impl);

  /**
   * Creates the Statement for the given Session.
   *
   * The following:
   *
   *     Statement stmt(sess);
   *     stmt << "SELECT * FROM Table", ...
   *
   * is equivalent to:
   *
   *     Statement stmt(sess << "SELECT * FROM Table", ...);
   *
   * but in some cases better readable.
   */
  explicit Statement(Session& session);

  /**
   * Destroys the Statement.
   */
  ~Statement();

  /**
   * Copy constructor.
   * If the statement has been executed asynchronously and has not been
   * synchronized prior to copy operation (i.e. is copied while executing),
   * this constructor shall synchronize it.
   */
  Statement(const Statement& stmt);

  /**
   * Move constructor.
   * If the statement has been executed asynchronously and has not been
   * synchronized prior to move operation (i.e. is moved while executing),
   * this constructor shall synchronize it.
   */
  Statement(Statement&& stmt);

  /**
   * Assignment operator.
   * If the statement has been executed asynchronously and has not been
   * synchronized prior to assignment operation (i.e. is assigned while
   * executing), this operator shall synchronize it.
   */
  Statement& operator=(const Statement& stmt);

  /**
   * Swaps the statement with another one.
   */
  void Swap(Statement& other);

  /**
   * Move assignment operator.
   * If the statement has been executed asynchronously and has not been
   * synchronized prior to assignment operation (i.e. is assigned while
   * executing), this operator shall synchronize it.
   */
  Statement& operator=(Statement&& stmt);

  /**
   * Swaps the statement with another one.
   */
  void Move(Statement&& stmt);

  /**
   * Concatenates data with the sql statement string.
   */
  template <typename T>
  Statement& operator<<(const T& t) {
    impl_->Add(t);
    return *this;
  }

  /**
   * Handles manipulators, such as now, async, etc.
   */
  Statement& operator,(Manipulator manip);

  /**
   * Registers the Binding with the Statement by calling AddBind().
   */
  Statement& operator,(BindingBase::Ptr bind);

  /**
   * Registers a single binding with the statement.
   */
  Statement& AddBind(BindingBase::Ptr bind);

  /**
   * Removes the all the bindings with specified name from the statement.
   */
  void RemoveBind(const String& name);

  /**
   * Registers the Binding vector with the Statement.
   */
  Statement& operator,(BindingBaseVec& bindVec);

  /**
   * Registers binding container with the Statement.
   */
  template <typename C>
  Statement& AddBinding(C& bindingCont, bool do_reset) {
    if (do_reset) {
      impl_->ResetBinding();
    }

    typename C::iterator itAB = bindingCont.begin();
    typename C::iterator itABEnd = bindingCont.end();
    for (; itAB != itABEnd; ++itAB) AddBind(*itAB);
    return *this;
  }

  /**
   * Adds a binding to the Statement. This can be used to implement
   * generic binding mechanisms and is a nicer syntax for:
   *
   *     statement , Bind(value);
   */
  template <typename C>
  Statement& Bind(const C& value) {
    (*this), Keywords::Bind(value);
    return *this;
  }

  /**
   * Registers objects used for extracting data with the Statement by
   * calling AddExtract().
   */
  Statement& operator,(ExtractionBase::Ptr extract);

  /**
   * Registers the extraction vector with the Statement.
   * The vector is registered at position 0 (i.e. for the first returned data
   * set).
   */
  Statement& operator,(ExtractionBaseVec& extVec);

  /**
   * Registers the vector of extraction vectors with the Statement.
   */
  Statement& operator,(ExtractionBaseVecVec& extVecVec);

  /**
   * Registers extraction container with the Statement.
   */
  template <typename C>
  Statement& AddExtraction(C& val, bool do_reset) {
    if (do_reset) impl_->ResetExtraction();
    typename C::iterator itAE = val.begin();
    typename C::iterator itAEEnd = val.end();
    for (; itAE != itAEEnd; ++itAE) AddExtract(*itAE);
    return *this;
  }

  /**
   * Registers container of extraction containers with the Statement.
   */
  template <typename C>
  Statement& AddExtractions(C& val) {
    impl_->ResetExtraction();
    typename C::iterator itAEV = val.begin();
    typename C::iterator itAEVEnd = val.end();
    for (; itAEV != itAEVEnd; ++itAEV) AddExtraction(*itAEV, false);
    return *this;
  }

  /**
   * Registers a single extraction with the statement.
   */
  Statement& AddExtract(ExtractionBase::Ptr extract);

  /**
   * Sets the bulk execution mode (both binding and extraction) for this
   * statement.Statement must not have any extractors or binders set at the
   * time when this operator is applied.
   * Failure to adhere to the above constraint shall result in
   * InvalidAccessException.
   */
  Statement& operator,(const Bulk& bulk);

  /**
   * Sets the bulk execution mode (both binding and extraction) for this
   * statement.Statement must not have any extractors or binders set at the
   * time when this operator is applied.
   * Additionally, this function requires limit to be set in order to
   * determine the bulk size.
   * Failure to adhere to the above constraints shall result in
   * InvalidAccessException.
   */
  Statement& operator,(BulkFnType);

  /**
   * Sets a limit on the maximum number of rows a select is allowed to return.
   *
   * Set per default to zero to Limit::LIMIT_UNLIMITED, which disables the
   * limit.
   */
  Statement& operator,(const Limit& extrLimit);

  /**
   * Sets the row formatter for the statement.
   */
  Statement& operator,(RowFormatter::Ptr row_formatter);

  /**
   * Sets a an extraction range for the maximum number of rows a select is
   * allowed to return.
   *
   * Set per default to Limit::LIMIT_UNLIMITED which disables the range.
   */
  Statement& operator,(const Range& extrRange);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(char value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(uint8 value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(int8 value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(uint16 value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(int16 value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(uint32 value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(int32 value);

#ifndef FUN_LONG_IS_64_BIT
  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(long value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(unsigned long value);
#endif
  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(uint64 value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(int64 value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(double value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(float value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(bool value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(const String& value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  Statement& operator,(const char* value);

  /**
   * Adds the value to the list of values to be supplied to the sql string
   * formatting function.
   */
  const String& ToString() const;

  /**
   * Executes the statement synchronously or asynchronously.
   * Stops when either a limit is hit or the whole statement was executed.
   * Returns the number of rows extracted from the database (for statements
   * returning data) or number of rows affected (for all other statements).
   * If reset is true (default), associated storage is reset and reused.
   * Otherwise, the results from this execution step are appended.
   * Reset argument has no meaning for unlimited statements that return all
   * rows. If IsAsync() returns  true, the statement is executed asynchronously
   * and the return value from this function is zero.
   * The result of execution (i.e. number of returned or affected rows) can be
   * obtained by calling Wait() on the statement at a later point in time.
   */
  size_t Execute(bool reset = true);

  /**
   * Executes the statement asynchronously.
   * Stops when either a limit is hit or the whole statement was executed.
   * Returns immediately. Calling Wait() (on either the result returned from
   * this call or the statement itself) returns the number of rows extracted or
   * number of rows affected by the statement execution. When executed on a
   * synchronous statement, this method does not alter the statement's
   * synchronous nature.
   */
  const Result& ExecuteAsync(bool reset = true);

  /**
   * Sets the asynchronous flag. If this flag is true, ExecuteAsync() is called
   * from the now() manipulator. This setting does not affect the statement's
   * capability to be executed synchronously by directly calling Execute().
   */
  void SetAsync(bool async = true);

  /**
   * Returns true if statement was marked for asynchronous execution.
   */
  bool IsAsync() const;

  /**
   * Waits for the execution completion for asynchronous statements or
   * returns immediately for synchronous ones. The return value for
   * asynchronous statement is the execution result (i.e. number of
   * rows retrieved). For synchronous statements, the return value is zero.
   */
  size_t Wait(long milliseconds = WAIT_FOREVER) const;

  /**
   * Returns true if the statement was initialized (i.e. not executed yet).
   */
  bool IsInitialized();

  /**
   * Returns true if the statement was paused (a range limit stopped it
   * and there is more work to do).
   */
  bool IsPaused();

  /**
   * Returns true if the statement was completely executed or false if a range
   * limit stopped it and there is more work to do. When no limit is set, it
   * will always return true after calling Execute().
   */
  bool IsDone() const;

  /**
   * Resets the Statement so that it can be filled with a new sql command.
   */
  Statement& Reset(Session& session);

  /**
   * Returns true if statement is in a state that allows the internal storage to
   * be modified.
   */
  bool CanModifyStorage();

  /**
   * Returns the internal storage type for the statement.
   */
  Storage GetStorage() const;

  /**
   * Sets the internal storage type for the statement.
   */
  void SetStorage(const String& storage);

  /**
   * Returns the internal storage type for the statement.
   */
  const String& GetStorage() const;

  /**
   * Returns the number of columns returned for current data set.
   * Default value indicates current data set (if any).
   */
  size_t ExtractedColumnCount(
      int data_set = StatementImpl::USE_CURRENT_DATA_SET) const;

  /**
   * Returns the number of rows returned for current data set during last
   * statement execution. Default value indicates current data set (if any).
   */
  size_t ExtractedRowCount(
      int data_set = StatementImpl::USE_CURRENT_DATA_SET) const;

  /**
   * Returns the number of rows extracted so far for the data set.
   * Default value indicates current data set (if any).
   */
  size_t SubTotalRowCount(
      int data_set = StatementImpl::USE_CURRENT_DATA_SET) const;

  //@ deprecated
  /**
   * Replaced with SubTotalRowCount() and GetTotalRowCount().
   */
  size_t TotalRowCount() const;

  /**
   * Returns the total number of rows in the RecordSet.
   * The number of rows reported is independent of filtering.
   * If the total row count has not been set externally
   * (either explicitly or implicitly through sql), the value
   * returned shall only be accurate if the statement limit
   * is less or equal to the total row count.
   */
  size_t GetTotalRowCount() const;

  /**
   * Explicitly sets the total row count.
   */
  void SetTotalRowCount(size_t TotalRowCount);

  /**
   * Implicitly sets the total row count.
   * The supplied sql must return exactly one column
   * and one row. The returned value must be an unsigned
   * integer. The value is set as the total number of rows.
   */
  void SetTotalRowCount(const String& sql);

  /**
   * Returns the number of extraction storage buffers associated
   * with the current data set.
   */
  size_t ExtractionCount() const;

  /**
   * Returns the number of data sets associated with the statement.
   */
  size_t DataSetCount() const;

  /**
   * Returns the index of the next data set.
   */
  size_t NextDataSet();

  /**
   * Returns the index of the previous data set.
   */
  size_t PreviousDataSet();

  /**
   * Returns false if the current data set index points to the last
   * data set. Otherwise, it returns true.
   */
  bool HasMoreDataSets() const;

  /**
   * Activates the first data set
   */
  size_t FirstDataSet();

  /**
   * Returns the current data set.
   */
  size_t GetCurrentDataSet() const;

  /**
   * Sets the row formatter for this statement.
   * Statement takes the ownership of the formatter.
   */
  void SetRowFormatter(RowFormatter::Ptr row_formatter);

  /**
   * Tells the statement that it is an sinsert one
   */
  void InsertHint();

 protected:
  typedef StatementImpl::Ptr ImplPtr;

  /**
   * Returns the extractions vector.
   */
  const ExtractionBaseVec& extractions() const;

  /**
   * Returns the type for the column at specified position.
   */
  const MetaColumn& metaColumn(size_t pos) const;

  /**
   * Returns the type for the column with specified name.
   */
  const MetaColumn& metaColumn(const String& name) const;

  /**
   * Returns true if the current row value at column pos is null.
   */
  bool IsNull(size_t col, size_t row) const;

  /**
   * Returns true if this statement extracts data in bulk.
   */
  bool IsBulkExtraction() const;

  /**
   * Returns pointer to statement implementation.
   */
  ImplPtr GetImpl() const;

  /**
   * Returns the row formatter for this statement.
   */
  const RowFormatter::Ptr& GetRowFormatter();

  /**
   * Returns the underlying session.
   */
  Session GetSession();

 private:
  /**
   * Asynchronously executes the statement.
   */
  const Result& DoAsyncExec(bool reset = true);

  template <typename T>
  Statement& CommaPODImpl(const T& val) {
    arguments_.push_back(val);
    return *this;
  }

  StatementImpl::Ptr impl_;

  // asynchronous execution related members
  bool async_;
  mutable ResultPtr result_;
  Mutex mutex_;
  AsyncExecMethodPtr async_exec_;
  std::vector<Any> arguments_;
  RowFormatter::Ptr row_formatter_;
  mutable String stmt_string_;
};

//
// inlines

inline size_t Statement::SubTotalRowCount(int data_set) const {
  return impl_->SubTotalRowCount(data_set);
}

inline size_t Statement::GetTotalRowCount() const {
  return impl_->GetTotalRowCount();
}

inline size_t Statement::TotalRowCount() const { return GetTotalRowCount(); }

inline void Statement::SetTotalRowCount(size_t count) {
  impl_->SetTotalRowCount(count);
}

namespace Keywords {

//
// Manipulators
//

/**
 * Enforces immediate execution of the statement.
 * If async_ flag has been set, execution is invoked asynchronously.
 */
inline void FUN_SQL_API now(Statement& statement) { statement.Execute(); }

/**
 * Sets the async_ flag to false, signalling synchronous execution.
 * Synchronous execution is default, so specifying this manipulator
 * only makes sense if async() was called for the statement before.
 */
inline void FUN_SQL_API sync(Statement& statement) {
  statement.SetAsync(false);
}

/**
 * Sets the async_ flag to true, signalling asynchronous execution.
 */
inline void FUN_SQL_API async(Statement& statement) {
  statement.SetAsync(true);
}

/**
 * Sets the internal storage to std::deque.
 * std::deque is default storage, so specifying this manipulator
 * only makes sense if list() or deque() were called for the statement before.
 */
inline void FUN_SQL_API deque(Statement& statement) {
  if (!statement.CanModifyStorage()) {
    throw InvalidAccessException("Storage not modifiable.");
  }

  statement.SetStorage("deque");
}

/**
 * Sets the internal storage to std::vector.
 */
inline void FUN_SQL_API vector(Statement& statement) {
  if (!statement.CanModifyStorage()) {
    throw InvalidAccessException("Storage not modifiable.");
  }

  statement.SetStorage("vector");
}

/**
 * Sets the internal storage to std::list.
 */
inline void FUN_SQL_API list(Statement& statement) {
  if (!statement.CanModifyStorage()) {
    throw InvalidAccessException("Storage not modifiable.");
  }

  statement.SetStorage("list");
}

/**
 * Sets all internal settings to their respective default values.
 */
inline void FUN_SQL_API reset(Statement& statement) {
  if (!statement.CanModifyStorage()) {
    throw InvalidAccessException("Storage not modifiable.");
  }

  statement.SetStorage("deque");
  statement.SetAsync(false);
}

}  // namespace Keywords

//
// inlines
//

inline Statement &Statement::operator,(RowFormatter::Ptr row_formatter) {
  row_formatter_ = row_formatter;
  return *this;
}

inline Statement &Statement::operator,(char value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(uint8 value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(int8 value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(uint16 value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(int16 value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(uint32 value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(int32 value) {
  return CommaPODImpl(value);
}

#ifndef FUN_LONG_IS_64_BIT
inline Statement &Statement::operator,(long value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(unsigned long value) {
  return CommaPODImpl(value);
}
#endif

inline Statement &Statement::operator,(uint64 value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(int64 value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(double value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(float value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(bool value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(const String&value) {
  return CommaPODImpl(value);
}

inline Statement &Statement::operator,(const char*value) {
  return CommaPODImpl(String(value));
}

inline void Statement::RemoveBind(const String& name) {
  impl_->RemoveBind(name);
}

inline Statement &Statement::operator,(BindingBase::Ptr Bind) {
  return AddBind(Bind);
}

inline Statement &Statement::operator,(BindingBaseVec&bindVec) {
  return AddBinding(bindVec, false);
}

inline Statement &Statement::operator,(ExtractionBase::Ptr extract) {
  return AddExtract(extract);
}

inline Statement &Statement::operator,(ExtractionBaseVec&extVec) {
  return AddExtraction(extVec, false);
}

inline Statement &Statement::operator,(ExtractionBaseVecVec&extVecVec) {
  return AddExtractions(extVecVec);
}

inline Statement::ImplPtr Statement::GetImpl() const { return impl_; }

inline const String& Statement::ToString() const {
  return stmt_string_ = impl_->ToString();
}

inline const ExtractionBaseVec& Statement::extractions() const {
  return impl_->extractions();
}

inline const MetaColumn& Statement::metaColumn(size_t pos) const {
  return impl_->metaColumn(pos, impl_->GetCurrentDataSet());
}

inline const MetaColumn& Statement::metaColumn(const String& name) const {
  return impl_->metaColumn(name);
}

inline void Statement::SetStorage(const String& storage) {
  impl_->SetStorage(storage);
}

inline size_t Statement::ExtractionCount() const {
  return impl_->ExtractionCount();
}

inline size_t Statement::ExtractedColumnCount(int data_set) const {
  return impl_->ExtractedColumnCount(data_set);
}

inline size_t Statement::ExtractedRowCount(int data_set) const {
  return impl_->ExtractedRowCount(data_set);
}

inline size_t Statement::DataSetCount() const { return impl_->DataSetCount(); }

inline size_t Statement::NextDataSet() { return impl_->ActivateNextDataSet(); }

inline size_t Statement::PreviousDataSet() {
  return impl_->ActivatePreviousDataSet();
}

inline bool Statement::HasMoreDataSets() const {
  return impl_->HasMoreDataSets();
}

inline Statement::Storage Statement::GetStorage() const {
  return static_cast<Storage>(impl_->GetStorage());
}

inline bool Statement::CanModifyStorage() {
  return (0 == ExtractionCount()) && (IsInitialized() || IsDone());
}

inline bool Statement::IsInitialized() {
  return impl_->GetState() == StatementImpl::ST_INITIALIZED;
}

inline bool Statement::IsPaused() {
  return impl_->GetState() == StatementImpl::ST_PAUSED;
}

inline bool Statement::IsDone() const {
  return impl_->GetState() == StatementImpl::ST_DONE;
}

inline bool Statement::IsNull(size_t col, size_t row) const {
  return impl_->IsNull(col, row);
}

inline bool Statement::IsBulkExtraction() const {
  return impl_->IsBulkExtraction();
}

inline size_t Statement::FirstDataSet() {
  impl_->FirstDataSet();
  return 0;
}

inline size_t Statement::GetCurrentDataSet() const {
  return impl_->GetCurrentDataSet();
}

inline bool Statement::IsAsync() const { return async_; }

inline void Statement::SetRowFormatter(RowFormatter::Ptr row_formatter) {
  row_formatter_ = row_formatter;
}

inline const RowFormatter::Ptr& Statement::GetRowFormatter() {
  if (!row_formatter_) {
    row_formatter_ = new SimpleRowFormatter;
  }
  return row_formatter_;
}

inline void Statement::InsertHint() { impl_->InsertHint(); }

inline void Swap(Statement& s1, Statement& s2) { s1.Swap(s2); }

}  // namespace sql
}  // namespace fun

namespace std {

/**
 * Full template specialization of std:::Swap for Statement
 */
template <>
inline void Swap<fun::sql::Statement>(fun::sql::Statement& s1,
                                      fun::sql::Statement& s2) {
  s1.Swap(s2);
}

}  // namespace std
