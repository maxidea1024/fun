#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * A pair of 3D vectors.
 */
class TwoVectors {
 public:
  Vector v1;
  Vector v2;

  TwoVectors();
  TwoVectors(const Vector& v1, const Vector& v2);
  explicit TwoVectors(ForceInit_TAG);

  TwoVectors operator + (const TwoVectors& v) const;
  TwoVectors operator - (const TwoVectors& v) const;
  TwoVectors operator * (float scale) const;
  TwoVectors operator / (float scale) const;
  TwoVectors operator * (const TwoVectors& v) const;
  TwoVectors operator / (const TwoVectors& v) const;
  bool operator == (const TwoVectors& v) const;
  bool operator != (const TwoVectors& v) const;
  bool Equals(const TwoVectors& v, float tolerance = KINDA_SMALL_NUMBER) const;

  TwoVectors operator -() const;
  TwoVectors operator += (const TwoVectors& v);
  TwoVectors operator -= (const TwoVectors& v);
  TwoVectors operator *= (float scale);
  TwoVectors operator /= (float v);
  TwoVectors operator *= (const TwoVectors& v);
  TwoVectors operator /= (const TwoVectors& v);
  float& operator [] (int32 i);
  float operator [] (int32 i) const;

  float GetMax() const;
  float GetMin() const;
  String ToString() const;

  friend Archive& operator & (Archive& ar, TwoVectors& v) {
    return ar & v.v1 & v.v2;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }
};

template <> struct IsPOD<TwoVectors> { enum { Value = true }; };


//
// inlines
//

FUN_ALWAYS_INLINE TwoVectors operator*(float scale, const TwoVectors& v) {
  return v.operator*(scale);
}

FUN_ALWAYS_INLINE TwoVectors::TwoVectors()
  : v1(0.f), v2(0.f) {}

FUN_ALWAYS_INLINE TwoVectors::TwoVectors(const Vector& v1, const Vector& v2)
  : v1(v1), v2(v2) {}

FUN_ALWAYS_INLINE TwoVectors::TwoVectors(ForceInit_TAG)
  : v1(ForceInit), v2(ForceInit) {}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator + (const TwoVectors& v) const {
  return TwoVectors(v1 + v.v1, v2 + v.v2);
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator - (const TwoVectors& v) const {
  return TwoVectors(v1 - v.v1, v2 - v.v2);
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator * (float scale) const {
  return TwoVectors(v1 * scale, v2 * scale);
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator / (float scale) const {
  const float one_over_scale = 1.f / scale;
  return TwoVectors(v1 * one_over_scale, v2 * one_over_scale);
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator * (const TwoVectors& v) const {
  return TwoVectors(v1 * v.v1, v2 * v.v2);
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator / (const TwoVectors& v) const {
  return TwoVectors(v1 / v.v1, v2 / v.v2);
}

FUN_ALWAYS_INLINE bool TwoVectors::operator == (const TwoVectors& v) const {
  return ((v1 == v.v1) && (v2 == v.v2));
}

FUN_ALWAYS_INLINE bool TwoVectors::operator != (const TwoVectors& v) const {
  return ((v1 != v.v1) || (v2 != v.v2));
}

FUN_ALWAYS_INLINE bool TwoVectors::Equals(const TwoVectors& v, float tolerance) const {
  return v1.Equals(v.v1, tolerance) && v2.Equals(v.v2, tolerance);
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator -() const {
  return TwoVectors(-v1, -v2);
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator += (const TwoVectors& v) {
  v1 += v.v1;
  v2 += v.v2;
  return *this;
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator -= (const TwoVectors& v) {
  v1 -= v.v1;
  v2 -= v.v2;
  return *this;
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator *= (float scale) {
  v1 *= scale;
  v2 *= scale;
  return *this;
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator /= (float scale) {
  const float one_over_scale = 1.f / scale;
  v1 *= one_over_scale;
  v2 *= one_over_scale;
  return *this;
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator *= (const TwoVectors& v) {
  v1 *= v.v1;
  v2 *= v.v2;
  return *this;
}

FUN_ALWAYS_INLINE TwoVectors TwoVectors::operator /= (const TwoVectors& v) {
  v1 /= v.v1;
  v2 /= v.v2;
  return *this;
}

FUN_ALWAYS_INLINE float TwoVectors::GetMax() const {
  const float max_max = Math::Max(Math::Max(v1.x, v1.y), v1.z);
  const float max_min = Math::Max(Math::Max(v2.x, v2.y), v2.z);
  return Math::Max(max_max, max_min);
}

FUN_ALWAYS_INLINE float TwoVectors::GetMin() const {
  const float min_max = Math::Min(Math::Min(v1.x, v1.y), v1.z);
  const float min_min = Math::Min(Math::Min(v2.x, v2.y), v2.z);
  return Math::Min(min_max, min_min);
}

FUN_ALWAYS_INLINE float& TwoVectors::operator[](int32 i) {
  fun_check(i > -1);
  fun_check(i < 6);

  switch (i) {
    case 0:  return v1.x;
    case 1:  return v2.x;
    case 2:  return v1.y;
    case 3:  return v2.y;
    case 4:  return v1.z;
    default: return v2.z;
  }
}

FUN_ALWAYS_INLINE float TwoVectors::operator[](int32 i) const {
  fun_check(i > -1);
  fun_check(i < 6);

  switch (i) {
    case 0:  return v1.x;
    case 1:  return v2.x;
    case 2:  return v1.y;
    case 3:  return v2.y;
    case 4:  return v1.z;
    default: return v2.z;
  }
}

FUN_ALWAYS_INLINE String TwoVectors::ToString() const {
  return String::Format("v1=(%s) v2=(%s)", *v1.ToString(), *v2.ToString());
}

} // namespace fun
