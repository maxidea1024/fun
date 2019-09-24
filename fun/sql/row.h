#pragma once

#include <ostream>
#include <string>
#include <vector>
#include "fun/base/dynamic/var.h"
#include "fun/base/shared_ptr.h"
#include "fun/sql/row_formatter.h"
#include "fun/sql/sql.h"
#include "fun/tuple.h"

namespace fun {
namespace sql {

class RecordSet;

/**
 * Row class provides a data type for RecordSet iteration purposes.
 * Dereferencing a RowIterator returns Row.
 * Rows are sortable. The sortability is maintained at all times (i.e. there
 * is always at least one column specified as a sorting criteria) .
 * The default and minimal sorting criteria is the first field (position 0).
 * The default sorting criteria can be replaced with any other field by
 * calling ReplaceSortField() member function.
 * Additional fields can be added to sorting criteria, in which case the
 * field precedence corresponds to addition order (i.e. later added fields
 * have lower sorting precedence).
 * These features make Row suitable for use with standard sorted
 * containers and algorithms. The main constraint is that all the rows from
 * a Set that is being sorted must have the same sorting criteria (i.e., the
 * same Set of fields must be in sorting criteria in the same order). Since rows
 * don't know about each other, it is the programmer's responsibility to ensure
 * this constraint is satisfied. Field names are a shared pointer to a vector of
 * strings. For efficiency sake, a constructor taking a shared pointer to names
 * vector argument is provided. The stream operator is provided for Row data
 * type as a free-standing function.
 */
class FUN_SQL_API Row {
 public:
  typedef RowFormatter::NameVec NameVec;
  typedef RowFormatter::NameVecPtr NameVecPtr;
  typedef RowFormatter::ValueVec ValueVec;

  enum ComparisonType {
    COMPARE_AS_EMPTY,
    COMPARE_AS_INTEGER,
    COMPARE_AS_FLOAT,
    COMPARE_AS_STRING
  };

  typedef Tuple<size_t, ComparisonType> SortTuple;
  /**
   * The type for map holding fields used for sorting criteria.
   * Fields are added sequentially and have precedence that
   * corresponds to field adding sequence order (rather than field's
   * position in the row).
   * This requirement rules out use of std::map due to its sorted nature.
   */
  typedef std::vector<SortTuple> SortMap;
  typedef SharedPtr<SortMap> SortMapPtr;

  /**
   * Creates the Row.
   */
  Row();

  /**
   * Creates the Row.
   */
  Row(NameVecPtr names, const RowFormatter::Ptr& formatter = nullptr);

  /**
   * Creates the Row.
   */
  Row(NameVecPtr names, const SortMapPtr& sort_map,
      const RowFormatter::Ptr& formatter = nullptr);

  /**
   * Destroys the Row.
   */
  ~Row();

  /**
   * Returns the reference to data value at column location.
   */
  fun::dynamic::Var& Get(size_t col);

  /**
   * Returns the reference to data value at column location.
   */
  fun::dynamic::Var& operator[](size_t col);

  /**
   * Returns the reference to data value at named column location.
   */
  fun::dynamic::Var& operator[](const String& name);

  /**
   * Returns the reference to data value at column location.
   */
  const fun::dynamic::Var& Get(size_t col) const;

  /**
   * Returns the reference to data value at column location.
   */
  const fun::dynamic::Var& operator[](size_t col) const;

  /**
   * Returns the reference to data value at named column location.
   */
  const fun::dynamic::Var& operator[](const String& name) const;

  /**
   * Appends the value to the row.
   */
  template <typename T>
  void Append(const String& name, const T& val) {
    if (!names_) {
      names_ = new NameVec;
    }
    values_.push_back(val);
    names_->push_back(name);
    if (1 == values_.size()) {
      AddSortField(0);
    }
  }

  /**
   * Assigns the value to the row.
   */
  template <typename T>
  void Set(size_t pos, const T& val) {
    try {
      values_.at(pos) = val;
    } catch (std::out_of_range&) {  // TODO std::vector�� ������� �����Ƿ� ������
                                    // ���� ó���� �ؾ���.
      throw RangeException("Invalid column number.");
    }
  }

