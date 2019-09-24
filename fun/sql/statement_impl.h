#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/binding_base.h"
#include "fun/sql/extraction_base.h"
#include "fun/sql/range.h"
#include "fun/sql/bulk.h"
#include "fun/sql/column.h"
#include "fun/sql/extraction.h"
#include "fun/sql/bulk_extraction.h"
#include "fun/sql/session_impl.h"
#include "fun/ref_counted_object.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/string.h"
#include "fun/base/format.h"
#include "fun/base/exception.h"

#include <vector>
#include <list>
#include <deque>
#include <string>
#include <sstream>

namespace fun {
namespace sql {

class RecordSet;

/**
 * StatementImpl interface that subclasses must implement to define database dependent query execution.
 *
 * StatementImpl's are noncopyable.
 */
class FUN_SQL_API StatementImpl {
 public:
  typedef fun::SharedPtr<StatementImpl> Ptr;

  enum State {
    ST_INITIALIZED,
    ST_COMPILED,
    ST_BOUND,
    ST_PAUSED,
    ST_DONE,
    ST_RESET
  };

  enum Storage {
    STORAGE_DEQUE_IMPL,
    STORAGE_VECTOR_IMPL,
    STORAGE_LIST_IMPL,
    STORAGE_UNKNOWN_IMPL
  };

  enum BulkType {
    /**
     * Bulk mode not defined yet.
     */
    BULK_UNDEFINED,
    /**
     * Binding in bulk mode.
     * If extraction is present in the same statement,
     * it must also be bulk.
     */
    BULK_BINDING,
    /**
     * Extraction in bulk mode.
     * If binding is present in the same statement,
     * it must also be bulk.
     */
    BULK_EXTRACTION,
    /**
     * Bulk forbidden.
     * Happens when the statement has already been
     * configured as non-bulk.
     */
    BULK_FORBIDDEN
  };

  static const String DEQUE;
  static const String VECTOR;
  static const String LIST;
  static const String UNKNOWN;

  static const int USE_CURRENT_DATA_SET = -1;

  static const size_t UNKNOWN_TOTAL_ROW_COUNT;

  /**
   * Creates the StatementImpl.
   */
  StatementImpl(SessionImpl& session);

  /**
   * Destroys the StatementImpl.
   */
  virtual ~StatementImpl();

  /**
   * Appends sql statement (fragments).
   */
  template <typename T>
  void Add(const T& t) {
    ostr_ << t;
  }

  /**
   * Registers the Binding with the StatementImpl.
   */
  void AddBind(BindingBase::Ptr binding);

  /**
   * Unregisters all the bindings having specified name with the StatementImpl.
   * Bindings are released and, if this class was the sole owner, deleted.
   */
  void RemoveBind(const String& name);

  /**
   * Registers objects used for extracting data with the StatementImpl.
   */
  void AddExtract(ExtractionBase::Ptr extraction);

  /**
   * Changes the extractionLimit to extrLimit.
   * Per default no limit (EXTRACT_UNLIMITED) is set.
   */
  void SetExtractionLimit(const Limit& extrLimit);

  /**
   * Create a string version of the sql statement.
   */
  String ToString() const;

  /**
   * Executes a statement. Returns the number of rows
   * extracted for statements returning data or number of rows
   * affected for all other statements (insert, update, delete).
   * If reset is true (default), the underlying bound storage is
   * reset and reused. In case of containers, this means they are
   * cleared and resized to accommodate the number of rows returned by
   * this execution step. When reset is false, data is appended to the
   * bound containers during multiple execute calls.
   */
  size_t Execute(const bool& reset = true);

  /**
   * Resets the statement, so that we can reuse all bindings and re-execute again.
   */
  void Reset();

  /**
   * Returns the state of the Statement.
   */
  State GetState() const;

  /**
   * Sets the storage type for this statement;
   */
  void SetStorage(Storage storage);

  /**
   * Sets the storage type for this statement;
   */
  void SetStorage(const String& storage);

