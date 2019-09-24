#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/meta_column.h"
#include "fun/base/shared_ptr.h"
#include "fun/ref_counted_object.h"

#include <vector>
#include <list>
#include <deque>

namespace fun {
namespace sql {

/**
 * Column class is column data container.
 * Data (a pointer to underlying STL container) is assigned to the class
 * at construction time. Construction with null pointer is not allowed.
 * This class owns the data assigned to it and deletes the storage on destruction.
 */
template <typename C>
class Column {
 public:
  typedef C Container;
  typedef fun::SharedPtr<C> ContainerPtr;
  typedef typename C::const_iterator Iterator;
  typedef typename C::const_reverse_iterator RIterator;
  typedef typename C::size_type Size;
  typedef typename C::value_type Type;

  /**
   * Creates the Column.
   */
  Column(const MetaColumn& meta_column, ContainerPtr data)
    : meta_column_(meta_column),
      data_(data) {
    if (!data_) {
      throw NullPointerException("Container pointer must point to valid storage.");
    }
  }

  /**
   * Creates the Column.
   */
  Column(const Column& col)
    : meta_column_(col.meta_column_),
      data_(col.data_) {}

  /**
   * Destroys the Column.
   */
  ~Column() {}

  /**
   * Assignment operator.
   */
  Column& operator = (const Column& col) {
    Column tmp(col);
    Swap(tmp);
    return *this;
  }

  /**
   * Swaps the column with another one.
   */
  void Swap(Column& other) {
    using std::Swap;
    Swap(meta_column_, other.meta_column_);
    Swap(data_, other.data_);
  }

  /**
   * Returns reference to contained data.
   */
  Container& data() {
    return *data_;
  }

  /**
   * Returns the field value in specified row.
   */
  const Type& Value(size_t row) const {
    try {
      return data_->at(row);
    } catch (std::out_of_range& e) {
      throw RangeException(e.what());
    }
  }

  /**
   * Returns the field value in specified row.
   */
  const Type& operator [] (size_t row) const {
    return value(row);
  }

  /**
   * Returns number of rows.
   */
  Size RowCount() const {
    return data_->size();
  }

  /**
   * Clears and shrinks the storage.
   */
  void Reset() {
    Container().Swap(*data_);
  }

  /**
   * Returns column name.
   */
  const String& name() const {
    return meta_column_.name();
  }

  /**
   * Returns column maximum length.
   */
  size_t length() const {
    return meta_column_.length();
  }

  /**
   * Returns column precision.
   * Valid for floating point fields only (zero for other data types).
   */
  size_t precision() const {
    return meta_column_.precision();
  }

  /**
   * Returns column position.
   */
  size_t position() const {
    return meta_column_.Position();
  }

  /**
   * Returns column type.
   */
  MetaColumn::ColumnDataType type() const {
    return meta_column_.type();
  }

  /**
   * Returns iterator pointing to the beginning of data storage vector.
   */
  Iterator begin() const {
    return data_->begin();
  }

  /**
   * Returns iterator pointing to the end of data storage vector.
   */
  Iterator end() const {
    return data_->end();
  }

 private:
  Column();

  MetaColumn meta_column_;
  ContainerPtr data_;
};

/**
 * The std::vector<bool> specialization for the Column class.
 *
 * This specialization is necessary due to the nature of std::vector<bool>.
 * For details, see the standard library implementation of vector<bool>
 * or
 * S. Meyers: "Effective STL" (Copyright Addison-Wesley 2001),
 * Item 18: "Avoid using vector<bool>."
 *
 * The workaround employed here is using deque<bool> as an
 * internal "companion" container kept in sync with the vector<bool>
 * column data.
 */
template <>
class Column<std::vector<bool> > {
 public:
  typedef std::vector<bool> Container;
  typedef fun::SharedPtr<Container> ContainerPtr;
  typedef Container::const_iterator Iterator;
  typedef Container::const_reverse_iterator RIterator;
  typedef Container::size_type Size;

  /**
   * Creates the Column.
   */
  Column(const MetaColumn& meta_column, Container* data)
    : meta_column_(meta_column),
      data_(data) {
    fun_check_ptr(data_);
    deque_.Assign(data_->begin(), data_->end());
  }

  /**
   * Creates the Column.
   */
  Column(const Column& col)
    : meta_column_(col.meta_column_),
      data_(col.data_) {
    deque_.Assign(data_->begin(), data_->end());
  }

  /**
   * Destroys the Column.
   */
  ~Column() {}

  /**
   * Assignment operator.
   */
  Column& operator = (const Column& col) {
    Column tmp(col);
    Swap(tmp);
    return *this;
  }

  /**
   * Swaps the column with another one.
   */
  void Swap(Column& other) {
    fun::Swap(meta_column_, other.meta_column_);
    fun::Swap(data_, other.data_);
    fun::Swap(deque_, other.deque_);
  }

  /**
   * Returns reference to contained data.
   */
  Container& data() {
    return *data_;
  }

