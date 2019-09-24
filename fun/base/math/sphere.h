#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Implements a basic sphere.
 */
class Sphere {
 public:
  Vector center;
  float radius;

  Sphere() {}

  Sphere(int32) : center(0.f, 0.f, 0.f), radius(0) {}

  Sphere(const Vector& center, float radius)
    : center(center), radius(radius) {}

  explicit FUN_ALWAYS_INLINE Sphere(ForceInit_TAG)
    : center(ForceInit), radius(0.f) {}

  FUN_BASE_API Sphere(const Vector* points, int32 count);

  bool Equals(const Sphere& sphere, float tolerance = KINDA_SMALL_NUMBER) const {
    return center.Equals(sphere.center, tolerance) && Math::Abs(radius - sphere.radius) <= tolerance;
  }

  bool IsInside(const Sphere& other, float tolerance = KINDA_SMALL_NUMBER) const {
    if (radius > other.radius + tolerance) {
      return false;
    }

    return (center - other.center).SizeSquared() <= Math::Square(other.radius + tolerance - radius);
  }

  bool IsInside(const Vector& point, float tolerance = KINDA_SMALL_NUMBER) const {
    return (center - point).SizeSquared() <= Math::Square(radius + tolerance);
  }

  FUN_ALWAYS_INLINE bool Intersects(const Sphere& other, float tolerance = KINDA_SMALL_NUMBER) const {
    return (center - other.center).SizeSquared() <= Math::Square(Math::Max(0.f, other.radius + radius + tolerance));
  }

  FUN_BASE_API Sphere TransformBy(const Matrix& m) const;
  FUN_BASE_API Sphere TransformBy(const Transform& m) const;

  FUN_BASE_API float GetVolume() const;

  FUN_BASE_API Sphere& operator += (const Sphere& other);
  Sphere operator + (const Sphere& other) const {
    return Sphere(*this) += other;
  }

  friend Archive& operator & (Archive& ar, Sphere& sphere) {
    ar & sphere.center & sphere.radius;
    return ar;
  }
};


/**
 * CapsuleShape
 */
struct CapsuleShape {
  Vector center;
  float radius;
  Vector orientation;
  float length;

  CapsuleShape( const Vector& center,
                float radius,
                const Vector& orientation,
                float length)
    : center(center),
      radius(radius),
      orientation(orientation),
      length(length) {}
};

} // namespace fun