  /**
   * Returns the storage type for this statement.
   */
  Storage GetStorage() const;

  /**
   * Returns the number of extraction storage buffers associated
   * with the statement.
   */
  size_t ExtractionCount() const;

  /**
   * Returns the number of data sets associated with the statement.
   */
  size_t DataSetCount() const;

  /**
   * Returns the current data set.
   */
  size_t GetCurrentDataSet() const;

 protected:
  /**
   * Hints the implementation that it is an insert statement
   */
  virtual void InsertHint();

  /**
   * Returns number of columns returned by query.
   */
  virtual size_t ReturnedColumnCount() const = 0;

  /**
   * Returns the number of affected rows.
   * Used to find out the number of rows affected by insert, delete or update.
   *
   * Some back-ends may return a negative number in certain circumstances (e.g.
   * some ODBC drivers when this function is called after a select statement
   * execution).
   */
  virtual int AffectedRowCount() const = 0;

  //TODO 함수명을 어떤식으로하는게 좋을까??
  /**
   * Returns column meta data.
   */
  virtual const MetaColumn& MetaColumnAt(size_t pos, size_t data_set) const = 0;

  /**
   * Returns column meta data.
   */
  const MetaColumn& MetaColumnAt(const String& name) const;

  /**
   * Returns true if a call to Next() will return data.
   *
   * Note that the implementation must support
   * several consecutive calls to HasNext without data getting lost,
   * ie. HasNext(); HasNext(); Next() must be equal to HasNext(); Next();
   */
  virtual bool HasNext() = 0;

  /**
   * Retrieves the next row or set of rows from the resultset and
   * returns the number of rows retrieved.
   *
   * Will throw, if the resultset is empty.
   * Expects the statement to be compiled and bound.
   */
  virtual size_t Next() = 0;

  /**
   * Returns true if another bind is possible.
   */
  virtual bool CanBind() const = 0;

  /**
   * Returns true if another compile is possible.
   */
  virtual bool CanCompile() const = 0;

  /**
   * Compiles the statement, doesn't bind yet.
   */
  virtual void CompileImpl() = 0;

  /**
   * Binds parameters.
   */
  virtual void BindImpl() = 0;

  /**
   * Returns the concrete extractor used by the statement.
   */
  virtual ExtractionBase::ExtractorPtr GetExtractor() = 0;

  /**
   * Returns the const reference to extractions vector.
   */
  const ExtractionBaseVec& GetExtractions() const;

  /**
   * Returns the reference to extractions vector.
   */
  ExtractionBaseVec& GetExtractions();

  /**
   * Sets the ExtractorBase at the extractors.
   */
  void FixupExtraction();

  /**
   * Returns the extraction limit value.
   */
  Limit::SizeT GetExtractionLimit();

  /**
   * Returns the extraction limit.
   */
  const Limit& extractionLimit() const;

  /**
   * Returns the number of columns that the extractors handle.
   */
  size_t ExtractedColumnCount(int data_set = USE_CURRENT_DATA_SET) const;

  /**
   * Returns the number of rows extracted for the data set.
   * Default value (USE_CURRENT_DATA_SET) indicates current data set (if any).
   */
  size_t ExtractedRowCount(int data_set = USE_CURRENT_DATA_SET) const;

  /**
   * Returns the number of rows extracted so far for the data set.
   * Default value indicates current data set (if any).
   */
  size_t SubTotalRowCount(int data_set = USE_CURRENT_DATA_SET) const;

    //@ deprecated
  /**
   * Replaced with SubTotalRowCount() and GetTotalRowCount().
   */
  size_t TotalRowCount() const;

  /**
   * Returns the total number of rows.
   * The number of rows reported is independent of filtering.
   * If the total row count has not been set externally
   * (either implicitly or explicitly through sql), the value
   * returned shall only be accurate if the statement limit
   * is less than or equal to the total row count.
   */
  size_t GetTotalRowCount() const;

  /*
   * Explicitly sets the total row count.
   */
  void SetTotalRowCount(size_t count);

