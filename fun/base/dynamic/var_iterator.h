#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

#include <iterator>
#include <algorithm>

namespace fun {
namespace dynamic {

class Var;

/**
 * VarIterator class.
 */
class FUN_BASE_API VarIterator {
 public:
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef Var value_type;
  typedef std::ptrdiff_t difference_type;
  typedef Var* pointer;
  typedef Var& reference;

  /**
   * End position indicator.
   */
  static const size_t POSITION_END;

  /**
   * Creates the VarIterator and positions it at the end of
   * the recordset if position_end is true. Otherwise, it is
   * positioned at the beginning.
   */
  VarIterator(Var* var, bool position_end);

  /**
   * Creates a copy of other VarIterator.
   */
  VarIterator(const VarIterator& other);

  /**
   * Destroys the VarIterator.
   */
  ~VarIterator();

  /**
   * Assigns the other VarIterator.
   */
  VarIterator& operator = (const VarIterator& other);

  /**
   * Equality operator.
   */
  bool operator == (const VarIterator& other) const;

  /**
   * Inequality operator.
   */
  bool operator != (const VarIterator& other) const;

  /**
   * Returns value at the current position.
   */
  Var& operator * () const;

  /**
   * Returns pointer to the value at current position.
   */
  Var* operator -> () const;

  /**
   * Advances by one position and returns current position.
   */
  const VarIterator& operator ++ () const;

  /**
   * Advances by one position and returns copy of the iterator with
   * previous current position.
   */
  VarIterator operator ++ (int) const;

  /**
   * Goes back by one position and returns copy of the iterator with
   * previous current position.
   */
  const VarIterator& operator -- () const;

  /**
   * Goes back by one position and returns previous current position.
   */
  VarIterator operator -- (int) const;

  /**
   * Returns a copy the VarIterator advanced by diff positions.
   */
  VarIterator operator + (size_t diff) const;

  /**
   * Returns a copy the VarIterator backed by diff positions.
   * Throws RangeException if diff is larger than current position.
   */
  VarIterator operator - (size_t diff) const;

  /**
   * Swaps the VarIterator with another one.
   */
  void swap(VarIterator& other);

 private:
  VarIterator();

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

  Var* var_;
  mutable size_t position_;

  friend class Var;
};


//
// inlines
//

inline bool VarIterator::operator == (const VarIterator& other) const {
  return var_ == other.var_ && position_ == other.position_;
}

inline bool VarIterator::operator != (const VarIterator& other) const {
  return var_ != other.var_ || position_ != other.position_;
}

} // namespace dynamic
} // namespace fun

namespace std {
  /**
   * Full template specialization of std:::swap for VarIterator
   */
  template <>
  inline void swap<fun::dynamic::VarIterator>(fun::dynamic::VarIterator& s1, fun::dynamic::VarIterator& s2) {
    s1.swap(s2);
  }
} // namespace std
