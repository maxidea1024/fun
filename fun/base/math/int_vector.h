#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Structure for integer vectors in 3-d space.
 */
class IntVector {
 public:
  int32 x;
  int32 y;
  int32 z;

  FUN_BASE_API static const IntVector ZeroValue;
  FUN_BASE_API static const IntVector NoneValue;

  IntVector();
  IntVector(int32 x, int32 y, int32 z);
  explicit IntVector(int32 value);
  explicit IntVector(ForceInit_TAG);

  const int32& operator[](int32 index) const;
  int32& operator[](int32 index);

  bool operator == (const IntVector& other) const;
  bool operator != (const IntVector& other) const;
  IntVector& operator *= (int32 scale);
  IntVector& operator /= (int32 divisor);
  IntVector& operator += (const IntVector& other);
  IntVector& operator -= (const IntVector& other);
  IntVector& operator = (const IntVector& other);
  IntVector operator * (int32 scale) const;
  IntVector operator / (int32 divisor) const;
  IntVector operator + (const IntVector& other) const;
  IntVector operator - (const IntVector& other) const;

  float GetMax() const;
  float GetMin() const;

  int32 Size() const;

  String ToString() const;

  static IntVector DivideAndRoundUp(const IntVector& lhs, int32 divisor);

  static int32 Count();

  friend Archive& operator & (Archive& ar, IntVector& v) {
    return ar & v.x & v.y & v.z;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE IntVector::IntVector() {}

FUN_ALWAYS_INLINE IntVector::IntVector(int32 x, int32 y, int32 z)
  : x(x), y(y), z(z) {}

FUN_ALWAYS_INLINE IntVector::IntVector(int32 value)
  : x(value), y(value), z(value) {}

FUN_ALWAYS_INLINE IntVector::IntVector(ForceInit_TAG)
  : x(0), y(0), z(0) {}

FUN_ALWAYS_INLINE const int32& IntVector::operator[](int32 index) const {
  return (&x)[index];
}

FUN_ALWAYS_INLINE int32& IntVector::operator[](int32 index) {
  return (&x)[index];
}

FUN_ALWAYS_INLINE bool IntVector::operator == (const IntVector& other) const {
  return x == other.x && y == other.y && z == other.z;
}

FUN_ALWAYS_INLINE bool IntVector::operator != (const IntVector& other) const {
  return x != other.x || y != other.y || z != other.z;
}

FUN_ALWAYS_INLINE IntVector& IntVector::operator *= (int32 scale) {
  x *= scale;
  y *= scale;
  z *= scale;
  return *this;
}

FUN_ALWAYS_INLINE IntVector& IntVector::operator /= (int32 scale) {
  x /= scale;
  y /= scale;
  z /= scale;
  return *this;
}

FUN_ALWAYS_INLINE IntVector& IntVector::operator += (const IntVector& other) {
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

FUN_ALWAYS_INLINE IntVector& IntVector::operator -= (const IntVector& other) {
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

FUN_ALWAYS_INLINE IntVector& IntVector::operator = (const IntVector& other) {
  x = other.x;
  y = other.y;
  z = other.z;

  return *this;
}

FUN_ALWAYS_INLINE IntVector IntVector::operator * (int32 scale) const {
  return IntVector(*this) *= scale;
}

FUN_ALWAYS_INLINE IntVector IntVector::operator / (int32 scale) const {
  return IntVector(*this) /= scale;
}

FUN_ALWAYS_INLINE IntVector IntVector::operator + (const IntVector& other) const {
  return IntVector(*this) += other;
}

FUN_ALWAYS_INLINE IntVector IntVector::operator - (const IntVector& other) const {
  return IntVector(*this) -= other;
}

FUN_ALWAYS_INLINE IntVector IntVector::DivideAndRoundUp(const IntVector& lhs, int32 divisor) {
  return IntVector( Math::DivideAndRoundUp(lhs.x, divisor),
                    Math::DivideAndRoundUp(lhs.y, divisor),
                    Math::DivideAndRoundUp(lhs.z, divisor));
}

FUN_ALWAYS_INLINE float IntVector::GetMax() const {
  return Math::Max(Math::Max(x, y), z);
}

FUN_ALWAYS_INLINE float IntVector::GetMin() const {
  return Math::Min(Math::Min(x, y), z);
}

FUN_ALWAYS_INLINE int32 IntVector::Count() {
  return 2;
}

FUN_ALWAYS_INLINE int32 IntVector::Size() const {
  return int32(Math::Sqrt(float(x * x + y * y + z * z)));
}

FUN_ALWAYS_INLINE String IntVector::ToString() const {
  return String::Format("x=%d y=%d z=%d", x, y, z);
}

FUN_ALWAYS_INLINE uint32 HashOf(const IntVector& vector) {
  return Crc::Crc32(&vector, sizeof(IntVector));
}

template <> struct IsPOD<IntVector> { enum { Value = true }; };

} // namespace fun