  /**
   * Returns true if extractors can be created.
   */
  virtual bool CanMakeExtractors();

  /**
   * Determines the type of the internal extraction container and
   * calls the extraction creation function (AddInternalExtract)
   * with appropriate data type and container type arguments.
   *
   * This function is only called in cases when there is data
   * returned by query, but no data storage supplied by user.
   *
   * The type of the internal container is determined in the
   * following order:
   * 1. If statement has the container type set, the type is used.
   * 2. If statement does not have the container type set,
   *    session is queried for container type setting. If the
   *    session container type setting is found, it is used.
   * 3. If neither session nor statement have the internal
   *    container type set, std::deque is used.
   *
   * Supported internal extraction container types are:
   * - std::deque (default)
   * - std::vector
   * - std::list
   */
  virtual void MakeExtractors(size_t count);

  /**
   * Create extractors for the specified dataset
   */
  void MakeExtractors(size_t count, const Position& position);

  /**
   * Returns session associated with this statement.
   */
  SessionImpl& GetSession();

  /**
   * Returns the concrete binder used by the statement.
   */
  virtual BindingBase::BinderPtr GetBinder() = 0;

  /**
   * Returns the const reference to bindings vector.
   */
  const BindingBaseVec& GetBindings() const;

  /**
   * Returns the reference to bindings.
   */
  BindingBaseVec& GetBindings();

  /**
   * Sets the BinderBase at the bindings.
   */
  void FixupBinding();

  /**
   * Resets binding so it can be reused again.
   */
  void ResetBinding();

  /**
   * Returns true if the statement is stored procedure.
   * Used as a help to determine whether to automatically create the
   * internal extractions when no outside extraction is supplied.
   * The reason for this function is to prevent unnecessary internal
   * extraction creation in cases (behavior exhibited by some ODBC drivers)
   * when there is data available from the stored procedure call
   * statement execution but no external extraction is supplied (as is
   * usually the case when stored procedures are called). In such cases
   * no storage is needed because output parameters serve as storage.
   * At the Data framework level, this function always returns false.
   * When connector-specific behavior is desired, it should be overriden
   * by the statement implementation.
   */
  virtual bool IsStoredProcedure() const;

  /**
   * Returns the next data set index, or throws NoSqlException if the last
   * data set was reached.
   */
  size_t ActivateNextDataSet();

  /**
   * Returns the previous data set index, or throws NoSqlException if the last
   * data set was reached.
   */
  size_t ActivatePreviousDataSet();

  //TODO 이름을 좀더 의미 있게 바꿔주어야하지 않을까??
  /**
   * Activate first data set
   */
  void FirstDataSet();

  /**
   * Returns true if there are data sets not activated yet.
   */
  bool HasMoreDataSets() const;

 private:
  /**
   * Compiles the statement.
   */
  void Compile();

  /**
   * Binds the statement, if not yet bound.
   */
  void Bind();

  /**
   * Executes with an upper limit set. Returns the number of rows
   * extracted for statements returning data or number of rows
   * affected for all other statements (insert, update, delete).
   */
  size_t ExecuteWithLimit();

  /**
   * Executes without an upper limit set. Returns the number of rows
   * extracted for statements returning data or number of rows
   * affected for all other statements (insert, update, delete).
   */
  size_t ExecuteWithoutLimit();

  /**
   * Resets extraction so it can be reused again.
   */
  void ResetExtraction();

  template <typename C>
  SharedPtr<InternalExtraction<C> >
  CreateExtract(const MetaColumn& mc, size_t position) {
    C* data = new C;
    Column<C>* col = new Column<C>(mc, data);
    return new InternalExtraction<C>(*data, col, uint32(position));
  }

  template <typename C>
  SharedPtr<InternalBulkExtraction<C> >
  CreateBulkExtract(const MetaColumn& mc, size_t position) {
    C* data = new C;
    Column<C>* col = new Column<C>(mc, data);
    return new InternalBulkExtraction<C>(*data,
      col,
      static_cast<uint32>(GetExtractionLimit()),
      Position(static_cast<uint32>(position)));
  }

