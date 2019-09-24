#pragma once

#include "fun/base/base.h"
#include "fun/base/math/vector.h"

namespace fun {

/**
 * Structure for capsules.
 *
 * A capsule consists of two sphere connected by a cylinder.
 */
struct CapsuleShape {
  /** The capsule's center point. */
  Vector center;

  /** The capsule's radius. */
  float radius;

  /** The capsule's orientation in space. */
  Vector orientation;

  /** The capsule's length. */
  float length;

 public:
  /**
   * Default constructor.
   */
  FUN_ALWAYS_INLINE CapsuleShape() {}

  /**
   * Create and inintialize a new instance.
   *
   * \param center The capsule's center point.
   * \param radius The capsule's radius.
   * \param orientation The capsule's orientation in space.
   * \param length The capsule's length.
   */
  FUN_ALWAYS_INLINE CapsuleShape(const Vector& center, float radius, const Vector& orientation, float length)
    : center(center),
      radius(radius),
      orientation(orientation),
      length(length) {}
};

} // namespace fun
