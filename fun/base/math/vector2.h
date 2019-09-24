#pragma once

#include "fun/base/base.h"
#include "fun/base/serialization/archive.h"
#include "fun/base/math/math_util.h"

namespace fun {

/**
 * a vector in 2-D space composed of components (x, y) with
 * floating point precision.
 */
class Vector2 {
 public:
  float x;
  float y;

  static FUN_BASE_API const Vector2 ZeroVector;
  static FUN_BASE_API const Vector2 UnitVector;

  Vector2() {}
  Vector2(float scalar);
  Vector2(float x, float y);
  Vector2(const IntPoint& pt);
  explicit Vector2(ForceInit_TAG);
  explicit Vector2(const Vector& v);

  Vector2 operator + (const Vector2& v) const;
  Vector2 operator - (const Vector2& v) const;
  Vector2 operator * (float scale) const;
  Vector2 operator / (float scale) const;
  Vector2 operator + (float a) const;
  Vector2 operator - (float a) const;
  Vector2 operator * (const Vector2& v) const;
  Vector2 operator / (const Vector2& v) const;

  float operator | (const Vector2& v) const;
  float operator ^ (const Vector2& v) const;

  bool operator == (const Vector2& v) const;
  bool operator != (const Vector2& v) const;
  bool operator < (const Vector2& v) const;
  bool operator > (const Vector2& v) const;
  bool operator <= (const Vector2& v) const;
  bool operator >= (const Vector2& v) const;

  Vector2 operator -() const;
  Vector2 operator += (const Vector2& v);
  Vector2 operator -= (const Vector2& v);
  Vector2 operator *= (float scale);
  Vector2 operator /= (float scale);
  Vector2 operator *= (const Vector2& v);
  Vector2 operator /= (const Vector2& v);

  float& operator[](int32 index);
  float operator[](int32 index) const;
  float& Component(int32 index);
  float Component(int32 index) const;

  static float DotProduct(const Vector2& a, const Vector2& b);
  static float DistanceSqr(const Vector2& v1, const Vector2& v2);
  static float Distance(const Vector2& v1, const Vector2& v2);
  static float CrossProduct(const Vector2& a, const Vector2& b);

  bool Equals(const Vector2& v, float tolerance) const;

  void Set(float x, float y);

  float GetMax() const;
  float GetAbsMax() const;
  float GetMin() const;

  float Size() const;
  float SizeSquared() const;

  Vector2 GetRotated(float degrees) const;
  Vector2 GetSafeNormal(float tolerance = SMALL_NUMBER) const;
  void Normalize(float tolerance = SMALL_NUMBER);

  bool IsNearlyZero(float tolerance = KINDA_SMALL_NUMBER) const;

  void ToDirectionAndLength(Vector2& out_dir, float& out_length) const;

  bool IsZero() const;

  IntPoint ToIntPoint() const;

  Vector2 ClampAxes(float min_axis_val, float max_axis_val) const;

  FUN_ALWAYS_INLINE Vector2 GetSignVector() const;

  String ToString() const;
  bool InitFromString(const String& string);

  friend Archive& operator & (Archive& ar, Vector2& v) {
    // @warning BulkSerialize: Vector2 is serialized as memory dump
    // See Array::BulkSerialize for detailed description of implied limitations.
    return ar & v.x & v.y;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }

#if FUN_ENABLE_NAN_DIAGNOSTIC
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() {
    if (ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Vector contains NaN: %s", *ToString());
      *this = Vector2::ZeroVector;
    }
  }
#else
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() {}
#endif

  FUN_ALWAYS_INLINE bool ContainsNaN() const {
    return (Math::IsNaN(x) || !Math::IsFinite(x) ||
            Math::IsNaN(y) || !Math::IsFinite(y));
  }

  FUN_BASE_API bool NetSerialize(Archive& ar, class RPackageMap* map, bool& out_success);