  /**
   * Returns the field value in specified row.
   */
  const bool& value(size_t row) const {
    if (deque_.size() < data_->size()) {
      deque_.resize(data_->size());
    }

    try {
      return deque_.at(row) = data_->at(row);
    } catch (std::out_of_range& e) {
      throw RangeException(e.what());
    }
  }

  /**
   * Returns the field value in specified row.
   */
  const bool& operator [] (size_t row) const {
    return value(row);
  }

  /**
   * Returns number of rows.
   */
  Size RowCount() const {
    return data_->size();
  }

  /**
   * Clears and shrinks the storage.
   */
  void Reset() {
    Container().Swap(*data_);
    deque_.clear();
  }

  /**
   * Returns column name.
   */
  const String& name() const {
    return meta_column_.name();
  }

  /**
   * Returns column maximum length.
   */
  size_t length() const {
    return meta_column_.length();
  }

  /**
   * Returns column precision.
   * Valid for floating point fields only (zero for other data types).
   */
  size_t precision() const {
    return meta_column_.precision();
  }

  /**
   * Returns column position.
   */
  size_t position() const {
    return meta_column_.Position();
  }

  /**
   * Returns column type.
   */
  MetaColumn::ColumnDataType type() const {
    return meta_column_.type();
  }

  /**
   * Returns iterator pointing to the beginning of data storage vector.
   */
  Iterator begin() const {
    return data_->begin();
  }

  /**
   * Returns iterator pointing to the end of data storage vector.
   */
  Iterator end() const {
    return data_->end();
  }

 private:
  Column();

  MetaColumn meta_column_;
  ContainerPtr data_;
  mutable std::deque<bool> deque_;
};

/**
 * Column specialization for std::list
 */
template <typename T>
class Column<std::list<T> > {
 public:
  typedef std::list<T> Container;
  typedef fun::SharedPtr<Container> ContainerPtr;
  typedef typename Container::const_iterator Iterator;
  typedef typename Container::const_reverse_iterator RIterator;
  typedef typename Container::size_type Size;

  /**
   * Creates the Column.
   */
  Column(const MetaColumn& meta_column, std::list<T>* data)
    : meta_column_(meta_column),
      data_(data) {
    fun_check_ptr(data_);
  }

  /**
   * Creates the Column.
   */
  Column(const Column& col)
    : meta_column_(col.meta_column_),
      data_(col.data_) {}

  /**
   * Destroys the Column.
   */
  ~Column() {}

  /**
   * Assignment operator.
   */
  Column& operator = (const Column& col) {
    Column tmp(col);
    Swap(tmp);
    return *this;
  }

  /**
   * Swaps the column with another one.
   */
  void Swap(Column& other) {
    fun::Swap(meta_column_, other.meta_column_);
    fun::Swap(data_, other.data_);
  }

  /**
   * Returns reference to contained data.
   */
  Container& data() {
    return *data_;
  }

  /**
   * Returns the field value in specified row.
   * This is the std::list specialization and std::list
   * is not the optimal solution for cases where random
   * access is needed.
   * However, to allow for compatibility with other
   * containers, this functionality is provided here.
   * To alleviate the problem, an effort is made
   * to start iteration from beginning or end,
   * depending on the position requested.
   */
  const T& value(size_t row) const {
    if (row <= (size_t) (data_->size() / 2))
    {
      Iterator it = data_->begin();
      Iterator itEnd = data_->end();
      for (int i = 0; it != itEnd; ++it, ++i)
        if (i == row) return *it;
    }
    else
    {
      row = data_->size() - row;
      RIterator it = data_->rbegin();
      RIterator itEnd = data_->rend();
      for (int i = 1; it != itEnd; ++it, ++i)
        if (i == row) return *it;
    }

    throw RangeException("Invalid row number.");
  }

  /**
   * Returns the field value in specified row.
   */
  const T& operator [] (size_t row) const {
    return value(row);
  }

  /**
   * Returns number of rows.
   */
  Size RowCount() const {
    return data_->size();
  }

  /**
   * Clears the storage.
   */
  void Reset() {
    data_->clear();
  }

  /**
   * Returns column name.
   */
  const String& name() const {
    return meta_column_.name();
  }

  /**
   * Returns column maximum length.
   */
  size_t length() const {
    return meta_column_.length();
  }

  /**
   * Returns column precision.
   * Valid for floating point fields only (zero for other data types).
   */
  size_t precision() const {
    return meta_column_.precision();
  }

  /**
   * Returns column position.
   */
  size_t position() const {
    return meta_column_.Position();
  }

  /**
   * Returns column type.
   */
  MetaColumn::ColumnDataType type() const {
    return meta_column_.type();
  }

  /**
   * Returns iterator pointing to the beginning of data storage vector.
   */
  Iterator begin() const {
    return data_->begin();
  }

  /**
   * Returns iterator pointing to the end of data storage vector.
   */
  Iterator end() const {
    return data_->end();
  }

 private:
  Column();

  MetaColumn meta_column_;
  ContainerPtr data_;
};

template <typename C>
inline void Swap(Column<C>& c1, Column<C>& c2) {
  c1.Swap(c2);
}

} // namespace sql
} // namespace fun
