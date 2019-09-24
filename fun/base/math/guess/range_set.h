#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Template for range sets.
 *
 * @todo gmp: RangeSet needs an overhaul
 */
template <typename ElementType> class RangeSet
{
  typedef Range<ElementType> RangeType;

 public:
  /** Default constructor. */
  RangeSet() {}

  /** Destructor. */
  ~RangeSet() {}

 public:
  /**
  adds a range to the set.

  This method merges overlapping ranges into a single range (i.e. {[1, 4], [4, 6]} becomes [1, 6]).
  Adjacent ranges (i.e. {[1, 3], [4, 6]}) are not merged.

  \param range The range to add.
  *//*
  void Add(RangeType range)
  {
    for (int32 Index = 0; Index < ranges_.Count(); ++Index) {
      RangeType& current = ranges_(Index);

      if ((current.GetUpperBound() < range.GetLowerBound()) || current.GetLowerBound() > range.GetUpperBound()) {
        continue;
      }

      range = RangeType(Math::Min(current.GetLowerBound(), range.GetLowerBound()), Math::Max(current.GetUpperBound(), range.GetUpperBound()));

      ranges_.RemoveAtSwap(Index--);
    }

    ranges_.Add(range);
  }*/

  /**
  Merges another range set into this set.

  \param other The range set to merge.
  */
/*  void Merge(const RangeSet& other)
  {
    for (typename Array<RangeType>::ConstIterator it(other.ranges_); it; ++it) {
      Add(*it);
    }
  }*/

  /**
  Removes a range from the set.

  ranges_ that overlap with the removed range will be split.

  \param range The range to remove.
  */
/*  void Remove(const RangeType& range)
  {
    for (int32 Index = 0; Index < ranges_.Count(); ++Index) {
      RangeType& current = ranges_(Index);

      if ((current.GetUpperBound() < range.GetLowerBound()) || (current.GetLowerBound() > range.GetUpperBound())) {
        continue;
      }

      if (current.GetLowerBound() < range.GetLowerBound()) {
        ranges_.Add(RangeType(current.GetLowerBound(), range.GetLowerBound()));
      }

      if (current.GetUpperBound() > range.GetUpperBound()) {
        ranges_.Add(RangeType(range.GetUpperBound(), current.GetUpperBound()));
      }

      ranges_.RemoveAtSwap(Index--);
    }
  }*/

  /**
   * Removes all ranges from the set.
   */
  void Clear()
  {
    ranges_.Clear();
  }

 public:
  /**
   * Checks whether this set contains the specified element.
   *
   * \param element - The element to check.
   *
   * \return true if the element is in the set, false otherwise.
   */
  bool Contains(const ElementType& element) const
  {
    for (typename Array<RangeType>::ConstIterator it(ranges_); it; ++it) {
      if (it->Contains(element)) {
        return true;
      }
    }

    return false;
  }

  /**
   * Checks whether this set contains the specified range.
   *
   * \param range - The range to check.
   *
   * \return true if the set contains the range, false otherwise.
   */
  bool Contains(const RangeType& range) const
  {
    for (typename Array<RangeType>::ConstIterator it(ranges_); it; ++it) {
      if (it->Contains(range)) {
        return true;
      }
    }

    return false;
  }

  /**
   * Returns a read-only collection of the ranges contained in this set.
   *
   * \return Array of ranges.
   */
  const Array<RangeType>& GetRanges() const
  {
    return ranges_;
  }

  /**
   * Checks whether this range set is empty.
   *
   * \return true if the range set is empty, false otherwise.
   */
  bool IsEmpty() const
  {
    return ranges_.Count() == 0;
  }

  /**
   * Checks whether this range set overlaps with the specified range.
   *
   * \param range - The range to check.
   *
   * \return true if this set overlaps with the range, false otherwise.
   */
  bool Overlaps(const RangeType& range) const
  {
    for (typename Array<RangeType>::ConstIterator it(ranges_); it; ++it) {
      if (it->Overlaps(range)) {
        return true;
      }
    }

    return false;
  }

  /**
   * Checks whether this range set overlaps with another.
   *
   * \param other - The other range set.
   *
   * \return true if the range sets overlap, false otherwise.
   *
   * @todo gmp: This could be optimized to O(n*logn) using a line sweep on a pre-sorted array of bounds.
   */
  bool Overlaps(const RangeSet& other) const
  {
    for (typename Array<RangeType>::ConstIterator it(other.ranges_); it; ++it) {
      if (Overlaps(*it)) {
        return true;
      }
    }

    return false;
  }

 public:
  /**
   * Serializes the given range set from or into the specified archive.
   *
   * \param ar - The archive to serialize from or into.
   * \param v - The range set to serialize.
   *
   * \return The archive.
   */
  friend class Archive& operator & (class Archive& ar, RangeSet& v)
  {
    return ar & v.ranges_;
  }

 private:
  /** Holds the set of ranges. */
  Array<RangeType> ranges_;
};

} // namespace fun
