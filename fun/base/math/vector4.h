#pragma once

#include "fun/base/base.h"
#include "fun/base/parse.h"
#include "fun/base/math/math_util.h"

namespace fun {

/**
 * A 4D homogeneous vector, 4x1 FLOATs, 16-byte aligned.
 */
class alignas(16) Vector4 {
 public:
  float x;
  float y;
  float z;
  float w;

  Vector4(const Vector& v3, float w = 1.f);
  Vector4(const LinearColor& color);
  explicit Vector4(float x = 0.f, float y = 0.f, float z = 0.f, float w = 1.f);
  explicit Vector4(const Vector2& xy, const Vector2& zw);
  explicit Vector4(ForceInit_TAG);

  float& operator[](int32 index);
  float operator[](int32 index) const;

  Vector4 operator -() const;
  Vector4 operator + (const Vector4& v) const;
  Vector4 operator += (const Vector4& v);
  Vector4 operator - (const Vector4& v) const;
  Vector4 operator * (float scale) const;
  Vector4 operator / (float scale) const;
  Vector4 operator / (const Vector4& v) const;
  Vector4 operator * (const Vector4& v) const;
  Vector4 operator *= (const Vector4& v);
  Vector4 operator /= (const Vector4& v);
  Vector4 operator *= (float scale);

  FUN_ALWAYS_INLINE friend float Dot3(const Vector4& v1, const Vector4& v2) {
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
  }

  FUN_ALWAYS_INLINE friend float Dot4(const Vector4& v1, const Vector4& v2) {
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
  }

  FUN_ALWAYS_INLINE friend Vector4 operator * (float scale, const Vector4& v) {
    return v.operator*(scale);
  }

  bool operator == (const Vector4& v) const;
  bool operator != (const Vector4& v) const;

  Vector4 operator ^ (const Vector4& v) const;

  float& Component(int32 index);

  bool Equals(const Vector4& v, float tolerance = KINDA_SMALL_NUMBER) const;

  bool IsUnit3(float length_sqr_tolerance = KINDA_SMALL_NUMBER) const;

  String ToString() const;

  bool InitFromString(const String& string);

  Vector4 GetSafeNormal(float tolerance = SMALL_NUMBER) const;
  Vector4 GetUnsafeNormal3() const;

  FUN_BASE_API Rotator ToOrientationRotator() const;
  FUN_BASE_API Quat ToOrientationQuat() const;
  Rotator Rotation() const;

  void Set(float x, float y, float z, float w);

  float Size3() const;
  float SizeSquared3() const;

  bool ContainsNaN() const;

  bool IsNearlyZero3(float tolerance = KINDA_SMALL_NUMBER) const;

  Vector4 Reflect3(const Vector4& normal) const;

  void FindBestAxisVectors3(Vector4& axis1, Vector4& axis2) const;

#if FUN_ENABLE_NAN_DIAGNOSTIC
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() {
    if (ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Vector contains NaN: %s", *ToString());
      *this = Vector4(Vector::ZeroVector);
    }
  }
#else
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() {}
#endif

