#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Enumerates the valid types of range bounds.
 */
enum class RangeBoundTypes {
  /** The range excludes the bound. */
  Exclusive,

  /** The range includes the bound. */
  Inclusive,

  /** The bound is open. */
  Open
};


/**
 * Template for range bounds.
 */
template <typename ElementType>
class RangeBound
{
 public:
  /**
   * Default constructor.
   *
   * @see Exclusive, Inclusive, Open
   */
  RangeBound()
    : type_(RangeBoundTypes::Open)
    , value_()
  {}

  /**
   * Creates a closed bound that includes the specified value.
   *
   * \param value - The bound's value.
   *
   * @see Exclusive, Inclusive, Open
   */
  RangeBound(const ElementType& value)
    : type_(RangeBoundTypes::Inclusive)
    , value_(value)
  {}

 public:
  /**
   * Compares this bound with the specified bound for equality.
   *
   * \param other - The bound to compare with.
   *
   * \return true if the bounds are equal, false otherwise.
   */
  bool operator == (const RangeBound& other) const
  {
    return (type_ == other.type_) && (IsOpen() || (value_ == other.value_));
  }

  /**
   * Compares this range with the specified bound for inequality.
   *
   * \param other - The bound to compare with.
   *
   * \return true if the bounds are not equal, false otherwise.
   */
  bool operator != (const RangeBound& other) const
  {
    return (type_ != other.type_) || (!IsOpen() && (value_ != other.value_));
  }

 public:
  /**
   * Gets the bound's value.
   *
   * Use IsClosed() to verify that this bound is closed before calling this method.
   *
   * \return Bound value.
   *
   * @see IsOpen
   */
  const ElementType& GetValue() const
  {
    fun_check(type_ != RangeBoundTypes::Open);
    return value_;
  }

  /**
   * Checks whether the bound is closed.
   *
   * \return true if the bound is closed, false otherwise.
   */
  bool IsClosed() const
  {
    return type_ != RangeBoundTypes::Open;
  }

  /**
   * Checks whether the bound is exclusive.
   *
   * \return true if the bound is exclusive, false otherwise.
   */
  bool IsExclusive() const
  {
    return type_ == RangeBoundTypes::Exclusive;
  }

  /**
   * Checks whether the bound is inclusive.
   *
   * \return true if the bound is inclusive, false otherwise.
   */
  bool IsInclusive() const
  {
    return type_ == RangeBoundTypes::Inclusive;
  }

  /**
   * Checks whether the bound is open.
   *
   * \return true if the bound is open, false otherwise.
   */
  bool IsOpen() const
  {
    return type_ == RangeBoundTypes::Open;
  }

 public:
  /**
   * Serializes the given bound from or into the specified archive.
   *
   * \param ar - The archive to serialize from or into.
   * \param Bound - The bound to serialize.
   *
   * \return The archive.
   */
  friend class Archive& operator & (class Archive& ar, RangeBound& bound)
  {
    return ar & (uint8&)bound.type_ & bound.value_;
  }

  /**
   * Gets the hash for the specified bound.
   *
   * \param bound - The bound to get the hash for.
   *
   * \return Hash value.
   */
  friend uint32 HashOf(const RangeBound& bound)
  {
    return (HashOf((uint8)bound.type_) + 23 * HashOf(bound.value_));
  }

 public:
  /**
   * Returns a closed bound that excludes the specified value.
   *
   * \param value - The bound value.
   *
   * \return An exclusive closed bound.
   */
  static FUN_ALWAYS_INLINE RangeBound Exclusive(const ElementType& value)
  {
    RangeBound result;
    result.type_ = RangeBoundTypes::Exclusive;
    result.value = value;
    return result;
  }

  /**
   * Returns a closed bound that includes the specified value.
   *
   * \param value - The bound value.
   *
   * \return An inclusive closed bound.
   */
  static FUN_ALWAYS_INLINE RangeBound Inclusive(const ElementType& value)
  {
    RangeBound result;
    result.type_ = RangeBoundTypes::Inclusive;
    result.value_ = value;
    return result;
  }

  /**
   * Returns an open bound.
   *
   * \return An open bound.
   */
  static FUN_ALWAYS_INLINE RangeBound Open()
  {
    RangeBound result;
    result.type_ = RangeBoundTypes::Open;
    return result;
  }

 public:
  /**
   * Returns the given bound with its inclusion flipped between inclusive and exclusive.
   *
   * If the bound is open it is returned unchanged.
   *
   * \return a new bound.
   */
  static FUN_ALWAYS_INLINE RangeBound FlipInclusion(const RangeBound& bound)
  {
    if (bound.IsExclusive()) {
      return Inclusive(bound.value_);
    }

    if (bound.IsInclusive()) {
      return Exclusive(bound.value_);
    }

    return bound;
  }

