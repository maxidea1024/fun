#pragma once

#include "fun/base/base.h"
#include "fun/base/serialization/archive.h"
#include "fun/base/math/math_util.h"

namespace fun {

/**
 * Structure for two dimensional vectors with half floating point precision.
 */
class Vector2Half {
 public:
  Float16 x;
  Float16 y;

  FUN_ALWAYS_INLINE Vector2Half() {}
  FUN_ALWAYS_INLINE Vector2Half(const Float16& x, const Float16& y);
  FUN_ALWAYS_INLINE Vector2Half(float x, float y);
  FUN_ALWAYS_INLINE Vector2Half(const Vector2& v2);

  Vector2Half& operator = (const Vector2& v2);
  operator Vector2() const;

  String ToString() const;

  friend Archive& operator & (Archive& ar, Vector2Half& v) {
    return ar & v.x & v.y;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE Vector2Half::Vector2Half(const Float16& x, const Float16& y)
  : x(x), y(y) {}

FUN_ALWAYS_INLINE Vector2Half::Vector2Half(float x, float y)
  : x(x), y(y) {}

FUN_ALWAYS_INLINE Vector2Half::Vector2Half(const Vector2& v2)
  : x(v2.x), y(v2.y) {}

FUN_ALWAYS_INLINE Vector2Half& Vector2Half::operator = (const Vector2& v2) {
  x = Float16(v2.x);
  y = Float16(v2.y);
  return *this;
}

FUN_ALWAYS_INLINE String Vector2Half::ToString() const {
  return String::Format("x=%3.3f y=%3.3f", (float)x, (float)y);
}

FUN_ALWAYS_INLINE Vector2Half::operator Vector2() const {
  return Vector2((float)x, (float)y);
}

} // namespace fun