  friend Archive& operator & (Archive& ar, Vector4& v) {
    return ar & v.x & v.y & v.z & v.w;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE Vector4::Vector4(const Vector& v3, float w)
  : x(v3.x), y(v3.y), z(v3.z), w(w) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector4::Vector4(const LinearColor& color)
  : x(color.r)
  , y(color.g)
  , z(color.b)
  , w(color.a) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector4::Vector4(float x, float y, float z, float w)
  : x(x), y(y), z(z), w(w) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector4::Vector4(ForceInit_TAG)
  : x(0.f), y(0.f), z(0.f), w(0.f) {}

FUN_ALWAYS_INLINE Vector4::Vector4(const Vector2& xy, const Vector2& zw)
  : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE float& Vector4::operator[](int32 index) {
  return (&x)[index];
}

FUN_ALWAYS_INLINE float Vector4::operator[](int32 index) const {
  return (&x)[index];
}

FUN_ALWAYS_INLINE void Vector4::Set(float x, float y, float z, float w) {
  this->x = x;
  this->y = y;
  this->z = z;
  this->w = w;
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator - () const {
  return Vector4(-x, -y, -z, -w);
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator + (const Vector4& v) const {
  return Vector4(x + v.x, y + v.y, z + v.z, w + v.w);
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator += (const Vector4& v) {
  x += v.x;
  y += v.y;
  z += v.z;
  w += v.w;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator - (const Vector4& v) const {
  return Vector4(x - v.x, y - v.y, z - v.z, w - v.w);
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator * (float scale) const {
  return Vector4(x * scale, y * scale, z * scale, w * scale);
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator / (float scale) const {
  const float one_over_scale = 1.f / scale;
  return Vector4( x * one_over_scale,
                  y * one_over_scale,
                  z * one_over_scale,
                  w * one_over_scale);
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator * (const Vector4& v) const {
  return Vector4(x * v.x, y * v.y, z * v.z, w * v.w);
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator ^ (const Vector4& v) const {
  return Vector4( y * v.z - z * v.y,
                  z * v.x - x * v.z,
                  x * v.y - y * v.x,
                  0.f);
}

FUN_ALWAYS_INLINE float& Vector4::Component(int32 index) {
  return (&x)[index];
}

FUN_ALWAYS_INLINE bool Vector4::operator == (const Vector4& v) const {
  return x == v.x && y == v.y && z == v.z && w == v.w;
}

FUN_ALWAYS_INLINE bool Vector4::operator != (const Vector4& v) const {
  return x != v.x || y != v.y || z != v.z || w != v.w;
}

FUN_ALWAYS_INLINE bool Vector4::Equals(const Vector4& v, float tolerance) const {
  return  Math::Abs(x - v.x) <= tolerance &&
          Math::Abs(y - v.y) <= tolerance &&
          Math::Abs(z - v.z) <= tolerance &&
          Math::Abs(w - v.w) <= tolerance;
}

FUN_ALWAYS_INLINE String Vector4::ToString() const {
  return String::Format("x=%3.3f y=%3.3f z=%3.3f w=%3.3f", x, y, z, w);
}

FUN_ALWAYS_INLINE bool Vector4::InitFromString(const String& string) {
  x = y = z = 0.f;
  w = 1.f;

  // The initialization is only successful if the x, y, and z values can all be parsed from the string
  const bool ok =
    Parse::Value(*string, "x=", x) &&
    Parse::Value(*string, "y=", y) &&
    Parse::Value(*string, "z=", z);

  // w is optional, so don't factor in its presence (or lack thereof) in determining initialization success
  Parse::Value(*string, "w=", w);

  return ok;
}

FUN_ALWAYS_INLINE Vector4 Vector4::GetSafeNormal(float tolerance) const {
  const float length_sqr = x*x + y*y + z*z;
  if (length_sqr > tolerance) {
    const float scale = Math::InvSqrt(length_sqr);
    return Vector4(x*scale, y*scale, z*scale, 0.f);
  }
  return Vector4(0.f);
}

FUN_ALWAYS_INLINE Vector4 Vector4::GetUnsafeNormal3() const {
  const float scale = Math::InvSqrt(x*x + y*y + z*z);
  return Vector4(x*scale, y*scale, z*scale, 0.f);
}

FUN_ALWAYS_INLINE float Vector4::Size3() const {
  return Math::Sqrt(x*x + y*y + z*z);
}

FUN_ALWAYS_INLINE float Vector4::SizeSquared3() const {
  return x*x + y*y + z*z;
}

FUN_ALWAYS_INLINE bool Vector4::IsUnit3(float length_sqr_tolerance) const {
  return Math::Abs(1.f - SizeSquared3()) < length_sqr_tolerance;
}

FUN_ALWAYS_INLINE bool Vector4::ContainsNaN() const {
  return (Math::IsNaN(x) || !Math::IsFinite(x) ||
          Math::IsNaN(y) || !Math::IsFinite(y) ||
          Math::IsNaN(z) || !Math::IsFinite(z) ||
          Math::IsNaN(w) || !Math::IsFinite(w));
}

FUN_ALWAYS_INLINE bool Vector4::IsNearlyZero3(float tolerance) const {
  return  Math::Abs(x) <= tolerance &&
          Math::Abs(y) <= tolerance &&
          Math::Abs(z) <= tolerance;
}

FUN_ALWAYS_INLINE Vector4 Vector4::Reflect3(const Vector4& normal) const {
  return 2.f * Dot3(*this, normal) * normal - *this;
}

FUN_ALWAYS_INLINE void Vector4::FindBestAxisVectors3(Vector4& axis1, Vector4& axis2) const {
  const float nx = Math::Abs(x);
  const float ny = Math::Abs(y);
  const float nz = Math::Abs(z);

  // Find best basis vectors.
  if (nz > nx && nz > ny) {
    axis1 = Vector4(1, 0, 0);
  } else {
    axis1 = Vector4(0, 0, 1);
  }

  axis1 = (axis1 - *this * Dot3(axis1, *this)).GetSafeNormal();
  axis2 = axis1 ^ *this;
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator *= (const Vector4& v) {
  x *= v.x;
  y *= v.y;
  z *= v.z;
  w *= v.w;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator /= (const Vector4& v) {
  x /= v.x;
  y /= v.y;
  z /= v.z;
  w /= v.w;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator *= (float scale) {
  x *= scale;
  y *= scale;
  z *= scale;
  w *= scale;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Vector4 Vector4::operator / (const Vector4& v) const {
  return Vector4(x / v.x, y / v.y, z / v.z, w / v.w);
}

FUN_ALWAYS_INLINE uint32 HashOf(const Vector4& vector) {
  // Note: this assumes there's no padding in Vector that could contain uncompared data.
  return Crc::Crc32(&vector, sizeof vector);
}

template <> struct IsPOD<Vector4> { enum { Value = true }; };

} // namespace fun