  /**
   * Returns the greater of two lower bounds.
   *
   * \param a - The first lower bound.
   * \param b - The second lower bound.
   *
   * \return The greater lower bound.
   */
  static FUN_ALWAYS_INLINE const RangeBound& MaxLower(const RangeBound& a, const RangeBound& b)
  {
    if (a.IsOpen()) { return b; }
    if (b.IsOpen()) { return a; }
    if (a.value_ > b.value_) { return a; }
    if (b.value_ > a.value_) { return b; }
    if (a.IsExclusive()) { return a; }

    return b;
  }

  /**
   * Returns the greater of two upper bounds.
   *
   * \param a - The first upper bound.
   * \param b - The second upper bound.
   *
   * \return The greater upper bound.
   */
  static FUN_ALWAYS_INLINE const RangeBound& MaxUpper(const RangeBound& a, const RangeBound& b)
  {
    if (a.IsOpen()) { return a; }
    if (b.IsOpen()) { return b; }
    if (a.value_ > b.value_) { return a; }
    if (b.value_ > a.value_) { return b; }
    if (a.IsInclusive()) { return a; }

    return b;
  }

  /**
   * Returns the lesser of two lower bounds.
   *
   * \param a - The first lower bound.
   * \param b - The second lower bound.
   *
   * \return The lesser lower bound.
   */
  static FUN_ALWAYS_INLINE const RangeBound& MinLower(const RangeBound& a, const RangeBound& b)
  {
    if (a.IsOpen()) { return a; }
    if (b.IsOpen()) { return b; }
    if (a.value_ < b.value_) { return a; }
    if (b.value_ < a.value_) { return b; }
    if (a.IsInclusive()) { return a; }

    return b;
  }

  /**
   * Returns the lesser of two upper bounds.
   *
   * \param a - The first upper bound.
   * \param b - The second upper bound.
   *
   * \return The lesser upper bound.
   */
  static FUN_ALWAYS_INLINE const RangeBound& MinUpper(const RangeBound& a, const RangeBound& b)
  {
    if (a.IsOpen()) { return b; }
    if (b.IsOpen()) { return a; }
    if (a.value_ < b.value_) { return a; }
    if (b.value_ < a.value_) { return b; }
    if (a.IsExclusive()) { return a; }

    return b;
  }

 private:
  /** Holds the type of the bound. */
  EnumAsByte<RangeBoundTypes> type_;

  /** Holds the bound's value. */
  ElementType value_;
};


// Default range bounds for built-in types

#define FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(Name, ElementType) \
  struct Name : RangeBound<ElementType> { \
   private: \
    typedef RangeBound<ElementType> Super; \
   \
  public: \
    Name() \
      : Super() \
    { \
    } \
     \
    Name(const Super& other) \
      : Super(other) \
    { \
    } \
     \
    Name(const int64& InValue) \
      : Super(InValue) \
    { \
    } \
     \
    static FUN_ALWAYS_INLINE Name Exclusive(const ElementType& value) \
    { \
      return static_cast<const Name&>(Super::Exclusive(value)); \
    } \
     \
    static FUN_ALWAYS_INLINE Name Inclusive(const ElementType& value) \
    { \
      return static_cast<const Name&>(Super::Inclusive(value)); \
    } \
     \
    static FUN_ALWAYS_INLINE Name Open() \
    { \
      return static_cast<const Name&>(Super::Open()); \
    } \
     \
    static FUN_ALWAYS_INLINE Name FlipInclusion(const Name& bound) \
    { \
      return static_cast<const Name&>(Super::FlipInclusion(bound)); \
    } \
     \
    static FUN_ALWAYS_INLINE const Name& MaxLower(const Name& a, const Name& b) \
    { \
      return static_cast<const Name&>(Super::MaxLower(a, b)); \
    } \
     \
    static FUN_ALWAYS_INLINE const Name& MaxUpper(const Name& a, const Name& b) \
    { \
      return static_cast<const Name&>(Super::MaxUpper(a, b)); \
    } \
     \
    static FUN_ALWAYS_INLINE const Name& MinLower(const Name& a, const Name& b) \
    { \
      return static_cast<const Name&>(Super::MinLower(a, b)); \
    } \
     \
    static FUN_ALWAYS_INLINE const Name& MinUpper(const Name& a, const Name& b) \
    { \
      return static_cast<const Name&>(Super::MinUpper(a, b)); \
    } \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<Name, RangeBound<ElementType>> { \
    enum { Value = true }; \
  }; \
   \
  template <> \
  struct IsBitwiseConstructible<RangeBound<ElementType>, Name> { \
    enum { Value = true }; \
  };

//TODO?
//FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(DateRangeBound, DateTime)
FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(DoubleRangeBound, double)
FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(FloatRangeBound, float)
FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(Int8RangeBound, int8)
FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(Int16RangeBound, int16)
FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(Int32RangeBound, int32)
FUN_DEFINE_RANGEBOUND_WRAPPER_STRUCT(Int64RangeBound, int64)

} // namespace fun
