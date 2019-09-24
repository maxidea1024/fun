#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Structure for integer points in 2-d space.
 *
 * @todo Docs: The operators need better documentation, i.e. what does it mean to divide a point?
 */
class IntPoint {
 public:
  int32 x;
  int32 y;

  FUN_BASE_API static const IntPoint ZeroValue;
  FUN_BASE_API static const IntPoint NoneValue;

  IntPoint();
  IntPoint(int32 x, int32 y);
  explicit IntPoint(ForceInit_TAG);

  bool operator == (const IntPoint& other) const;
  bool operator != (const IntPoint& other) const;

  IntPoint& operator *= (int32 scale);
  IntPoint& operator /= (int32 divisor);
  IntPoint& operator += (const IntPoint& other);
  IntPoint& operator -= (const IntPoint& other);
  IntPoint& operator /= (const IntPoint& other);
  IntPoint& operator = (const IntPoint& other);
  IntPoint operator * (int32 scale) const;
  IntPoint operator / (int32 divisor) const;
  IntPoint operator + (const IntPoint& other) const;
  IntPoint operator - (const IntPoint& other) const;
  IntPoint operator / (const IntPoint& other) const;
  int32& operator[](int32 index);
  int32 operator[](int32 index) const;

  IntPoint ComponentMin(const IntPoint& other) const;
  IntPoint ComponentMax(const IntPoint& other) const;

  int32 GetMax() const;
  int32 GetMin() const;

  int32 Size() const;
  int32 SizeSquared() const;

  String ToString() const;

  static IntPoint DivideAndRoundUp(const IntPoint& lhs, int32 divisor);
  static IntPoint DivideAndRoundUp(const IntPoint& lhs, const IntPoint& divisor);
  static IntPoint DivideAndRoundDown(const IntPoint& lhs, int32 divisor);

  static int32 Count();

  friend Archive& operator & (Archive& ar, IntPoint& point) {
    return ar & point.x & point.y;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE IntPoint::IntPoint() {}

FUN_ALWAYS_INLINE IntPoint::IntPoint(int32 x, int32 y)
  : x(x), y(y) {}

FUN_ALWAYS_INLINE IntPoint::IntPoint(ForceInit_TAG)
  : x(0), y(0) {}

FUN_ALWAYS_INLINE int32 IntPoint::Count() {
  return 2;
}

FUN_ALWAYS_INLINE bool IntPoint::operator == (const IntPoint& other) const {
  return x == other.x && y == other.y;
}

FUN_ALWAYS_INLINE bool IntPoint::operator != (const IntPoint& other) const {
  return (x != other.x) || (y != other.y);
}

FUN_ALWAYS_INLINE IntPoint& IntPoint::operator *= (int32 scale) {
  x *= scale;
  y *= scale;
  return *this;
}

FUN_ALWAYS_INLINE IntPoint& IntPoint::operator /= (int32 scale) {
  x /= scale;
  y /= scale;
  return *this;
}

FUN_ALWAYS_INLINE IntPoint& IntPoint::operator += (const IntPoint& other) {
  x += other.x;
  y += other.y;
  return *this;
}

FUN_ALWAYS_INLINE IntPoint& IntPoint::operator -= (const IntPoint& other) {
  x -= other.x;
  y -= other.y;
  return *this;
}

FUN_ALWAYS_INLINE IntPoint& IntPoint::operator /= (const IntPoint& other) {
  x /= other.x;
  y /= other.y;
  return *this;
}

FUN_ALWAYS_INLINE IntPoint& IntPoint::operator = (const IntPoint& other) {
  x = other.x;
  y = other.y;
  return *this;
}

FUN_ALWAYS_INLINE IntPoint IntPoint::operator * (int32 scale) const {
  return IntPoint(*this) *= scale;
}

FUN_ALWAYS_INLINE IntPoint IntPoint::operator / (int32 scale) const {
  return IntPoint(*this) /= scale;
}

FUN_ALWAYS_INLINE int32& IntPoint::operator[](int32 index) {
  fun_check(index >= 0 && index < 2);
  return index == 0 ? x : y;
}

FUN_ALWAYS_INLINE int32 IntPoint::operator[](int32 index) const {
  fun_check(index >= 0 && index < 2);
  return index == 0 ? x : y;
}

FUN_ALWAYS_INLINE IntPoint IntPoint::ComponentMin(const IntPoint& other) const {
  return IntPoint(Math::Min(x, other.x), Math::Min(y, other.y));
}

FUN_ALWAYS_INLINE IntPoint IntPoint::ComponentMax(const IntPoint& other) const {
  return IntPoint(Math::Max(x, other.x), Math::Max(y, other.y));
}

FUN_ALWAYS_INLINE IntPoint IntPoint::DivideAndRoundUp(const IntPoint& lhs, int32 divisor) {
  return IntPoint(Math::DivideAndRoundUp(lhs.x, divisor), Math::DivideAndRoundUp(lhs.y, divisor));
}

FUN_ALWAYS_INLINE IntPoint IntPoint::DivideAndRoundUp(const IntPoint& lhs, const IntPoint& divisor) {
  return IntPoint(Math::DivideAndRoundUp(lhs.x, divisor.x), Math::DivideAndRoundUp(lhs.y, divisor.y));
}

FUN_ALWAYS_INLINE IntPoint IntPoint::DivideAndRoundDown(const IntPoint& lhs, int32 divisor) {
  return IntPoint(Math::DivideAndRoundDown(lhs.x, divisor), Math::DivideAndRoundDown(lhs.y, divisor));
}

FUN_ALWAYS_INLINE IntPoint IntPoint::operator+(const IntPoint& other) const {
  return IntPoint(*this) += other;
}

FUN_ALWAYS_INLINE IntPoint IntPoint::operator-(const IntPoint& other) const {
  return IntPoint(*this) -= other;
}

FUN_ALWAYS_INLINE IntPoint IntPoint::operator/(const IntPoint& other) const {
  return IntPoint(*this) /= other;
}

FUN_ALWAYS_INLINE int32 IntPoint::GetMax() const {
  return Math::Max(x, y);
}

FUN_ALWAYS_INLINE int32 IntPoint::GetMin() const {
  return Math::Min(x, y);
}

FUN_ALWAYS_INLINE uint32 HashOf(const IntPoint& point) {
  return HashCombine(HashOf(point.x), HashOf(point.y));
}

FUN_ALWAYS_INLINE int32 IntPoint::Size() const {
  return int32(Math::Sqrt(float(x*x + y*y)));
}

FUN_ALWAYS_INLINE int32 IntPoint::SizeSquared() const {
  return x*x + y*y;
}

FUN_ALWAYS_INLINE String IntPoint::ToString() const {
  return String::Format("x=%d y=%d", x, y);
}

template <> struct IsPOD<IntPoint> { enum { Value = true }; };

} // namespace fun
