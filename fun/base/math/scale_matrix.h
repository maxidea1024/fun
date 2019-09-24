#pragma once

#include "fun/base/base.h"
#include "fun/base/math/matrix.h"

namespace fun {

/**
 * scale matrix.
 */
class ScaleMatrix : public Matrix {
 public:
  ScaleMatrix(float scale);
  ScaleMatrix(const Vector& scale);

  static Matrix Make(float scale);
  static Matrix Make(const Vector& scale);
};


//
// inlines
//

FUN_ALWAYS_INLINE ScaleMatrix::ScaleMatrix(float scale)
  : Matrix(
    Plane(scale, 0.f,   0.f,   0.f),
    Plane(0.f,   scale, 0.f,   0.f),
    Plane(0.f,   0.f,   scale, 0.f),
    Plane(0.f,   0.f,   0.f,   1.f)
  ) {}

FUN_ALWAYS_INLINE ScaleMatrix::ScaleMatrix(const Vector& scale)
  : Matrix(
    Plane(scale.x, 0.f,     0.f,     0.f),
    Plane(0.f,     scale.y, 0.f,     0.f),
    Plane(0.f,     0.f,     scale.z, 0.f),
    Plane(0.f,     0.f,     0.f,     1.f)
  ) {}

FUN_ALWAYS_INLINE Matrix ScaleMatrix::Make(float scale) {
  return ScaleMatrix(scale);
}

FUN_ALWAYS_INLINE Matrix ScaleMatrix::Make(const Vector& scale) {
  return ScaleMatrix(scale);
}

} // namespace fun