  /**
   * Creates and adds the internal extraction.
   *
   * The decision about internal extraction container is done
   * in a following way:
   *
   * If this statement has storage_ member set, that setting
   * overrides the session setting for storage, otherwise the
   * session setting is used.
   * If neither this statement nor the session have the storage
   * type set, std::deque is the default container type used.
   */
  template <typename T>
  void AddInternalExtract(const MetaColumn& mc, size_t position) {
    String storage;

    switch (storage_) {
      case STORAGE_DEQUE_IMPL:
        storage = DEQUE; break;
      case STORAGE_VECTOR_IMPL:
        storage = VECTOR; break;
      case STORAGE_LIST_IMPL:
        storage = LIST; break;
      case STORAGE_UNKNOWN_IMPL:
        storage = AnyCast<String>(GetSession().GetProperty("storage"));
        break;
    }

    if (storage.IsEmpty()) {
      storage = VECTOR;
    }

    if (0 == icompare(DEQUE, storage)) {
      if (!IsBulkExtraction()) {
        AddExtract(CreateExtract<std::deque<T> >(mc, position));
      } else {
        AddExtract(CreateBulkExtract<std::deque<T> >(mc, position));
      }
    } else if (0 == icompare(VECTOR, storage)) {
      if (!IsBulkExtraction()) {
        AddExtract(CreateExtract<std::vector<T> >(mc, position));
      } else {
        AddExtract(CreateBulkExtract<std::vector<T> >(mc, position));
      }
    } else if (0 == icompare(LIST, storage)) {
      if (!IsBulkExtraction()) {
        AddExtract(CreateExtract<std::list<T> >(mc, position));
      } else {
        AddExtract(CreateBulkExtract<std::list<T> >(mc, position));
      }
    }
  }

  /**
   * Returns true if the value in [col, row] is null.
   */
  bool IsNull(size_t col, size_t row) const;

  /**
   * Forbids bulk operations.
   */
  void ForbidBulk();

  /**
   * Sets the bulk binding flag.
   */
  void SetBulkBinding();

  /**
   * Sets the bulk extraction flag and extraction limit.
   */
  void SetBulkExtraction(const Bulk& l);

  /**
   * Resets the bulk extraction and binding flag.
   */
  void ResetBulk();

  /**
   * Returns true if statement can be set to bind data in bulk.
   * Once bulk binding is set for a statement, it can be
   * neither altered nor mixed with non-bulk mode binding.
   */
  bool BulkBindingAllowed() const;

  /**
   * Returns true if statement can be set to extract data in bulk.
   * Once bulk extraction is set for a statement, it can be
   * neither altered nor mixed with non-bulk mode extraction.
   */
  bool BulkExtractionAllowed() const;

  /**
   * Returns true if statement is set to bind data in bulk.
   */
  bool IsBulkBinding() const;

  /**
   * Returns true if statement is set to extract data in bulk.
   */
  bool IsBulkExtraction() const;

  /**
   * Returns true if connector and session support bulk operation.
   */
  bool IsBulkSupported() const;

  /**
   * Formats the sql string by filling in placeholders with values from supplied vector.
   */
  void FormatSql(std::vector<Any>& arguments);

  void AssignSubTotal(bool reset);

  StatementImpl(const StatementImpl& stmt);
  StatementImpl& operator = (const StatementImpl& stmt);

  typedef std::vector<size_t> CountVec;

  State state_;
  Limit extr_limit_;
  size_t lower_limit_;
  std::vector<int> columns_extracted_;
  SessionImpl& session_;
  Storage storage_;
  std::ostringstream ostr_;
  BindingBaseVec bindings_;
  ExtractionBaseVecVec extractors_;
  size_t cur_data_set_;
  BulkType bulk_binding_;
  BulkType bulk_extraction_;
  CountVec sub_total_row_count_;
  size_t total_row_count_;

