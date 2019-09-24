#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Template for ranges.
 *
 * Note: This class is not intended for interval arithmetic (see Interval for that).
 *
 * a range represents a contiguous set of elements that only stores the set's
 * lower and upper bound values (aka. endpoints) for storage efficiency. Bound
 * values may be exclusive (the value is not part of the range), inclusive (the
 * value is part of the range) or open (there is no limit on the values).
 *
 * The template's primary focus is on continuous ranges, but it can be used for the
 * representation of discrete ranges as well. The element type of discrete ranges
 * has a well-defined stepping, such as an integer or a date, that separates the
 * neighboring elements. This is in contrast with continuous ranges in which the
 * step sizes, such as floats or time spans, are not of interest, and other elements
 * may be found between any two elements (although, in practice, all ranges are
 * discrete due to the limited precision of numerical values in computers).
 *
 * When working with ranges, the user of this template is responsible for correctly
 * interpreting the range endpoints. Certain semantics will be different depending
 * on whether the range is interpreted in a continuous or discrete domain.
 *
 * Iteration of a discrete range [a, b) includes the elements a to b-1. The elements
 * of continuous ranges are generally not meant to be iterated. It is also important
 * to consider the equivalence of different representations of discrete ranges. For
 * example, the ranges [2, 6), (1, 5] and [2, 5] are equivalent in discrete domains,
 * but different in continuous ones. In order to keep this class simple, we have not
 * included canonicalization functions or auxiliary template parameters, such as
 * unit and min/max domain elements. For ease of use in most common use cases, it is
 * recommended to limit all operations to canonical ranges of the form [a, b) in
 * which the lower bound is included and the upper bound is excluded from the range.
 *
 * \param ElementType - The type of elements represented by the range
 *
 * @see RangeBound
 * @see Interval
 */
template <typename ElementType>
class Range
{
 public:
  typedef RangeBound<ElementType> BoundsType;

  /**
   * Default constructor (no initialization).
   */
  Range() {}

  /**
   * Creates a range with a single element.
   *
   * The created range is of the form [a, a].
   *
   * \param a - The element in the range.
   */
  explicit Range(const ElementType& a)
    : lower_bound_(BoundsType::Inclusive(a))
    , upper_bound_(BoundsType::Inclusive(a))
  {}

  /**
   * Creates and initializes a new range with the given lower and upper bounds.
   *
   * The created range is of the form [a, b).
   *
   * \param a - The range's lower bound value (inclusive).
   * \param b - The range's upper bound value (exclusive).
   */
  explicit Range(const ElementType& a, const ElementType& b)
    : lower_bound_(BoundsType::Inclusive(a))
    , upper_bound_(BoundsType::Exclusive(b))
  {}

  /**
   * Creates and initializes a new range with the given lower and upper bounds.
   *
   * \param lower_bound - The range's lower bound.
   * \param upper_bound - The range's upper bound.
   */
  explicit Range(const BoundsType& lower_bound, const BoundsType& upper_bound)
    : lower_bound_(lower_bound)
    , upper_bound_(upper_bound)
  {}

 public:
  /**
   * Compares this range with the specified range for equality.
   *
   * \param other - The range to compare with.
   *
   * \return true if the ranges are equal, false otherwise.
   */
  bool operator == (const Range& other) const
  {
    if (IsEmpty() && other.IsEmpty()) {
      return true;
    }

    return (lower_bound_ == other.lower_bound_) && (upper_bound_ == other.upper_bound_);
  }

  /**
   * Compares this range with the specified range for inequality.
   *
   * \param other - The range to compare with.
   *
   * \return true if the ranges are not equal, false otherwise.
   */
  bool operator != (const Range& other) const
  {
    if (IsEmpty() && other.IsEmpty()) {
      return false;
    }

    return (lower_bound_ != other.lower_bound_) || (upper_bound_ != other.upper_bound_);
  }

