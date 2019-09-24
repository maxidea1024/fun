#pragma once

namespace fun {

/**
Template for numeric interval
*/
template <typename ElementType>
struct Interval
{
  static_assert(IsArithmetic<ElementType>::Value, "Interval can be used only with numeric types");

  /** Holds the lower bound of the interval. */
  ElementType min;

  /** Holds the upper bound of the interval. */
  ElementType max;

 public:
  /**
   * Default constructor.
   * 
   * The interval is invalid
   */
  Interval()
    : min(NumericLimits<ElementType>::Max())
    , max(NumericLimits<ElementType>::Lowest())
  {}

  /**
   * Creates and initializes a new interval with the specified lower and upper bounds.
   * 
   * \param min - The lower bound of the constructed interval.
   * \param max - The upper bound of the constructed interval.
   */
  Interval(ElementType min, ElementType max)
    : min(min)
    , max(max)
  {}

 public:
  /**
   * Offset the interval by adding x.
   * 
   * \param x - The offset.
   */
  void operator += (ElementType x)
  {
    if (IsValid()) {
      min += x;
      max += x;
    }
  }

  /**
   * Offset the interval by subtracting x.
   * 
   * \param x - The offset.
   */
  void operator -= (ElementType x)
  {
    if (IsValid()) {
      min -= x;
      max -= x;
    }
  }

 public:
  /**
   * Computes the size of this interval.
   * 
   * \return Interval size.
   */
  ElementType Size() const
  {
    return max - min;
  }

  /**
   * Whether interval is valid (min <= max).
   * 
   * \return false when interval is invalid, true otherwise
   */
  ElementType IsValid() const
  {
    return min <= max;
  }

  /**
   * Checks whether this interval contains the specified element.
   * 
   * \param element - The element to check.
   * 
   * \return true if the range interval the element, false otherwise.
   */
  bool Contains(const ElementType& element) const
  {
    return IsValid() && (element >= min && element <= max);
  }

  /**
   * Expands this interval to both sides by the specified amount.
   * 
   * \param amount - The amount to expand by.
   */
  void Expand(ElementType amount)
  {
    if (IsValid()) {
      min -= amount;
      max += amount;
    }
  }

  /**
   * Expands this interval if necessary to include the specified element.
   * 
   * \param x - The element to include.
   */
  void Include(ElementType x)
  {
    if (!IsValid()) {
      min = x;
      max = x;
    }
    else {
      if (x < min) {
        min = x;
      }

      if (x > max) {
        max = x;
      }
    }
  }

  /**
   * Interval interpolation
   * 
   * \param alpha - interpolation amount
   * 
   * \return interpolation result
   */
  ElementType Interpolate(float alpha) const
  {
    if (IsValid()) {
      return min + ElementType(alpha*Size());
    }

    return ElementType();
  }

 public:
  /**
   * Calculates the intersection of two intervals.
   * 
   * \param a - The first interval.
   * \param b - The second interval.
   * 
   * \return The intersection.
   */
  friend Interval Intersect(const Interval& a, const Interval& b)
  {
    if (a.IsValid() && b.IsValid()) {
      return Interval(Math::Max(a.min, b.min), Math::Min(a.max, b.max));
    }

    return Interval();
  }

  /**
   * Serializes the interval.
   * 
   * \param ar - The archive to serialize into.
   * \param interval - The interval to serialize.
   * 
   * \return Reference to the Archive after serialization.
   */
  friend class Archive& operator & (class Archive& ar, Interval& interval)
  {
    return ar & interval.min & interval.max;
  }

  /**
   * Gets the hash for the specified interval.
   * 
   * \param interval - The Interval to get the hash for.
   * 
   * \return Hash value.
   */
  friend uint32 HashOf(const Interval& interval)
  {
    return HashCombine(HashOf(interval.min), HashOf(interval.max));
  }
};


// Default intervals for built-in types

#define FUN_DEFINE_INTERVAL_WRAPPER_STRUCT(Name, ElementType) \
  struct Name : Interval<ElementType> { \
  private: \
    typedef Interval<ElementType> Super; \
     \
  public: \
    Name() : Super() \ {} \
    Name(const Super& other) : Super(other) \ {} \
    Name(ElementType min, ElementType max) : Super(min, max) \ {} \
     \
    friend Name Intersect(const Name& a, const Name& b) \ { \
      return Intersect(static_cast<const Super&>(a), static_cast<const Super&>(b)); \
    } \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<Name, Interval<ElementType>> { \
    enum { Value = true }; \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<Interval<ElementType>, Name> { \
    enum { Value = true }; \
  };

FUN_DEFINE_INTERVAL_WRAPPER_STRUCT(FloatInterval, float)
FUN_DEFINE_INTERVAL_WRAPPER_STRUCT(Int32Interval, int32)

} // namespace fun