  /** Converts spherical coordinates on the unit sphere into a Cartesian unit length vector. */
  Vector SphericalToUnitCartesian() const;
};


template <> struct IsPOD<Vector2> { enum { Value = true }; };


//
// inlines
//

FUN_ALWAYS_INLINE Vector2 operator*(float scale, const Vector2& v) {
  return v.operator*(scale);
}

FUN_ALWAYS_INLINE Vector2(float scalar)
  : x(scalar), y(scalar) {}

FUN_ALWAYS_INLINE Vector2::Vector2(float x, float y)
  : x(x), y(y) {}

FUN_ALWAYS_INLINE Vector2::Vector2(const IntPoint& pt)
  : x((float)pt.x), y((float)pt.y) {}

FUN_ALWAYS_INLINE Vector2::Vector2(ForceInit_TAG)
  : x(0), y(0) {}

FUN_ALWAYS_INLINE Vector2 Vector2::operator + (const Vector2& v) const {
  return Vector2(x + v.x, y + v.y);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator - (const Vector2& v) const {
  return Vector2(x - v.x, y - v.y);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator * (float scale) const {
  return Vector2(x * scale, y * scale);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator / (float scale) const {
  const float one_over_scale = 1.f / scale;
  return Vector2(x * one_over_scale, y * one_over_scale);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator + (float a) const {
  return Vector2(x + a, y + a);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator - (float a) const {
  return Vector2(x - a, y - a);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator * (const Vector2& v) const {
  return Vector2(x * v.x, y * v.y);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator / (const Vector2& v) const {
  return Vector2(x / v.x, y / v.y);
}

FUN_ALWAYS_INLINE float Vector2::operator | (const Vector2& v) const {
  return x*v.x + y*v.y;
}

FUN_ALWAYS_INLINE float Vector2::operator ^ (const Vector2& v) const {
  return x*v.y - y*v.x;
}

FUN_ALWAYS_INLINE float Vector2::DotProduct(const Vector2& a, const Vector2& b) {
  return a | b;
}

FUN_ALWAYS_INLINE float Vector2::DistanceSqr(const Vector2& v1, const Vector2& v2) {
  return Math::Square(v2.x - v1.x) + Math::Square(v2.y - v1.y);
}

FUN_ALWAYS_INLINE float Vector2::Distance(const Vector2& v1, const Vector2& v2) {
  return Math::Sqrt(Vector2::DistanceSqr(v1, v2));
}

FUN_ALWAYS_INLINE float Vector2::CrossProduct(const Vector2& a, const Vector2& b) {
  return a ^ b;
}

FUN_ALWAYS_INLINE bool Vector2::operator == (const Vector2& v) const {
  return x == v.x && y == v.y;
}

FUN_ALWAYS_INLINE bool Vector2::operator != (const Vector2& v) const {
  return x != v.x || y != v.y;
}

FUN_ALWAYS_INLINE bool Vector2::operator < (const Vector2& v) const {
  return x < v.x && y < v.y;
}

FUN_ALWAYS_INLINE bool Vector2::operator > (const Vector2& v) const {
  return x > v.x && y > v.y;
}

FUN_ALWAYS_INLINE bool Vector2::operator <= (const Vector2& v) const {
  return x <= v.x && y <= v.y;
}

FUN_ALWAYS_INLINE bool Vector2::operator >= (const Vector2& v) const {
  return x >= v.x && y >= v.y;
}

FUN_ALWAYS_INLINE bool Vector2::Equals(const Vector2& v, float tolerance) const {
  return  Math::Abs(x - v.x) <= tolerance &&
          Math::Abs(y - v.y) <= tolerance;
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator -() const {
  return Vector2(-x, -y);
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator += (const Vector2& v) {
  x += v.x;
  y += v.y;
  return *this;
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator -= (const Vector2& v) {
  x -= v.x;
  y -= v.y;
  return *this;
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator *= (float scale) {
  x *= scale;
  y *= scale;
  return *this;
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator /= (float scale) {
  const float one_over_scale = 1.f / scale;
  x *= one_over_scale;
  y *= one_over_scale;
  return *this;
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator *= (const Vector2& v) {
  x *= v.x;
  y *= v.y;
  return *this;
}

FUN_ALWAYS_INLINE Vector2 Vector2::operator /= (const Vector2& v) {
  x /= v.x;
  y /= v.y;
  return *this;
}

FUN_ALWAYS_INLINE float& Vector2::operator[](int32 index) {
  fun_check(index >= 0 && index < 2);
  return index == 0 ? x : y;
}

FUN_ALWAYS_INLINE float Vector2::operator[](int32 index) const {
  fun_check(index >= 0 && index < 2);
  return index == 0 ? x : y;
}

FUN_ALWAYS_INLINE void Vector2::Set(float x, float y) {
  this->x = x;
  this->y = y;
}

FUN_ALWAYS_INLINE float Vector2::GetMax() const {
  return Math::Max(x, y);
}

FUN_ALWAYS_INLINE float Vector2::GetAbsMax() const {
  return Math::Max(Math::Abs(x), Math::Abs(y));
}

FUN_ALWAYS_INLINE float Vector2::GetMin() const {
  return Math::Min(x, y);
}

FUN_ALWAYS_INLINE float Vector2::Size() const {
  return Math::Sqrt(x*x + y*y);
}

FUN_ALWAYS_INLINE float Vector2::SizeSquared() const {
  return x*x + y*y;
}

FUN_ALWAYS_INLINE Vector2 Vector2::GetRotated(const float degrees) const {
  // Based on Vector::RotateAngleAxis with axis(0, 0, 1)

  float s, c;
  Math::SinCos(&s, &c, Math::DegreesToRadians(degrees));

  //TODO 이게 사용이 원래 안되는건지...??
  //const float OMC = 1.f - c;
  return Vector2(c * x - s * y, s * x + c * y);
}

FUN_ALWAYS_INLINE Vector2 Vector2::GetSafeNormal(float tolerance) const {
  const float length_sqr = x*x + y*y;
  if (length_sqr > tolerance) {
    const float scale = Math::InvSqrt(length_sqr);
    return Vector2(x*scale, y*scale);
  }

  return Vector2(0.f, 0.f);
}

FUN_ALWAYS_INLINE void Vector2::Normalize(float tolerance) {
  const float length_sqr = x*x + y*y;
  if (length_sqr > tolerance) {
    const float scale = Math::InvSqrt(length_sqr);
    x *= scale;
    y *= scale;
  } else {
    x = 0.f;
    y = 0.f;
  }
}

FUN_ALWAYS_INLINE void Vector2::ToDirectionAndLength(Vector2& out_dir, float& out_length) const {
  out_length = Size();
  if (out_length > SMALL_NUMBER) {
    float one_over_length = 1.f / out_length;
    out_dir = Vector2(x*one_over_length, y*one_over_length);
  } else {
    out_dir = Vector2::ZeroVector;
  }
}

FUN_ALWAYS_INLINE bool Vector2::IsNearlyZero(float tolerance) const {
  return Math::Abs(x) <= tolerance && Math::Abs(y) <= tolerance;
}

FUN_ALWAYS_INLINE bool Vector2::IsZero() const {
  return x == 0.f && y == 0.f;
}

FUN_ALWAYS_INLINE float& Vector2::Component(int32 index) {
  return (&x)[index];
}

FUN_ALWAYS_INLINE float Vector2::Component(int32 index) const {
  return (&x)[index];
}

FUN_ALWAYS_INLINE IntPoint Vector2::ToIntPoint() const {
  return IntPoint(Math::RoundToInt(x), Math::RoundToInt(y));
}

FUN_ALWAYS_INLINE Vector2 Vector2::ClampAxes(float min_axis_val, float max_axis_val) const {
  return Vector2( Math::Clamp(x, min_axis_val, max_axis_val),
                  Math::Clamp(y, min_axis_val, max_axis_val));
}

FUN_ALWAYS_INLINE Vector2 Vector2::GetSignVector() const {
  return Vector2(Math::FloatSelect(x, 1.f, -1.f), Math::FloatSelect(y, 1.f, -1.f));
}

FUN_ALWAYS_INLINE String Vector2::ToString() const {
  return String::Format("x=%3.3f y=%3.3f", x, y);
}

FUN_ALWAYS_INLINE bool Vector2::InitFromString(const String& string) {
  x = y = 0.f;

  // The initialization is only successful if the x and y values
  // can all be parsed from the string
  const bool ok =
    Parse::Value(*string, "x=", x) &&
    Parse::Value(*string, "y=", y);

  return ok;
}

} // namespace fun