 public:
  /**
   * Checks whether this range adjoins to another.
   *
   * Two ranges are adjoint if they are next to each other without overlapping, i.e.
   *      [a, b) and [c, c) or
   *      [a, b] and (b, c)
   *
   * \param other - The other range.
   *
   * \return true if this range adjoins the other, false otherwise.
   */
  bool Adjoins(const Range& other) const
  {
    if (IsEmpty() || other.IsEmpty()) {
      return false;
    }

    if (upper_bound_.GetValue() == other.lower_bound_.GetValue()) {
      return ((upper_bound_.IsInclusive() && other.lower_bound_.IsExclusive()) ||
              (upper_bound_.IsExclusive() && other.lower_bound_.IsInclusive()));
    }

    if (other.upper_bound_.GetValue() == lower_bound_.GetValue()) {
      return ((other.upper_bound_.IsInclusive() && lower_bound_.IsExclusive()) ||
              (other.upper_bound_.IsExclusive() && lower_bound_.IsInclusive()));
    }

    return false;
  }

  /**
   * Checks whether this range conjoins the two given ranges.
   *
   * a range conjoins two non-overlapping ranges if it adjoins both of them, i.e.
   *      [b, c) conjoins the two ranges [a, b) and [c, d).
   *
   * \param a - The first range.
   * \param b - The second range.
   *
   * \return true if this range conjoins the two ranges, false otherwise.
   */
  bool Conjoins(const Range& a, const Range& b) const
  {
    if (a.Overlaps(y)) {
      return false;
    }

    return Adjoins(a) && Adjoins(b);
  }

  /**
   * Checks whether this range contains the specified element.
   *
   * \param element - The element to check.
   *
   * \return true if the range contains the element, false otherwise.
   */
  bool Contains(const ElementType& element) const
  {
    return ((BoundsType::MinLower(lower_bound_, element) == lower_bound_) &&
            (BoundsType::MaxUpper(upper_bound_, element) == upper_bound_));
  }

  /**
   * Checks whether this range contains another range.
   *
   * \param other - The range to check.
   *
   * \return true if the range contains the other range, false otherwise.
   */
  bool Contains(const Range& other) const
  {
    return ((BoundsType::MinLower(lower_bound_, other.lower_bound_) == lower_bound_) &&
            (BoundsType::MaxUpper(upper_bound_, other.upper_bound_) == upper_bound_));
  }

  /**
   * Checks if this range is contiguous with another range.
   *
   * Two ranges are contiguous if they are adjoint or overlapping.
   *
   * \param other - The other range.
   *
   * \return true if the ranges are contiguous, false otherwise.
   */
  bool Contiguous(const Range& other) const
  {
    return Overlaps(other) || Adjoins(other);
  }

  /**
   * Gets the range's lower bound.
   *
   * \return Lower bound.
   */
  BoundsType GetLowerBound() const
  {
    return lower_bound_;
  }

  /**
   * Gets the value of the lower bound.
   * Use HasLowerBound() to ensure that this range actually has a lower bound.
   *
   * \return Bound value.
   *
   * @see GetUpperBoundValue, HasLowerBound
   */
  const ElementType& GetLowerBoundValue() const
  {
    return lower_bound_.GetValue();
  }

  /**
   * Gets the range's upper bound.
   *
   * \return Upper bound.
   */
  BoundsType GetUpperBound() const
  {
    return upper_bound_;
  }

  /**
   * Gets the value of the upper bound.
   *
   * Use HasUpperBound() to ensure that this range actually has an upper bound.
   *
   * \return Bound value.
   *
   * @see GetLowerBoundValue, HasUpperBound
   */
  const ElementType& GetUpperBoundValue() const
  {
    return upper_bound_.GetValue();
  }

  /**
   * Checks whether the range has a lower bound.
   *
   * \return true if the range has a lower bound, false otherwise.
   */
  bool HasLowerBound() const
  {
    return lower_bound_.IsClosed();
  }

  /**
   * Checks whether the range has an upper bound.
   *
   * \return true if the range has an upper bound, false otherwise.
   */
  bool HasUpperBound() const
  {
    return upper_bound_.IsClosed();
  }

  /**
   * Checks whether this range is degenerate.
   *
   * a range is degenerate if it contains only a single element, i.e. has the following form:
   *      [a, a]
   *
   * \return true if the range is degenerate, false otherwise.
   */
  bool IsDegenerate() const
  {
    return lower_bound_.IsInclusive() && (lower_bound_ == upper_bound_);
  }

