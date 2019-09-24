#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Structure for a combined axis aligned bounding box and
 * bounding sphere with the same origin. (28 bytes).
 */
class BoxSphereBounds {
 public:
  /** Holds the origin of the bounding box and sphere. */
  Vector origin;

  /** Holds the extent of the bounding box. */
  Vector box_extent;

  /** Holds the radius of the bounding sphere. */
  float sphere_radius;

  /** Default constructor. */
  BoxSphereBounds() {
    // NOOP
  }

  /**
   * Creates and initializes a new instance.
   *
   * \param ForceInit_TAG - Force Init Enum.
   */
  explicit FUN_ALWAYS_INLINE BoxSphereBounds(ForceInit_TAG)
    : origin(ForceInit),
      box_extent(ForceInit),
      sphere_radius(0.f) {
    DiagnosticCheckNaN();
  }

  /**
   * Creates and initializes a new instance from the specified parameters.
   *
   * \param origin - origin of the bounding box and sphere.
   * \param box_extent - half size of box.
   * \param sphere_radius - radius of the sphere.
   */
  BoxSphereBounds(const Vector& origin, const Vector& box_extent, float sphere_radius)
    : origin(origin),
      box_extent(box_extent),
      sphere_radius(sphere_radius) {
    DiagnosticCheckNaN();
  }

  /**
   * Creates and initializes a new instance from the given box and sphere.
   *
   * \param box - The bounding box.
   * \param sphere - The bounding sphere.
   */
  BoxSphereBounds(const Box& box, const Sphere& sphere) {
    box.GetCenterAndExtents(origin, box_extent);
    sphere_radius = Math::Min(box_extent.Size(), (sphere.center - origin).Size() + sphere.radius);

    DiagnosticCheckNaN();
  }

  /**
   * Creates and initializes a new instance the given box.
   *
   * The sphere radius is taken from the extent of the box.
   *
   * \param box - The bounding box.
   */
  BoxSphereBounds(const Box& box) {
    box.GetCenterAndExtents(origin, box_extent);
    sphere_radius = box_extent.Size();

    DiagnosticCheckNaN();
  }

  /**
   * Creates and initializes a new instance for the given sphere.
   */
  BoxSphereBounds(const Sphere& sphere) {
    origin = sphere.center;
    box_extent = Vector(sphere.radius);
    sphere_radius = sphere.radius;

    DiagnosticCheckNaN();
  }

  /**
   * Creates and initializes a new instance from the given set of points.
   *
   * The sphere radius is taken from the extent of the box.
   *
   * \param points - The points to be considered for the bounding box.
   * \param count - Number of points in the points array.
   */
  BoxSphereBounds(const Vector* points, uint32 count);

  /**
   * Constructs a bounding volume containing both this and b.
   *
   * \param other - The other bounding volume.
   *
   * \return The combined bounding volume.
   */
  FUN_ALWAYS_INLINE BoxSphereBounds operator + (const BoxSphereBounds& other) const;

  /**
   * Calculates the squared distance from a point to a bounding box
   *
   * \param point - The point.
   *
   * \return The distance.
   */
  FUN_ALWAYS_INLINE float ComputeSquaredDistanceFromBoxToPoint(const Vector& point) const {
    const Vector mins = origin - box_extent;
    const Vector maxs = origin + box_extent;
    return fun::ComputeSquaredDistanceFromBoxToPoint(mins, maxs, point);
  }

  /**
   * Test whether the spheres from two BoxSphereBounds intersect/overlap.
   *
   * \param a - First BoxSphereBounds to test.
   * \param b - Second BoxSphereBounds to test.
   * \param tolerance - Error tolerance added to test distance.
   *
   * \return true if spheres intersect, false otherwise.
   */
  FUN_ALWAYS_INLINE static bool SpheresIntersect(const BoxSphereBounds& a, const BoxSphereBounds& b, float tolerance = KINDA_SMALL_NUMBER) {
    return (a.origin - b.origin).SizeSquared() <= Math::Square(Math::Max(0.f, a.sphere_radius + b.sphere_radius + tolerance));
  }

  /**
   * Test whether the boxes from two BoxSphereBounds intersect/overlap.
   *
   * \param a - First BoxSphereBounds to test.
   * \param b - Second BoxSphereBounds to test.
   *
   * \return true if boxes intersect, false otherwise.
   */
  FUN_ALWAYS_INLINE static bool BoxesIntersect(const BoxSphereBounds& a, const BoxSphereBounds& b) {
    return a.GetBox().Intersect(b.GetBox());
  }

  /**
   * Gets the bounding box.
   *
   * \return The bounding box.
   */
  FUN_ALWAYS_INLINE Box GetBox() const {
    return Box(origin - box_extent, origin + box_extent);
  }

  /**
   * Gets the extrema for the bounding box.
   *
   * \param extrema - 1 for positive extrema from the origin, else negative
   *
   * \return The boxes extrema
   */
  Vector GetBoxExtrema(uint32 extrema) const {
    if (extrema) {
      return origin + box_extent;
    }

    return origin - box_extent;
  }