  friend class Statement;
  friend class RecordSet;
};


//
// inlines
//

inline void StatementImpl::AddBind(BindingBase::Ptr binding) {
  fun_check_ptr(binding);
  bindings_.push_back(binding);
}

inline String StatementImpl::ToString() const {
  return ostr_.str();
}

inline const BindingBaseVec& StatementImpl::GetBindings() const {
  return bindings_;
}

inline BindingBaseVec& StatementImpl::GetBindings() {
  return bindings_;
}

inline const ExtractionBaseVec& StatementImpl::GetExtractions() const {
  fun_check(cur_data_set_ < extractors_.size());
  return extractors_[cur_data_set_];
}

inline ExtractionBaseVec& StatementImpl::GetExtractions() {
  fun_check(cur_data_set_ < extractors_.size());
  return extractors_[cur_data_set_];
}

inline StatementImpl::State StatementImpl::GetState() const {
  return state_;
}

inline SessionImpl& StatementImpl::GetSession() {
  return session_;
}

inline void StatementImpl::SetStorage(Storage storage) {
  storage_ = storage;
}

inline StatementImpl::Storage StatementImpl::GetStorage() const {
  return storage_;
}

inline size_t StatementImpl::GetTotalRowCount() const {
  if (UNKNOWN_TOTAL_ROW_COUNT == total_row_count_)
    return SubTotalRowCount();
  else
    return total_row_count_;
}

inline size_t StatementImpl::TotalRowCount() const {
  return GetTotalRowCount();
}

inline void StatementImpl::SetTotalRowCount(size_t count) {
  total_row_count_ = count;
}

inline size_t StatementImpl::ExtractionCount() const {
  return static_cast<size_t>(extractions().size());
}

inline size_t StatementImpl::DataSetCount() const {
  return static_cast<size_t>(extractors_.size());
}

inline bool StatementImpl::IsStoredProcedure() const {
  return false;
}

inline bool StatementImpl::IsNull(size_t col, size_t row) const {
  try {
    return extractions().at(col)->IsNull(row);
  } catch (std::out_of_range& ex) { //TODO corrent exception handling...
    throw RangeException(ex.what());
  }
}

inline size_t StatementImpl::GetCurrentDataSet() const {
  return cur_data_set_;
}

inline Limit::SizeT StatementImpl::GetExtractionLimit() {
  return extr_limit_.value();
}

inline const Limit& StatementImpl::GetExtractionLimit() const {
  return extr_limit_;
}

inline void StatementImpl::ForbidBulk() {
  bulk_binding_ = BULK_FORBIDDEN;
  bulk_extraction_ = BULK_FORBIDDEN;
}

inline void StatementImpl::SetBulkBinding() {
  bulk_binding_ = BULK_BINDING;
}

inline bool StatementImpl::BulkBindingAllowed() const {
  return BULK_UNDEFINED == bulk_binding_ ||
    BULK_BINDING == bulk_binding_;
}

inline bool StatementImpl::BulkExtractionAllowed() const {
  return BULK_UNDEFINED == bulk_extraction_ ||
    BULK_EXTRACTION == bulk_extraction_;
}

inline bool StatementImpl::IsBulkBinding() const {
  return BULK_BINDING == bulk_binding_;
}

inline bool StatementImpl::IsBulkExtraction() const {
  return BULK_EXTRACTION == bulk_extraction_;
}

inline void StatementImpl::ResetBulk() {
  bulk_extraction_ = BULK_UNDEFINED;
  bulk_binding_ = BULK_UNDEFINED;
  SetExtractionLimit(Limit(static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), false, false));
}

inline bool StatementImpl::IsBulkSupported() const {
  return session_.GetFeature("bulk");
}

inline bool StatementImpl::HasMoreDataSets() const {
  return GetCurrentDataSet() + 1 < DataSetCount();
}

inline void StatementImpl::FirstDataSet() {
  cur_data_set_ = 0;
}

} // namespace sql
} // namespace fun