  /**
   * Checks whether this range is empty.
   *
   * a range is empty if it contains no elements, i.e.
   *      (a, a)
   *      (a, a]
   *      [a, a)
   *
   * \return true if the range is empty, false otherwise.
   */
  bool IsEmpty() const
  {
    if (lower_bound_.IsClosed() && upper_bound_.IsClosed()) {
      if (lower_bound_.GetValue() > upper_bound_.GetValue()) {
        return true;
      }

      return (lower_bound_.GetValue() == upper_bound_.GetValue()) && (lower_bound_.IsExclusive() || upper_bound_.IsExclusive());
    }

    return false;
  }

  /**
   * Checks whether this range overlaps with another.
   *
   * \param other - The other range.
   *
   * \return true if the ranges overlap, false otherwise.
   */
  bool Overlaps(const Range& other) const
  {
    if (IsEmpty() || other.IsEmpty()) {
      return false;
    }

    bool upper_open = upper_bound_.IsOpen() || other.lower_bound_.IsOpen();
    bool lower_open = lower_bound_.IsOpen() || other.upper_bound_.IsOpen();

    // true in the case that the bounds are open (default)
    bool upper_valid = true;
    bool lower_valid = true;

    if (!upper_open) {
      bool upper_grater_than = upper_bound_.GetValue() > other.lower_bound_.GetValue();
      bool upper_grater_than_or_equal_to = upper_bound_.GetValue() >= other.lower_bound_.GetValue();
      bool upper_both_inclusive = upper_bound_.IsInclusive() && other.lower_bound_.IsInclusive();

      upper_valid = upper_both_inclusive ? upper_grater_than_or_equal_to : upper_grater_than;
    }

    if (!lower_open) {
      bool lower_less_than = lower_bound_.GetValue() < other.upper_bound_.GetValue();
      bool lower_less_than_or_equal_to = lower_bound_.GetValue() <= other.upper_bound_.GetValue();
      bool lower_both_inclusive = lower_bound_.IsInclusive() && other.upper_bound_.IsInclusive();

      lower_valid = lower_both_inclusive ? lower_less_than_or_equal_to : lower_less_than;
    }

    return upper_valid && lower_valid;
  }

  /**
   * Computes the size (diameter, length, width) of this range.
   *
   * The size of a closed range is the difference between its upper and lower bound values.
   * Use IsClosed() on the lower and upper bounds before calling this method in order to
   * make sure that the range is closed.
   *
   * \return Range size.
   */
  template <typename DifferenceType> DifferenceType Size() const
  {
    fun_check(lower_bound_.IsClosed() && upper_bound_.IsClosed());
    return (upper_bound_.GetValue() - lower_bound_.GetValue());
  }

  /**
   * Splits the range into two ranges at the specified element.
   *
   * If a range [a, c) does not contain the element b, the original range is returned.
   * Otherwise the range is split into two ranges [a, b) and [b, c), each of which may be empty.
   *
   * \param element - The element at which to split the range.
   */
  Array<Range> Split(const ElementType& element) const
  {
    Array<Range> result;

    if (Contains(element)) {
      result.Add(Range(lower_bound_, BoundsType::Exclusive(element)));
      result.Add(Range(BoundsType::Inclusive(element), upper_bound_));
    }
    else {
      result.Add(*this);
    }

    return result;
  }

 public:
  /**
   * Calculates the difference between two ranges, i.e. x - y.
   *
   * \param x - The first range to subtract from.
   * \param y - The second range to subtract with.
   *
   * \return Between 0 and 2 remaining ranges.
   *
   * @see Hull, Intersection, Union
   */
  static inline Array<Range> Difference(const Range& x, const Range& y)
  {
    Array<Range> result;

    if (x.Overlaps(y)) {
      Range LowerRange = Range(x.lower_bound_, BoundsType::FlipInclusion(y.lower_bound_));
      Range UpperRange = Range(BoundsType::FlipInclusion(y.upper_bound_), x.upper_bound_);

      if (!LowerRange.IsEmpty()) {
        result.Add(LowerRange);
      }

      if (!UpperRange.IsEmpty()) {
        result.Add(UpperRange);
      }
    }
    else {
      result.Add(x);
    }

    return result;
  }

  /**
   * Returns an empty range.
   *
   * \return Empty range.
   */
  static inline Range Empty()
  {
    return Range(BoundsType::Exclusive(ElementType()), BoundsType::Exclusive(ElementType()));
  }

