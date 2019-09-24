#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/session.h"
#include "fun/sql/extraction.h"
#include "fun/sql/bulk_extraction.h"
#include "fun/sql/statement.h"
#include "fun/sql/row_iterator.h"
#include "fun/sql/row_filter.h"
#include "fun/sql/lob.h"
#include "fun/base/string.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/exception.h"
#include <ostream>
#include <limits>

namespace fun {
namespace sql {

/**
 * RecordSet provides access to data returned from a query.
 * Data access indices (row and column) are 0-based, as usual in C++.
 *
 * Recordset provides navigation methods to iterate through the
 * recordset, retrieval methods to extract data, and methods
 * to get metadata (type, etc.) about columns.
 *
 * To work with a RecordSet, first create a Statement, execute it, and
 * create the RecordSet from the Statement, as follows:
 *
 *     Statement select(session);
 *     select << "SELECT * FROM Person";
 *     select.Execute();
 *     RecordSet rs(select);
 *
 * The shorter way to do the above is following:
 *
 *     RecordSet rs(session, "SELECT * FROM Person"[, new SimpleRowFormatter]);
 *
 * The third (optional) argument passed to the Recordset constructor is a RowFormatter
 * implementation. The formatter is used in conjunction with << operator for recordset
 * data formatting.
 *
 * The number of rows in the RecordSet can be limited by specifying
 * a limit for the Statement.
 */
class FUN_SQL_API RecordSet : private Statement {
 public:
  typedef std::map<size_t, Row*> RowMap;
  typedef const RowIterator ConstIterator;
  typedef RowIterator Iterator;

  using Statement::IsNull;
  using Statement::SubTotalRowCount;
  using Statement::TotalRowCount;

  /**
   * Creates the RecordSet.
   */
  explicit RecordSet( const Statement& statement,
                      RowFormatter::Ptr row_formatter = nullptr);

  /**
   * Creates the RecordSet.
   */
  RecordSet(Session& session,
    const String& query,
    RowFormatter::Ptr row_formatter = nullptr);

  /**
   * Creates the RecordSet.
   */
  RecordSet(Session& session,
    const String& query,
    const RowFormatter& row_formatter);

  /**
   * Creates the RecordSet.
   */
  template <typename RF>
  RecordSet(Session& session, const String& query, const RF& row_formatter)
    : Statement((session << query, Keywords::now)),
      current_row_(0),
      begin_(new RowIterator(this, 0 == ExtractedRowCount())),
      end_(new RowIterator(this, true)) {
    SetRowFormatter(Keywords::format(row_formatter));
  }

  /**
   * Copy-creates the recordset.
   */
  RecordSet(const RecordSet& other);

  /**
   * Move-creates the recordset.
   */
  RecordSet(RecordSet&& other);

  /**
   * Destroys the RecordSet.
   */
  ~RecordSet();

  /**
   * Assigns the row formatter to the statement and all recordset rows.
   */
  void SetRowFormatter(RowFormatter::Ptr row_formatter);

  /**
   * Assignment operator.
   */
  Statement& operator = (const Statement& stmt);

  /**
   * Returns the number of rows in the RecordSet.
   * The number of rows reported is dependent on filtering.
   * Due to the need for filter conditions checking,
   * this function may suffer significant performance penalty
   * for large recordsets, so it should be used judiciously.
   * Use TotalRowCount() to obtain the total number of rows.
   */
  size_t RowCount() const;

  /**
   * Returns the number of rows extracted during the last statement
   * execution.
   * The number of rows reported is independent of filtering.
   */
  size_t ExtractedRowCount() const;

  /**
   * Returns the number of columns in the recordset.
   */
  size_t ColumnCount() const;

  /**
   * Returns the reference to the first Column with the specified name.
   */
  template <typename C>
  const Column<C>& ColumnAt(const String& name) const {
    if (IsBulkExtraction()) {
      typedef InternalBulkExtraction<C> E;
      return ColumnAtImpl<C,E>(name);
    } else {
      typedef InternalExtraction<C> E;
      return ColumnAtImpl<C,E>(name);
    }
  }

  /**
   * Returns the reference to column at specified position.
   */
  template <typename C>
  const Column<C>& ColumnAt(size_t pos) const {
    if (IsBulkExtraction()) {
      typedef InternalBulkExtraction<C> E;
      return ColumnAtImpl<C,E>(pos);
    } else {
      typedef InternalExtraction<C> E;
      return ColumnAtImpl<C,E>(pos);
    }
  }

  /**
   * Returns reference to row at position pos.
   * Rows are lazy-created and cached.
   */
  Row& RowAt(size_t pos);

