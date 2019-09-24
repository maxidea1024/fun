#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Implements an axis-aligned box.
 *
 * Boxes describe an axis-aligned extent in three dimensions. They are used for many different things in the
 * Engine and in games, such as bounding volumes, collision detection and visibility calculation.
 */
class Box {
 public:
  Vector min;
  Vector max;
  uint8 is_valid;

  Box() {
    // NOOP
  }

  Box(int32) {
    Init();
  }

  explicit Box(ForceInit_TAG) {
    Init();
  }

  Box(const Vector& min, const Vector& max)
    : min(min), max(max), is_valid(1) {}

  FUN_BASE_API Box(const Vector* points, int32 count);
  FUN_BASE_API Box(const Array<Vector>& points);

  bool operator == (const Box& other) const {
    return min == other.min && max == other.max;
  }

  FUN_ALWAYS_INLINE Box& operator += (const Vector& other);

  Box operator + (const Vector& other) const {
    return Box(*this) += other;
  }

  FUN_ALWAYS_INLINE Box& operator += (const Box& other);

  Box operator + (const Box& other) const {
    return Box(*this) += other;
  }

  Vector& operator [] (int32 index) {
    fun_check(index >= 0 && index < 2);
    return index == 0 ? min : max;
  }

  /**
   * Calculates the distance of a point to this box.
   *
   * \param point - The point.
   *
   * \return The distance.
   */
  FUN_ALWAYS_INLINE float ComputeSquaredDistanceToPoint(const Vector& point) const {
    return ComputeSquaredDistanceFromBoxToPoint(min, max, point);
  }

  /**
   * Increases the box size.
   *
   * \param w - The size to increase the volume by.
   *
   * \return A new bounding box.
   */
  Box ExpandBy(float w) const {
    return Box(min - Vector(w), max + Vector(w));
  }

  /**
   * Increases the box size.
   *
   * \param v - The size to increase the volume by.
   *
   * \return A new bounding box.
   */
  Box ExpandBy(const Vector& v) const {
    return Box(min - v, max + v);
  }

  /**
   * Shifts the bounding box position.
   *
   * \param offset - The vector to shift the box by.
   *
   * \return A new bounding box.
   */
  Box ShiftBy(const Vector& offset) const {
    return Box(min + offset, max + offset);
  }

  /**
   * Moves the center of bounding box to new destination.
   *
   * \param destination - The destination point to move center of box to.
   *
   * \return A new bounding box.
   */
  Box MoveTo(const Vector& destination) const {
    const Vector offset = destination - GetCenter();
    return Box(min + offset, max + offset);
  }

  /**
   * Gets the center point of this box.
   *
   * \return The center point.
   *
   * @see GetCenterAndExtents, GetExtent, GetSize, GetVolume
   */
  Vector GetCenter() const {
    return (min + max) * 0.5f;
  }

  /**
   * Gets the center and extents of this box.
   *
   * \param out_center - [out] Will contain the box center point.
   * \param out_extents - [out] Will contain the extent around the center.
   *
   * @see GetCenter, GetExtent, GetSize, GetVolume
   */
  void GetCenterAndExtents(Vector& out_center, Vector& out_extents) const {
    out_extents = GetExtent();
    out_center = min + out_extents;
  }

  /**
   * Calculates the closest point on or inside the box to a given point in space.
   *
   * \param point - The point in space.
   *
   * \return The closest point on or inside the box.
   */
  FUN_ALWAYS_INLINE Vector GetClosestPointTo(const Vector& point) const;

  /**
   * Gets the extents of this box.
   *
   * \return The box extents.
   *
   * @see GetCenter, GetCenterAndExtents, GetSize, GetVolume
   */
  Vector GetExtent() const {
    return (max - min) * 0.5f;
  }

  /**
   * Gets a reference to the specified point of the bounding box.
   *
   * \param point_index - The index of the extrema point to return.
   *
   * \return A reference to the point.
   */
  Vector& GetExtrema(int32 point_index) {
    return (&min)[point_index];
  }