  /**
   * Computes the hull of two ranges.
   *
   * The hull is the smallest range that contains both ranges.
   *
   * \param a - The first range.
   * \param b - The second range.
   *
   * \return The hull.
   *
   * @see Difference, Intersection, Union
   */
  static inline Range Hull(const Range& a, const Range& b)
  {
    if (a.IsEmpty()) {
      return b;
    }

    if (b.IsEmpty()) {
      return a;
    }

    return Range(BoundsType::MinLower(a.lower_bound_, b.lower_bound_), BoundsType::MaxUpper(a.upper_bound_, b.upper_bound_));
  }

  /**
   * Computes the hull of many ranges.
   *
   * \param ranges - The ranges to hull.
   *
   * \return The hull.
   *
   * @see Difference, Intersection, Union
   */
  static inline Range Hull(const Array<Range>& ranges)
  {
    if (ranges.Count() == 0) {
      return Range::Empty();
    }

    Range Bounds = ranges[0];

    for (int32 i = 1; i < ranges.Count(); ++i) {
      Bounds = Hull(Bounds, ranges[i]);
    }

    return Bounds;
  }

  /**
   * Computes the intersection of two ranges.
   *
   * The intersection of two ranges is the largest range that is contained by both ranges.
   *
   * \param a - The first range.
   * \param b - The second range.
   *
   * \return The intersection, or an empty range if the ranges do not overlap.
   *
   * @see Difference, Hull, Union
   */
  static inline Range Intersection(const Range& a, const Range& b)
  {
    if (a.IsEmpty()) {
      return b;
    }

    if (b.IsEmpty()) {
      return a;
    }

    return Range(BoundsType::MaxLower(a.lower_bound_, b.lower_bound_), BoundsType::MinUpper(a.upper_bound_, b.upper_bound_));
  }

  /**
   * Computes the intersection of many ranges.
   *
   * \param ranges - The ranges to intersect.
   *
   * \return The intersection.
   *
   * @see Difference, Hull, Union
   */
  static inline Range Intersection(const Array<Range>& ranges)
  {
    if (ranges.Count() == 0) {
      return Range::Empty();
    }

    Range bounds = ranges[0];
    for (int32 i = 1; i < ranges.Count(); ++i) {
      bounds = Intersection(bounds, ranges[i]);
    }
    return bounds;
  }

  /**
   * Returns the union of two contiguous ranges.
   *
   * a union is a range or series of ranges that contains both ranges.
   *
   * \param a - The first range.
   * \param b - The second range.
   *
   * \return The union, or both ranges if the two ranges are not contiguous, or no ranges if both ranges are empty.
   *
   * @see Difference, Hull, Intersection
   */
  static inline Array<Range> Union(const Range& a, const Range& b)
  {
    Array<Range> out_ranges;

    if (a.Contiguous(b)) {
      out_ranges.Add(Range(BoundsType::MinLower(a.lower_bound_, b.lower_bound_), BoundsType::MaxUpper(a.upper_bound_, b.upper_bound_)));
    }
    else {
      if (!a.IsEmpty()) {
        out_ranges.Add(a);
      }

      if (!b.IsEmpty()) {
        out_ranges.Add(b);
      }
    }

    return out_ranges;
  }

 public:
  /**
   * Creates an unbounded (open) range that contains all elements of the domain.
   *
   * \return a new range.
   */
  static inline Range All()
  {
    return Range(BoundsType::Open(), BoundsType::Open());
  }

  /**
   * Creates a left-bounded range that contains all elements greater than or equal to the specified value.
   *
   * \param value - The value.
   *
   * \return a new range.
   */
  static inline Range AtLeast(const ElementType& value)
  {
    return Range(BoundsType::Inclusive(value), BoundsType::Open());
  }

  /**
   * Creates a right-bounded range that contains all elements less than or equal to the specified value.
   *
   * \param value - The value.
   *
   * \return a new range.
   */
  static inline Range AtMost(const ElementType& value)
  {
    return Range(BoundsType::Open(), BoundsType::Inclusive(value));
  }

  /**
   * Creates a left-bounded range that contains all elements greater than the specified value.
   *
   * \param value - The value.
   *
   * \return a new range.
   */
  static inline Range GreaterThan(const ElementType& value)
  {
    return Range(BoundsType::Exclusive(value), BoundsType::Open());
  }

