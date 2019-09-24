#pragma once

#include "fun/base/base.h"
#include "fun/base/math/vector.h"

namespace fun {

/**
 * Structure for three dimensional planes.
 *
 * Stores the coeffecients as Ax+By+Cz=D.
 * Note that this is different than many other plane classes that use Ax+By+Cz+D=0.
 */
class alignas(16) Plane : public Vector {
 public:
  float w;

  Plane();
  Plane(const Plane& p);
  Plane(const Vector4& v4);
  Plane(float x, float y, float z, float w);
  Plane(const Vector& normal, float w);
  Plane(const Vector& base, const Vector& normal);
  Plane(const Vector& a, const Vector& b, const Vector& c);
  explicit Plane(ForceInit_TAG);

  float PlaneDot(const Vector& p) const;
  Plane Flip() const;
  Plane TransformBy(const Matrix& m) const;
  Plane TransformByUsingAdjointT(const Matrix& m, float det_m, const Matrix& ta) const;

  bool operator == (const Plane& p) const;
  bool operator != (const Plane& p) const;
  bool Equals(const Plane& p, float tolerance = KINDA_SMALL_NUMBER) const;

  float operator | (const Plane& p) const;
  Plane operator + (const Plane& p) const;
  Plane operator - (const Plane& p) const;
  Plane operator / (float scale) const;
  Plane operator * (float scale) const;
  Plane operator * (const Plane& p);
  Plane operator += (const Plane& p);
  Plane operator -= (const Plane& p);
  Plane operator *= (float scale);
  Plane operator *= (const Plane& p);
  Plane operator /= (float v);

  friend Archive& operator & (Archive& ar, Plane& p) {
    return ar & (Vector&)p & p.w;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }

  bool NetSerialize(Archive& ar, class RPackageMap*, bool& out_success) {
    if (ar.IsLoading()) {
      int16 ix;
      int16 iy;
      int16 iz;
      int16 iw;
      ar & ix & iy & iz & iw;
      *this = Plane(ix, iy, iz, iw);
    } else {
      int16 ix(Math::RoundToInt(x));
      int16 iy(Math::RoundToInt(y));
      int16 iz(Math::RoundToInt(z));
      int16 iw(Math::RoundToInt(w));
      ar & ix & iy & iz & iw;
    }
    out_success = true;
    return true;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE Plane::Plane() {}

FUN_ALWAYS_INLINE Plane::Plane(const Plane& p) : Vector(p), w(p.w) {}

FUN_ALWAYS_INLINE Plane::Plane(const Vector4& v4) : Vector(v4), w(v4.w) {}

FUN_ALWAYS_INLINE Plane::Plane(float x, float y, float z, float w)
  : Vector(x, y, z), w(w) {}

FUN_ALWAYS_INLINE Plane::Plane(const Vector& normal, float w)
  : Vector(normal), w(w) {}

FUN_ALWAYS_INLINE Plane::Plane(const Vector& base, const Vector& normal)
  : Vector(normal), w(base | normal) {}

FUN_ALWAYS_INLINE Plane::Plane(const Vector& a, const Vector& b, const Vector& c)
  : Vector(((b - a) ^ (c - a)).GetSafeNormal()) {
  w = a | (Vector)(*this);
}

FUN_ALWAYS_INLINE Plane::Plane(ForceInit_TAG)
  : Vector(ForceInit), w(0.f) {}

FUN_ALWAYS_INLINE float Plane::PlaneDot(const Vector& p) const {
  return x*p.x + y*p.y + z*p.z - w;
}

FUN_ALWAYS_INLINE Plane Plane::Flip() const {
  return Plane(-x, -y, -z, -w);
}

FUN_ALWAYS_INLINE bool Plane::operator == (const Plane& p) const {
  return x == p.x && y == p.y && z == p.z && w == p.w;
}

FUN_ALWAYS_INLINE bool Plane::operator != (const Plane& p) const {
  return x != p.x || y != p.y || z != p.z || w != p.w;
}

FUN_ALWAYS_INLINE bool Plane::Equals(const Plane& p, float tolerance) const {
  const float diff_x = Math::Abs(x - p.x);
  const float diff_y = Math::Abs(y - p.y);
  const float diff_z = Math::Abs(z - p.z);
  const float diff_w = Math::Abs(w - p.w);
  return diff_x < tolerance && diff_y < tolerance && diff_z < tolerance && diff_w < tolerance;
}

FUN_ALWAYS_INLINE float Plane::operator | (const Plane& p) const {
  return x*p.x + y*p.y + z*p.z + w*p.w;
}

FUN_ALWAYS_INLINE Plane Plane::operator + (const Plane& p) const {
  return Plane(x + p.x, y + p.y, z + p.z, w + p.w);
}

FUN_ALWAYS_INLINE Plane Plane::operator - (const Plane& p) const {
  return Plane(x - p.x, y - p.y, z - p.z, w - p.w);
}

FUN_ALWAYS_INLINE Plane Plane::operator / (float scale) const {
  const float one_over_scale = 1.f / scale;
  return Plane( x * one_over_scale,
                y * one_over_scale,
                z * one_over_scale,
                w * one_over_scale);
}

FUN_ALWAYS_INLINE Plane Plane::operator * (float scale) const {
  return Plane(x * scale, y * scale, z * scale, w * scale);
}

FUN_ALWAYS_INLINE Plane Plane::operator * (const Plane& p) {
  return Plane (x*p.x, y*p.y, z*p.z, w*p.w);
}

FUN_ALWAYS_INLINE Plane Plane::operator += (const Plane& p) {
  x += p.x; y += p.y; z += p.z; w += p.w;
  return *this;
}

FUN_ALWAYS_INLINE Plane Plane::operator -= (const Plane& p) {
  x -= p.x; y -= p.y; z -= p.z; w -= p.w;
  return *this;
}

FUN_ALWAYS_INLINE Plane Plane::operator *= (float scale) {
  x *= scale;
  y *= scale;
  z *= scale;
  w *= scale;
  return *this;
}

FUN_ALWAYS_INLINE Plane Plane::operator *= (const Plane& p) {
  x *= p.x;
  y *= p.y;
  z *= p.z;
  w *= p.w;
  return *this;
}

FUN_ALWAYS_INLINE Plane Plane::operator /= (float scale) {
  const float one_over_scale = 1.f / scale;
  x *= one_over_scale;
  y *= one_over_scale;
  z *= one_over_scale;
  w *= one_over_scale;
  return *this;
}

template <> struct IsPOD<Plane> { enum { Value = true }; };

} // namespace fun