  /**
   * Returns the reference to data value at [col] location.
   */
  template <typename T>
  const T& ValueAt(size_t col) const;

  /**
   * Returns the reference to data value at [col, row] location.
   */
  template <typename T>
  const T& ValueAt(size_t col, size_t data_row, bool use_filter = true) const {
    if (use_filter && IsFiltered() && !IsAllowed(data_row)) {
      throw InvalidAccessException("Row not allowed");
    }

    switch (storage()) {
      case STORAGE_VECTOR: {
        typedef typename std::vector<T> C;
        return ColumnAt<C>(col).ValueAt(data_row);
      }
      case STORAGE_LIST: {
        typedef typename std::list<T> C;
        return ColumnAt<C>(col).ValueAt(data_row);
      }
      case STORAGE_DEQUE:
      case STORAGE_UNKNOWN: {
        typedef typename std::deque<T> C;
        return ColumnAt<C>(col).ValueAt(data_row);
      }
      default:
        throw IllegalStateException("Invalid storage setting.");
    }
  }

  /**
   * Returns the reference to data value at named column, row location.
   */
  template <typename T>
  const T& ValueAt(const String& name, size_t data_row, bool use_filter = true) const {
    if (use_filter && IsFiltered() && !IsAllowed(data_row)) {
      throw InvalidAccessException("Row not allowed");
    }

    switch (storage()) {
      case STORAGE_VECTOR: {
        typedef typename std::vector<T> C;
        return ColumnAt<C>(name).ValueAt(data_row);
      }
      case STORAGE_LIST: {
        typedef typename std::list<T> C;
        return ColumnAt<C>(name).ValueAt(data_row);
      }
      case STORAGE_DEQUE:
      case STORAGE_UNKNOWN: {
        typedef typename std::deque<T> C;
        return ColumnAt<C>(name).ValueAt(data_row);
      }
      default:
        throw IllegalStateException("Invalid storage setting.");
    }
  }

  /**
   * Returns the data value at column, row location.
   */
  fun::dynamic::Var ValueAt(size_t col, size_t row, bool check_filtering = true) const;

  /**
   * Returns the data value at named column, row location.
   */
  fun::dynamic::Var ValueAt(const String& name, size_t row, bool check_filtering = true) const;

  /**
   * Returns the value in the named column of the current row
   * if the value is not NULL, or default_value otherwise.
   */
  template <typename T>
  fun::dynamic::Var NVL(const String& name, const T& default_value = T()) const {
    if (IsNull(name)) {
      return fun::dynamic::Var(default_value);
    } else {
      return ValueAt(name, current_row_);
    }
  }

  /**
   * Returns the value in the given column of the current row
   * if the value is not NULL, or default_value otherwise.
   */
  template <typename T>
  fun::dynamic::Var NVL(size_t index, const T& default_value = T()) const {
    if (IsNull(index, current_row_)) {
      return fun::dynamic::Var(default_value);
    } else {
      return ValueAt(index, current_row_);
    }
  }

  /**
   * Returns the const row iterator.
   */
  ConstIterator& begin() const;

  /**
   * Returns the const row iterator.
   */
  ConstIterator& end() const;

  /**
   * Returns the row iterator.
   */
  Iterator begin();

  /**
   * Returns the row iterator.
   */
  Iterator end();

  /**
   * Moves the row cursor to the first row.
   *
   * Returns true if there is at least one row in the RecordSet,
   * false otherwise.
   */
  bool MoveFirst();

  /**
   * Moves the row cursor to the next row.
   *
   * Returns true if the row is available, or false
   * if the end of the record set has been reached and
   * no more rows are available.
   */
  bool MoveNext();

  /**
   * Moves the row cursor to the previous row.
   *
   * Returns true if the row is available, or false
   * if there are no more rows available.
   */
  bool MovePrevious();

  /**
   * Moves the row cursor to the last row.
   *
   * Returns true if there is at least one row in the RecordSet,
   * false otherwise.
   */
  bool MoveLast();

  /*
   * Don't hide base class method.
   */
  using Statement::Reset;

  /**
   * Resets the RecordSet and assigns a new statement.
   * Should be called after the given statement has been reset,
   * assigned a new sql statement, and executed.
   *
   * Does not remove the associated RowFilter or RowFormatter.
   */
  RecordSet& Reset(const Statement& stmt);

  /**
   * Returns the value in the named column of the current row.
   */
  fun::dynamic::Var ValueAt(const String& name) const;