  /**
   * Creates a right-bounded range that contains all elements less than the specified value.
   *
   * \param value - The value.
   *
   * \return a new range.
   */
  static inline Range LessThan(const ElementType& value)
  {
    return Range(BoundsType::Open(), BoundsType::Exclusive(value));
  }

 public:
  /**
   * Serializes the given range from or into the specified archive.
   *
   * \param ar - The archive to serialize from or into.
   * \param range - The range to serialize.
   *
   * \return The archive.
   */
  friend class Archive& operator & (class Archive& ar, Range& range)
  {
    return ar & range.lower_bound_ & range.upper_bound_;
  }

  /**
  Gets the hash for the specified range.

  \param Range - The range to get the hash for.

  \return Hash value.
  */
  friend uint32 HashOf(const Range& range)
  {
    return (HashOf(range.lower_bound_) + 23 * HashOf(range.upper_bound_));
  }

 private:
  /** Holds the range's lower bound. */
  BoundsType lower_bound_;

  /** Holds the range's upper bound. */
  BoundsType upper_bound_;
};


// Default ranges for built-in types

#define FUN_DEFINE_RANGE_WRAPPER_STRUCT(Name, ElementType) \
  struct Name : Range<ElementType> { \
  private: \
    typedef Range<ElementType> Super; \
   \
  public: \
    Name() \
      : Super() \ {} \
     \
    Name(const Super& rhs) \
      : Super(rhs) \ {} \
     \
    explicit Name(const ElementType& a) \
      : Super(a) \ {} \
     \
    explicit Name(const ElementType& a, const ElementType& b) \
      : Super(a, b) \ {} \
     \
    explicit Name(const RangeBound<ElementType>& lower_bound, const RangeBound<ElementType>& upper_bound) \
      : Super(lower_bound, upper_bound) \ {} \
     \
    Array<Name> Split(const ElementType& element) const \ { \
      return Array<Name>(Super::Split(element)); \
    } \
     \
    static inline Array<Name> Difference(const Name& a, const Name& b) \ { \
      return Array<Name>(Super::Difference(a, b)); \
    } \
     \
    static inline Name Empty() \ { \
      return Super::Empty(); \
    } \
     \
    static inline Name Hull(const Name& a, const Name& b) \ { \
      return Super::Hull(a, b); \
    } \
     \
    static inline Name Hull(const Array<Name>& ranges) \ { \
      return Super::Hull(reinterpret_cast<const Array<Super>&>(ranges)); \
    } \
     \
    static inline Name Intersection(const Name& a, const Name& b) \ { \
      return Super::Intersection(a, b); \
    } \
     \
    static inline Name Intersection(const Array<Name>& ranges) \ { \
      return Super::Intersection(reinterpret_cast<const Array<Super>&>(ranges)); \
    } \
     \
    static inline Array<Name> Union(const Name& a, const Name& b) \ { \
      return Array<Name>(Super::Union(a, b)); \
    } \
     \
    static inline Name All() \ { \
      return Super::All(); \
    } \
     \
    static inline Name AtLeast(const ElementType& value) \ { \
      return Super::AtLeast(value); \
    } \
     \
    static inline Name AtMost(const ElementType& value) \ { \
      return Super::AtMost(value); \
    } \
     \
    static inline Range GreaterThan(const ElementType& value) \ { \
      return Super::GreaterThan(value); \
    } \
     \
    static inline Range LessThan(const ElementType& value) \ { \
      return Super::LessThan(value); \
    } \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<Name, Range<ElementType>> { \
    enum { Value = true }; \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<Range<ElementType>, Name> { \
    enum { Value = true }; \
  };

FUN_DEFINE_RANGE_WRAPPER_STRUCT(DateTimeRange, DateTime)
FUN_DEFINE_RANGE_WRAPPER_STRUCT(DoubleRange, double)
FUN_DEFINE_RANGE_WRAPPER_STRUCT(FloatRange, float)
FUN_DEFINE_RANGE_WRAPPER_STRUCT(Int8Range, int8)
FUN_DEFINE_RANGE_WRAPPER_STRUCT(Int16Range, int16)
FUN_DEFINE_RANGE_WRAPPER_STRUCT(Int32Range, int32)
FUN_DEFINE_RANGE_WRAPPER_STRUCT(Int64Range, int64)

} // namespace fun
