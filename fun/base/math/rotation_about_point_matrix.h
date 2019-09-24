#pragma once

#include "fun/base/base.h"
#include "fun/base/math/matrix.h"

namespace fun {

/**
 * Rotates about an origin point.
 */
class RotationAboutPointMatrix : public RotationTranslationMatrix {
 public:
  RotationAboutPointMatrix(const Rotator& rot, const Vector& origin);

  static Matrix Make(const Rotator& rot, const Vector& origin);

  /**
   * matrix factory. Return an Matrix so we don't have type conversion issues in expressions.
   */
  static FUN_BASE_API Matrix Make(const Quat& rot, const Vector& origin);
};


FUN_ALWAYS_INLINE RotationAboutPointMatrix::RotationAboutPointMatrix(
      const Rotator& rot, const Vector& origin)
  : RotationTranslationMatrix(rot, origin) {
  // RotationTranslationMatrix generates r * T.
  // We need -T * r * T, so prepend that translation:
  Vector x_axis(m[0][0], m[1][0], m[2][0]);
  Vector y_axis(m[0][1], m[1][1], m[2][1]);
  Vector z_axis(m[0][2], m[1][2], m[2][2]);

  m[3][0] -= x_axis | origin;
  m[3][1] -= y_axis | origin;
  m[3][2] -= z_axis | origin;
}

FUN_ALWAYS_INLINE Matrix RotationAboutPointMatrix::Make(const Rotator& rot, const Vector& origin) {
  return RotationAboutPointMatrix(rot, origin);
}

} // namespace fun
