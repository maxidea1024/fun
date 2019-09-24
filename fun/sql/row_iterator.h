#pragma once

#include <algorithm>
#include <iterator>
#include "fun/base/dynamic/var.h"
#include "fun/sql/row.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

class RecordSet;

/**
 * RowIterator class.
 */
class FUN_SQL_API RowIterator {
 public:
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef Row value_type;
  typedef std::ptrdiff_t difference_type;
  typedef Row* pointer;
  typedef Row& reference;

  /**
   * End position indicator.
   */
  static const size_t POSITION_END;

  //@ deprecated
  /**
   * Creates the RowIterator and positions it at the end of
   * the recordset if position_end is true. Otherwise, it is
   * positioned at the beginning.
   */
  RowIterator(RecordSet* record_set, bool position_end);

  /**
   * Creates the RowIterator and positions it at the end of
   * the recordset if position_end is true. Otherwise, it is
   * positioned at the beginning.
   */
  RowIterator(RecordSet& recordSet, bool position_end = false);

  /**
   * Creates a copy of other RowIterator.
   */
  RowIterator(const RowIterator& other);

  /**
   * Destroys the RowIterator.
   */
  ~RowIterator();

  /**
   * Assigns the other RowIterator.
   */
  RowIterator& operator=(const RowIterator& other);

  /**
   * Equality operator.
   */
  bool operator==(const RowIterator& other) const;

  /**
   * Inequality operator.
   */
  bool operator!=(const RowIterator& other) const;

  /**
   * Returns reference to the current row.
   */
  Row& operator*() const;

  /**
   * Returns pointer to the current row.
   */
  Row* operator->() const;

  /**
   * Advances by one position and returns current position.
   */
  const RowIterator& operator++() const;

  /**
   * Advances by one position and returns copy of the iterator with
   * previous current position.
   */
  RowIterator operator++(int) const;

  /**
   * Goes back by one position and returns copy of the iterator with
   * previous current position.
   */
  const RowIterator& operator--() const;

  /**
   * Goes back by one position and returns previous current position.
   */
  RowIterator operator--(int) const;

  /**
   * Returns a copy the RowIterator advanced by diff positions.
   */
  RowIterator operator+(size_t diff) const;

  /**
   * Returns a copy the RowIterator backed by diff positions.
   * Throws RangeException if diff is larger than current position.
   */
  RowIterator operator-(size_t diff) const;

  /**
   * Swaps the RowIterator with another one.
   */
  void Swap(RowIterator& other);

 private:
  RowIterator();

  /**
   * Increments the iterator position by one.
   * Throws RangeException if position is out of range.
   */
  void Increment() const;

  /**
   * Decrements the iterator position by one.
   * Throws RangeException if position is out of range.
   */
  void Decrement() const;

  /**
   * Sets the iterator position.
   * Throws RangeException if position is out of range.
   */
  void SetPosition(size_t pos) const;

  RecordSet* record_set_;
  mutable size_t position_;
};

//
// inlines
//

inline bool RowIterator::operator==(const RowIterator& other) const {
  return record_set_ == other.record_set_ && position_ == other.position_;
}

inline bool RowIterator::operator!=(const RowIterator& other) const {
  return record_set_ != other.record_set_ || position_ != other.position_;
}

}  // namespace sql
}  // namespace fun

namespace std {

/**
 * Full template specialization of std:::Swap for RowIterator
 */
template <>
inline void Swap<fun::sql::RowIterator>(fun::sql::RowIterator& s1,
                                        fun::sql::RowIterator& s2) {
  s1.Swap(s2);
}

}  // namespace std
