#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Implements a rectangular 2D box.
 */
class Box2 {
 public:
  /** Holds the box's minimum point. */
  Vector2 min;

  /** Holds the box's maximum point. */
  Vector2 max;

  /** Holds a flag indicating whether this box is valid. */
  bool is_valid;

  /**
   * Default constructor (no initialization).
   */
  Box2() {
    // NOOP
  }

  /**
   * Creates and initializes a new box.
   *
   * The box extents are initialized to zero and the box is marked as invalid.
   */
  Box2(int32) {
    Init();
  }

  /**
   * Creates and initializes a new box.
   *
   * The box extents are initialized to zero and the box is marked as invalid.
   *
   * \param ForceInit_TAG - Force Init Enum.
   */
  explicit Box2(ForceInit_TAG) {
    Init();
  }

  /**
   * Creates and initializes a new box from the specified parameters.
   *
   * \param InMin - The box's minimum point.
   * \param InMax - The box's maximum point.
   */
  Box2(const Vector2& min, const Vector2& max)
    : min(min),
      max(max),
      is_valid(true) {}

  /**
   * Creates and initializes a new box from the given set of points.
   *
   * \param points - Array of points to create for the bounding volume.
   * \param count - The number of points.
   */
  FUN_BASE_API Box2(const Vector2* points, const int32 count);

  /**
   * Creates and initializes a new box from an array of points.
   *
   * \param points - Array of points to create for the bounding volume.
   */
  FUN_BASE_API Box2(const Array<Vector2>& points);

  /**
   * Compares two boxes for equality.
   *
   * \param other - The other box to compare with.
   *
   * \return true if the boxes are equal, false otherwise.
   */
  bool operator == (const Box2& other) const {
    return min == other.min && max == other.max;
  }

  /**
   * adds to this bounding box to include a given point.
   *
   * \param other - The point to increase the bounding volume to.
   *
   * \return Reference to this bounding box after resizing to include the other point.
   */
  FUN_ALWAYS_INLINE Box2& operator += (const Vector2& other);

  /**
   * Gets the result of addition to this bounding volume.
   *
   * \param other - The other point to add to this.
   *
   * \return A new bounding volume.
   */
  Box2 operator + (const Vector2& other) const {
    return Box2(*this) += other;
  }

  /**
   * adds to this bounding box to include a new bounding volume.
   *
   * \param other - The bounding volume to increase the bounding volume to.
   *
   * \return Reference to this bounding volume after resizing to include the other bounding volume.
   */
  FUN_ALWAYS_INLINE Box2& operator += (const Box2& other);

  /**
   * Gets the result of addition to this bounding volume.
   *
   * \param other - The other volume to add to this.
   *
   * \return A new bounding volume.
   */
  Box2 operator + (const Box2& other) const {
    return Box2(*this) += other;
  }