  /**
   * Gets a read-only reference to the specified point of the bounding box.
   *
   * \param point_index - The index of extrema point to return.
   *
   * \return A read-only reference to the point.
   */
  const Vector& GetExtrema(int32 point_index) const {
    return (&min)[point_index];
  }

  /**
   * Gets the size of this box.
   *
   * \return The box size.
   *
   * @see GetCenter, GetCenterAndExtents, GetExtent, GetVolume
   */
  Vector GetSize() const {
    return max - min;
  }

  /**
   * Gets the volume of this box.
   *
   * \return The box volume.
   *
   * @see GetCenter, GetCenterAndExtents, GetExtent, GetSize
   */
  float GetVolume() const {
    return (max.x - min.x) * (max.y - min.y) * (max.z - min.z);
  }

  /**
   * Set the initial values of the bounding box to Zero.
   */
  void Init() {
    min = max = Vector::ZeroVector;
    is_valid = false;
  }

  /**
   * Checks whether the given bounding box intersects this bounding box.
   *
   * \param other - The bounding box to intersect with.
   *
   * \return true if the boxes intersect, false otherwise.
   */
  FUN_ALWAYS_INLINE bool Intersect(const Box& other) const;

  /**
   * Checks whether the given bounding box intersects this bounding box in the XY plane.
   *
   * \param other - The bounding box to test intersection.
   *
   * \return true if the boxes intersect in the XY plane, false otherwise.
   */
  FUN_ALWAYS_INLINE bool IntersectXY(const Box& other) const;

  /**
   * Returns the overlap Box of two box
   *
   * \param other - The bounding box to test overlap
   *
   * \return the overlap box. It can be 0 if they don't overlap
   */
  FUN_BASE_API Box Overlap(const Box& other) const;

  /**
   * Gets a bounding volume transformed by an inverted Transform object.
   *
   * \param m - The transformation object to perform the inversely transform this box with.
   *
   * \return The transformed box.
   */
  FUN_BASE_API Box InverseTransformBy(const Transform& m) const;

  /**
   * Checks whether the given location is inside this box.
   *
   * \param point - The location to test for inside the bounding volume.
   *
   * \return true if location is inside this volume.
   *
   * @see IsInsideXY
   */
  bool IsInside(const Vector& point) const {
    return  point.x > min.x &&
            point.x < max.x &&
            point.y > min.y &&
            point.y < max.y &&
            point.z > min.z &&
            point.z < max.z;
  }

  /**
   * Checks whether the given location is inside or on this box.
   *
   * \param point - The location to test for inside the bounding volume.
   *
   * \return true if location is inside this volume.
   *
   * @see IsInsideXY
   */
  bool IsInsideOrOn(const Vector& point) const {
    return  point.x >= min.x &&
            point.x <= max.x &&
            point.y >= min.y &&
            point.y <= max.y &&
            point.z >= min.z &&
            point.z <= max.z;
  }

  /**
   * Checks whether a given box is fully encapsulated by this box.
   *
   * \param other - The box to test for encapsulation within the bounding volume.
   *
   * \return true if box is inside this volume.
   */
  bool IsInside(const Box& other) const {
    return IsInside(other.min) && IsInside(other.max);
  }

  /**
   * Checks whether the given location is inside this box in the XY plane.
   *
   * \param In - The location to test for inside the bounding box.
   *
   * \return true if location is inside this box in the XY plane.
   *
   * @see IsInside
   */
  bool IsInsideXY(const Vector& point) const {
    return  point.x > min.x &&
            point.x < max.x &&
            point.y > min.y &&
            point.y < max.y;
  }

  /**
   * Checks whether the given box is fully encapsulated by this box in the XY plane.
   *
   * \param other - The box to test for encapsulation within the bounding box.
   *
   * \return true if box is inside this box in the XY plane.
   */
  bool IsInsideXY(const Box& other) const {
    return IsInsideXY(other.min) && IsInsideXY(other.max);
  }

  /**
   * Gets a bounding volume transformed by a matrix.
   *
   * \param m - The matrix to transform by.
   *
   * \return The transformed box.
   *
   * @see TransformProjectBy
   */
  FUN_BASE_API Box TransformBy(const Matrix& m) const;