  /**
   * Returns the value in the given column of the current row.
   */
  fun::dynamic::Var ValueAt(size_t index) const;

  /**
   * Returns the value in the named column of the current row.
   */
  fun::dynamic::Var operator [] (const String& name) const;

  /**
   * Returns the value in the named column of the current row.
   */
  fun::dynamic::Var operator [] (size_t index) const;

  /**
   * Returns the type for the column at specified position.
   */
  MetaColumn::ColumnDataType ColumnTypeAt(size_t pos) const;

  /**
   * Returns the type for the column with specified name.
   */
  MetaColumn::ColumnDataType ColumnTypeAt(const String& name) const;

  /**
   * Returns column name for the column at specified position.
   */
  const String& ColumnNameAt(size_t pos) const;

  /**
   * Returns column maximum length for the column at specified position.
   */
  size_t ColumnLengthAt(size_t pos) const;

  /**
   * Returns column maximum length for the column with specified name.
   */
  size_t ColumnLengthAt(const String& name) const;

  /**
   * Returns column precision for the column at specified position.
   * Valid for floating point fields only (zero for other data types).
   */
  size_t ColumnPrecisionAt(size_t pos) const;

  /**
   * Returns column precision for the column with specified name.
   * Valid for floating point fields only (zero for other data types).
   */
  size_t ColumnPrecisionAt(const String& name) const;

  /**
   * Returns true if column value of the current row is null.
   */
  bool IsNull(const String& name) const;

  /**
   * Returns true if column value of the current row is null.
   */
  bool IsNull(size_t& colNo) const;

  /**
   * Copies the column names to the target output stream.
   * Copied string is formatted by the current RowFormatter.
   */
  std::ostream& CopyNames(std::ostream& os) const;

  /**
   * Formats names using the current RowFormatter.
   */
  void FormatNames() const;

  /**
   * Copies the data values to the supplied output stream.
   * The data set to be copied is starting at the specified offset
   * from the recordset beginning. The number of rows to be copied
   * is specified by length argument.
   * An invalid combination of offset/length arguments shall
   * cause RangeException to be thrown.
   * Copied string is formatted by the current RowFormatter.
   */
  std::ostream& CopyValues( std::ostream& os,
                            size_t offset = 0,
                            size_t length = RowIterator::POSITION_END) const;

  /**
   * Formats values using the current RowFormatter.
   * The data set to be formatted is starting at the specified offset
   * from the recordset beginning. The number of rows to be copied
   * is specified by length argument.
   * An invalid combination of offset/length arguments shall
   * cause RangeException to be thrown.
   */
  void FormatValues(size_t offset, size_t length) const;

  /**
   * Copies the column names and values to the target output stream.
   * Copied strings are formatted by the current RowFormatter.
   */
  std::ostream& Copy( std::ostream& os,
                      size_t offset = 0,
                      size_t length = RowIterator::POSITION_END) const;

  /**
   * Returns true if recordset is filtered.
   */
  bool IsFiltered() const;

 private:
  RecordSet();

  /**
   * Returns the position of the column with specified name.
   */
  template <typename C, typename E>
  size_t ColumnPositionAt(const String& name) const {
    typedef typename C::value_type T;
    typedef const E* ExtractionVecPtr;

    bool typeFound = false;

    const ExtractionBaseVec& rExtractions = extractions();
    ExtractionBaseVec::const_iterator it = rExtractions.begin();
    ExtractionBaseVec::const_iterator itEnd = rExtractions.end();

    for (; it != itEnd; ++it) {
      ExtractionVecPtr extraction = dynamic_cast<ExtractionVecPtr>(it->get());
      if (extraction) {
        typeFound = true;
        const Column<C>& col = extraction->column();
        if (0 == fun::icompare(name, col.name())) {
          return col.Position();
        }
      }
    }

    if (typeFound) {
      throw NotFoundException(fun::Format("Column name: %s", name));
    } else {
      throw NotFoundException(fun::Format("Column type: %s, Container type: %s, name: %s",
        String(typeid(T).name()), String(typeid(ExtractionVecPtr).name()), name));
    }
  }

  /**
   * Returns the reference to the first Column with the specified name.
   */
  template <typename C, typename E>
  const Column<C>& ColumnAtImpl(const String& name) const {
    return ColumnAtImpl<C,E>(ColumnPositionAt<C,E>(name));
  }