  /**
   * Gets reference to the min or max of this bounding volume.
   *
   * \param Index - The index into points of the bounding volume.
   *
   * \return A reference to a point of the bounding volume.
   */
  Vector2& operator [] (int32 index) {
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
  FUN_ALWAYS_INLINE float ComputeSquaredDistanceToPoint(const Vector2& point) const {
    // Accumulates the distance as we iterate axis
    float dist_sqr = 0.f;

    if (point.x < min.x) {
      dist_sqr += Math::Square(point.x - min.x);
    } else if (point.x > max.x) {
      dist_sqr += Math::Square(point.x - max.x);
    }

    if (point.y < min.y) {
      dist_sqr += Math::Square(point.y - min.y);
    } else if (point.y > max.y) {
      dist_sqr += Math::Square(point.y - max.y);
    }

    return dist_sqr;
  }

  /**
   * Increase the bounding box volume.
   *
   * \param w - The size to increase volume by.
   *
   * \return A new bounding box increased in size.
   */
  Box2 ExpandBy(const float w) const {
    return Box2(min - Vector2(w), max + Vector2(w));
  }

  /**
   * Gets the box area.
   *
   * \return box area.
   *
   * @see GetCenter, GetCenterAndExtents, GetExtent, GetSize
   */
  float GetArea() const {
    return (max.x - min.x) * (max.y - min.y);
  }

  /**
   * Gets the box's center point.
   *
   * \return Th center point.
   *
   * @see GetArea, GetCenterAndExtents, GetExtent, GetSize
   */
  Vector2 GetCenter() const {
    return (min + max) * 0.5f;
  }

  /**
   * Get the center and extents
   *
   * \param out_center - [out] reference to center point
   * \param out_extents - [out] reference to the extent around the center
   *
   * @see GetArea, GetCenter, GetExtent, GetSize
   */
  void GetCenterAndExtents(Vector2& out_center, Vector2& out_extents) const {
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
  FUN_ALWAYS_INLINE Vector2 GetClosestPointTo(const Vector2& point) const;

  /**
   * Gets the box extents around the center.
   *
   * \return box extents.
   *
   * @see GetArea, GetCenter, GetCenterAndExtents, GetSize
   */
  Vector2 GetExtent() const {
    return (max - min) * 0.5f;
  }

  /**
   * Gets the box size.
   *
   * \return box size.
   *
   * @see GetArea, GetCenter, GetCenterAndExtents, GetExtent
   */
  Vector2 GetSize() const {
    return max - min;
  }

  /**
   * Set the initial values of the bounding box to Zero.
   */
  void Init() {
    min = max = Vector2::ZeroVector;
    is_valid = false;
  }

  /**
   * Checks whether the given box intersects this box.
   *
   * \param other - bounding box to test intersection
   *
   * \return true if boxes intersect, false otherwise.
   */
  FUN_ALWAYS_INLINE bool Intersect(const Box2& other) const;

  /**
   * Checks whether the given point is inside this box.
   *
   * \param point - The point to test.
   *
   * \return true if the point is inside this box, otherwise false.
   */
  bool IsInside(const Vector2& point) const {
    return  point.x > min.x &&
            point.x < max.x &&
            point.y > min.y &&
            point.y < max.y;
  }

  bool IsInsideOn(const Vector2& point) const {
    return  point.x >= min.x &&
            point.x <= max.x &&
            point.y >= min.y &&
            point.y <= max.y;
  }

  /**
   * Checks whether the given box is fully encapsulated by this box.
   *
   * \param other - The box to test for encapsulation within the bounding volume.
   *
   * \return true if box is inside this volume, false otherwise.
   */
  bool IsInside(const Box2& other) const {
    return IsInside(other.min) && IsInside(other.max);
  }

  /**
   * Shift bounding box position.
   *
   * \param offset - The offset vector to shift by.
   *
   * \return A new shifted bounding box.
   */
  Box2 ShiftBy(const Vector2& offset) const {
    return Box2(min + offset, max + offset);
  }

  /**
   * Get a textual representation of this box.
   *
   * \return A string describing the box.
   */
  String ToString() const;

  /**
   * Serializes the bounding box.
   *
   * \param ar - The archive to serialize into.
   * \param box - The box to serialize.
   *
   * \return Reference to the Archive after serialization.
   */
  friend Archive& operator & (Archive& ar, Box2& box) {
    return ar & box.min & box.max & box.is_valid;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE Box2& Box2::operator += (const Vector2& other) {
  if (is_valid) {
    min.x = Math::Min(min.x, other.x);
    min.y = Math::Min(min.y, other.y);

    max.x = Math::Max(max.x, other.x);
    max.y = Math::Max(max.y, other.y);
  } else {
    min = max = other;
    is_valid = true;
  }

  return *this;
}

FUN_ALWAYS_INLINE Box2& Box2::operator += (const Box2& other) {
  if (is_valid && other.is_valid) {
    min.x = Math::Min(min.x, other.min.x);
    min.y = Math::Min(min.y, other.min.y);

    max.x = Math::Max(max.x, other.max.x);
    max.y = Math::Max(max.y, other.max.y);
  } else if (other.is_valid) {
    *this = other;
  }

  return *this;
}

FUN_ALWAYS_INLINE Vector2 Box2::GetClosestPointTo(const Vector2& point) const {
  // Start by considering the point inside the box
  Vector2 closest_point = point;

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

  return closest_point;
}

FUN_ALWAYS_INLINE bool Box2::Intersect(const Box2& other) const {
  if (min.x > other.max.x || other.min.x > max.x) {
    return false;
  }

  if (min.y > other.max.y || other.min.y > max.y) {
    return false;
  }

  return true;
}

FUN_ALWAYS_INLINE String Box2::ToString() const {
  return String::Format("is_valid={0}, min=({1}), max=({2})",
                  is_valid ? "true" : "false", min.ToString(), max.ToString());
}

} // namespace fun