  /**
   * Gets the bounding sphere.
   *
   * \return The bounding sphere.
   */
  FUN_ALWAYS_INLINE Sphere GetSphere() const {
    return Sphere(origin, sphere_radius);
  }

  /**
   * Increase the size of the box and sphere by a given size.
   *
   * \param amount - The size to increase by.
   *
   * \return a new box with the expanded size.
   */
  FUN_ALWAYS_INLINE BoxSphereBounds ExpandBy(float amount) const {
    return BoxSphereBounds(origin, box_extent + amount, sphere_radius + amount);
  }

  /**
   * Gets a bounding volume transformed by a matrix.
   *
   * \param m - The matrix.
   *
   * \return The transformed volume.
   */
  FUN_BASE_API BoxSphereBounds TransformBy(const Matrix& m) const;

  /**
   * Gets a bounding volume transformed by a Transform object.
   *
   * \param m - The Transform object.
   *
   * \return The transformed volume.
   */
  FUN_BASE_API BoxSphereBounds TransformBy(const Transform& m) const;

  /**
   * Get a textual representation of this bounding box.
   *
   * \return Text describing the bounding box.
   */
  String ToString() const;

  /**
   * Constructs a bounding volume containing both a and b.
   *
   * This is a legacy version of the function used to compute primitive bounds, to avoid the need to rebuild lighting after the change.
   */
  friend BoxSphereBounds Union(const BoxSphereBounds& a, const BoxSphereBounds& b) {
    Box bounding_box(0);

    bounding_box += (a.origin - a.box_extent);
    bounding_box += (a.origin + a.box_extent);
    bounding_box += (b.origin - b.box_extent);
    bounding_box += (b.origin + b.box_extent);

    // Build a bounding sphere from the bounding box's origin and the radii of a and b.
    BoxSphereBounds result(bounding_box);

    result.sphere_radius = Math::Min(result.sphere_radius, Math::Max((a.origin - result.origin).Size() + a.sphere_radius, (b.origin - result.origin).Size() + b.sphere_radius));
    result.DiagnosticCheckNaN();

    return result;
  }

#if FUN_ENABLE_NAN_DIAGNOSTIC
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() const {
    if (origin.ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("origin contains NaN: %s", *origin.ToString());
      const_cast<BoxSphereBounds*>(this)->origin = Vector::ZeroVector;
    }

    if (box_extent.ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("box_extent contains NaN: %s", *box_extent.ToString());
      const_cast<BoxSphereBounds*>(this)->box_extent = Vector::ZeroVector;
    }

    if (Math::IsNaN(sphere_radius) || !Math::IsFinite(sphere_radius)) {
      LOG_OR_ENSURE_NAN_ERROR("sphere_radius contains NaN: %f", sphere_radius);
      const_cast<BoxSphereBounds*>(this)->sphere_radius = 0.f;
    }
  }
#else
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() const {}
#endif

  FUN_ALWAYS_INLINE bool ContainsNaN() const {
    return  origin.ContainsNaN() ||
            box_extent.ContainsNaN() ||
            Math::IsNaN(sphere_radius) ||
            !Math::IsFinite(sphere_radius);
  }

 public:
  friend Archive& operator & (Archive& ar, BoxSphereBounds& bounds) {
    return ar & bounds.origin & bounds.box_extent & bounds.sphere_radius;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE BoxSphereBounds::BoxSphereBounds(const Vector* points, uint32 count) {
  Box bounding_box(0);

  // find an axis aligned bounding box for the points.
  for (uint32 i = 0; i < count; ++i) {
    bounding_box += points[i];
  }

  bounding_box.GetCenterAndExtents(origin, box_extent);

  // using the center of the bounding box as the origin of the sphere,
  // find the radius of the bounding sphere.
  sphere_radius = 0.f;

  for (uint32 i = 0; i < count; ++i) {
    sphere_radius = Math::Max(sphere_radius, (points[i] - origin).Size());
  }

  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE BoxSphereBounds BoxSphereBounds::operator + (const BoxSphereBounds& other) const {
  Box bounding_box(0);

  bounding_box += (this->origin - this->box_extent);
  bounding_box += (this->origin + this->box_extent);
  bounding_box += (other.origin - other.box_extent);
  bounding_box += (other.origin + other.box_extent);

  // build a bounding sphere from the bounding box's origin and
  // the radii of a and b.
  BoxSphereBounds result(bounding_box);

  result.sphere_radius = Math::Min(result.sphere_radius, Math::Max((origin - result.origin).Size() + sphere_radius, (other.origin - result.origin).Size() + other.sphere_radius));
  result.DiagnosticCheckNaN();

  return result;
}

FUN_ALWAYS_INLINE String BoxSphereBounds::ToString() const {
  return String::Format("origin={0}, box_extent=({1}), sphere_radius=({2})",
                  origin.ToString(), box_extent.ToString(), sphere_radius);
}

template <> struct IsPOD<BoxSphereBounds> { enum { Value = true }; };

} // namespace fun