  /**
   * Assigns the value to the row.
   */
  template <typename T>
  void Set(const String& name, const T& val) {
    NameVec::iterator it = names_->begin();
    NameVec::iterator end = names_->end();
    for (int i = 0; it != end; ++it, ++i) {
      if (*it == name) {
        return Set(i, val);
      }
    }

    std::ostringstream os;
    os << "Column with name " << name << " not found.";
    throw NotFoundException(os.str());
  }

  /**
   * Returns the number of fields in this row.
   */
  size_t FieldCount() const;

  /**
   * Resets the row by clearing all field names and values.
   */
  void Reset();

  /**
   * Sets the separator.
   */
  void Separator(const String& sep);

  /**
   * Adds the field used for sorting.
   */
  void AddSortField(size_t pos);

  /**
   * Adds the field used for sorting.
   */
  void AddSortField(const String& name);

  /**
   * Removes the field used for sorting.
   */
  void RemoveSortField(size_t pos);

  /**
   * Removes the field used for sorting.
   */
  void RemoveSortField(const String& name);

  /**
   * Replaces the field used for sorting.
   */
  void ReplaceSortField(size_t old_pos, size_t new_pos);

  /**
   * Replaces the field used for sorting.
   */
  void ReplaceSortField(const String& old_name, const String& new_name);

  /**
   * Resets the sorting criteria to field 0 only.
   */
  void ResetSort();

  /**
   * Converts the column names to string.
   */
  const String& NamesToString() const;

  /**
   * Formats the column names.
   */
  void FormatNames() const;

  /**
   * Converts the row values to string and returns the formatted string.
   */
  const String& ValuesToString() const;

  /**
   * Formats the row values.
   */
  void FormatValues() const;

  /**
   * Equality operator.
   */
  bool operator==(const Row& other) const;

  /**
   * Inequality operator.
   */
  bool operator!=(const Row& other) const;

  /**
   * Less-than operator.
   */
  bool operator<(const Row& other) const;

  /**
   * Returns the shared pointer to names vector.
   */
  const NameVecPtr Names() const;

  /**
   * Returns the const reference to values vector.
   */
  const ValueVec& Values() const;

  /**
   * Sets the formatter for this row and takes the
   * shared ownership of it.
   */
  void SetFormatter(const RowFormatter::Ptr& formatter = nullptr);

  /**
   * Returns the reference to the formatter.
   */
  const RowFormatter& GetFormatter() const;

  /**
   * Adds the sorting fields entry and takes the
   * shared ownership of it.
   */
  void SetSortMap(const SortMapPtr& sort_map = nullptr);

  /**
   * Returns the reference to the sorting fields.
   */
  const SortMapPtr& GetSortMap() const;

 private:
  void Init(const SortMapPtr& sort_map, const RowFormatter::Ptr& formatter);

  /**
   * Returns the reference to values vector.
   */
  ValueVec& Values();

  size_t GetPosition(const String& name) const;
  bool IsEqualSize(const Row& other) const;
  bool IsEqualType(const Row& other) const;

  NameVecPtr names_;
  ValueVec values_;
  SortMapPtr sort_map_;
  mutable RowFormatter::Ptr formatter_;
  mutable String name_str_;
  mutable String value_str_;
};

FUN_SQL_API std::ostream& operator<<(std::ostream& os, const Row& row);

//
// inlines
//

inline size_t Row::FieldCount() const {
  return static_cast<size_t>(values_.size());
}

inline void Row::Reset() {
  names_->Clear();
  values_.Clear();
}

inline const Row::NameVecPtr Row::Names() const { return names_; }

inline const Row::ValueVec& Row::Values() const { return values_; }

inline Row::ValueVec& Row::Values() { return values_; }

inline fun::dynamic::Var& Row::operator[](size_t col) { return Get(col); }

inline fun::dynamic::Var& Row::operator[](const String& name) {
  return Get(GetPosition(name));
}

inline const fun::dynamic::Var& Row::operator[](size_t col) const {
  return Get(col);
}

inline const fun::dynamic::Var& Row::operator[](const String& name) const {
  return Get(GetPosition(name));
}

inline const RowFormatter& Row::GetFormatter() const { return *formatter_; }

inline const Row::SortMapPtr& Row::GetSortMap() const { return sort_map_; }

inline const String& Row::ValuesToString() const {
  return formatter_->FormatValues(Values(), value_str_);
}

inline void Row::FormatValues() const {
  return formatter_->FormatValues(Values());
}

}  // namespace sql
}  // namespace fun