  /**
   * Returns the reference to column at specified position.
   */
  template <typename C, typename E>
  const Column<C>& ColumnAtImpl(size_t pos) const {
    typedef typename C::value_type T;
    typedef const E* ExtractionVecPtr;

    const ExtractionBaseVec& rExtractions = extractions();

    size_t s = rExtractions.size();
    if (0 == s || pos >= s) {
      throw RangeException(fun::Format("Invalid column index: %z", pos));
    }

    ExtractionVecPtr extraction = dynamic_cast<ExtractionVecPtr>(rExtractions[pos].get());

    if (extraction) {
      return extraction->column();
    } else {
      throw fun::BadCastException(fun::Format("RecordSet::ColumnAtImpl(%z) type cast failed!\nTarget type:\t%s"
        "\nTarget container type:\t%s\nSource container type:\t%s\nSource abstraction type:\t%s",
        pos,
        String(typeid(T).name()),
        String(typeid(ExtractionVecPtr).name()),
        rExtractions[pos]->type(),
        String(typeid(rExtractions[pos].get()).name())));
    }
  }

  size_t StorageRowCount() const;

  /**
   * Returns true if the specified row is allowed by the
   * currently active filter.
   */
  bool IsAllowed(size_t row) const;

  /**
   * Sets the filter for the RecordSet.
   */
  void Filter(const fun::RefCountedPtr<RowFilter>& filter);

  /**
   * Returns the filter associated with the RecordSet.
   */
  const fun::RefCountedPtr<RowFilter>& GetFilter() const;

  size_t current_row_;
  RowIterator* begin_;
  RowIterator* end_;
  RowMap row_map_;
  fun::RefCountedPtr<RowFilter> filter_;

  friend class RowIterator;
  friend class RowFilter;
};


//
// inlines
//

inline FUN_SQL_API std::ostream& operator << (std::ostream &os, const RecordSet& rs) {
  return rs.Copy(os);
}

inline size_t RecordSet::ExtractedRowCount() const {
  return ExtractedRowCount();
}

inline size_t RecordSet::ColumnCount() const {
  return static_cast<size_t>(extractions().size());
}

inline Statement& RecordSet::operator = (const Statement& stmt) {
  reset(stmt);
  return *this;
}

inline fun::dynamic::Var RecordSet::ValueAt(const String& name)  const {
  return ValueAt(name, current_row_);
}

inline fun::dynamic::Var RecordSet::ValueAt(size_t index) const {
  return ValueAt(index, current_row_);
}

inline fun::dynamic::Var RecordSet::operator [] (const String& name) const {
  return ValueAt(name, current_row_);
}

inline fun::dynamic::Var RecordSet::operator [] (size_t index) const {
  return ValueAt(index, current_row_);
}

template <typename T>
inline const T& RecordSet::ValueAt(size_t col) const {
  return ValueAt<T>(col, current_row_);
}

inline MetaColumn::ColumnDataType RecordSet::ColumnTypeAt(size_t pos) const {
  return MetaColumnAt(static_cast<uint32>(pos)).Type();
}

inline MetaColumn::ColumnDataType RecordSet::ColumnTypeAt(const String& name) const {
  return MetaColumnAt(name).Type();
}

inline const String& RecordSet::ColumnNameAt(size_t pos) const {
  return MetaColumnAt(static_cast<uint32>(pos)).name();
}

inline size_t RecordSet::ColumnLengthAt(size_t pos) const {
  return MetaColumnAt(static_cast<uint32>(pos)).length();
}

inline size_t RecordSet::ColumnLengthAt(const String& name) const {
  return MetaColumnAt(name).length();
}

inline size_t RecordSet::ColumnPrecisionAt(size_t pos) const {
  return MetaColumnAt(static_cast<uint32>(pos)).precision();
}

inline size_t RecordSet::columnPrecision(const String& name) const {
  return MetaColumnAt(name).precision();
}

inline bool RecordSet::IsNull(const String& name) const {
  return IsNull(MetaColumnAt(name).position(), current_row_);
}

inline bool RecordSet::IsNull(size_t& colNo) const {
  return IsNull(colNo, current_row_);
}

inline RecordSet::ConstIterator& RecordSet::begin() const {
  return *begin_;
}

inline RecordSet::ConstIterator& RecordSet::end() const {
  return *end_;
}

inline RecordSet::Iterator RecordSet::begin() {
  return *begin_;
}

inline RecordSet::Iterator RecordSet::end() {
  return *end_;
}

inline const fun::RefCountedPtr<RowFilter>& RecordSet::GetFilter() const {
  return filter_;
}

inline void RecordSet::FormatNames() const {
  (*begin_)->FormatNames();
}

inline size_t RecordSet::StorageRowCount() const {
  return GetImpl()->ExtractedRowCount();
}

} // namespace sql
} // namespace fun