  /**
   * Gets a bounding volume transformed by a Transform object.
   *
   * \param m - The transformation object.
   *
   * \return The transformed box.
   *
   * @see TransformProjectBy
   */
  FUN_BASE_API Box TransformBy(const Transform& m) const;

  /**
   * Transforms and projects a world bounding box to screen space
   *
   * \param m - The projection matrix.
   *
   * \return The transformed box.
   *
   * @see TransformBy
   */
  FUN_BASE_API Box TransformProjectBy(const Matrix& m) const;

  /**
   * Get a textual representation of this box.
   *
   * \return A string describing the box.
   */
  String ToString() const;

  /**
   * Utility function to build an aabb from origin and Extent
   *
   * \param origin - The location of the bounding box.
   * \param extent - Half size of the bounding box.
   *
   * \return A new axis-aligned bounding box.
   */
  static Box BuildAABB(const Vector& origin, const Vector& extent) {
    return Box(origin - extent, origin + extent);
  }

  /**
   * Serializes the bounding box.
   *
   * \param ar - The archive to serialize into.
   * \param box - The box to serialize.
   *
   * \return Reference to the Archive after serialization.
   */
  friend Archive& operator & (Archive& ar, Box& box) {
    return ar & box.min & box.max & box.is_valid;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }
};

template <> struct IsPOD<Box> { enum { Value = true }; };


//
// inlines
//

FUN_ALWAYS_INLINE Box& Box::operator += (const Vector& other) {
  if (is_valid) {
    min.x = Math::Min(min.x, other.x);
    min.y = Math::Min(min.y, other.y);
    min.z = Math::Min(min.z, other.z);

    max.x = Math::Max(max.x, other.x);
    max.y = Math::Max(max.y, other.y);
    max.z = Math::Max(max.z, other.z);
  } else {
    min = max = other;
    is_valid = true;
  }

  return *this;
}

FUN_ALWAYS_INLINE Box& Box::operator += (const Box& other) {
  if (is_valid && other.is_valid) {
    min.x = Math::Min(min.x, other.min.x);
    min.y = Math::Min(min.y, other.min.y);
    min.z = Math::Min(min.z, other.min.z);

    max.x = Math::Max(max.x, other.max.x);
    max.y = Math::Max(max.y, other.max.y);
    max.z = Math::Max(max.z, other.max.z);
  } else if (other.is_valid) {
    *this = other;
  }

  return *this;
}

FUN_ALWAYS_INLINE Vector Box::GetClosestPointTo(const Vector& point) const {
  // Start by considering the point inside the box
  Vector closest_point = point;

  // Now clamp to inside box if it's outside
  if (point.x < min.x) {
    closest_point.x = min.x;
  } else if (point.x > max.x) {
    closest_point.x = max.x;
  }

  // Now clamp to inside box if it's outside
  if (point.y < min.y) {
    closest_point.y = min.y;
  } else if (point.y > max.y) {
    closest_point.y = max.y;
  }

  // Now clamp to inside box if it's outside.
  if (point.z < min.z) {
    closest_point.z = min.z;
  } else if (point.z > max.z) {
    closest_point.z = max.z;
  }

  return closest_point;
}

FUN_ALWAYS_INLINE bool Box::Intersect(const Box& other) const {
  if (min.x > other.max.x || other.min.x > max.x) {
    return false;
  }

  if (min.y > other.max.y || other.min.y > max.y) {
    return false;
  }

  if (min.z > other.max.z || other.min.z > max.z) {
    return false;
  }

  return true;
}

FUN_ALWAYS_INLINE bool Box::IntersectXY(const Box& other) const {
  if (min.x > other.max.x || other.min.x > max.x) {
    return false;
  }

  if (min.y > other.max.y || other.min.y > max.y) {
    return false;
  }

  return true;
}

FUN_ALWAYS_INLINE String Box::ToString() const {
  return String::Format("is_valid={0}, min=({1}), max=({2})",
                  is_valid ? "true" : "false", min.ToString(), max.ToString());
}

} // namespace fun
