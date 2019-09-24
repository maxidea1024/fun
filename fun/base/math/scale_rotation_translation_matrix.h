#pragma once

#include "fun/base/base.h"
#include "fun/base/math/matrix.h"
#include "fun/base/math/rotator.h"

namespace fun {

/**
 * Combined scale rotation and translation matrix.
 */
class ScaleRotationTranslationMatrix : public Matrix {
 public:
  ScaleRotationTranslationMatrix( const Vector& scale,
                                  const Rotator& rot,
                                  const Vector& origin);
};


//
// inlines
//

FUN_ALWAYS_INLINE ScaleRotationTranslationMatrix::ScaleRotationTranslationMatrix(
      const Vector& scale, const Rotator& rot, const Vector& origin) {
  float sp, sy, sr;
  float cp, cy, cr;
  Math::SinCos(&sp, &cp, Math::DegreesToRadians(rot.pitch));
  Math::SinCos(&sy, &cy, Math::DegreesToRadians(rot.yaw));
  Math::SinCos(&sr, &cr, Math::DegreesToRadians(rot.roll));

  m[0][0] = (cp * cy) * scale.x;
  m[0][1] = (cp * sy) * scale.x;
  m[0][2] = (sp) * scale.x;
  m[0][3] = 0.f;

  m[1][0] = (sr * sp * cy - cr * sy) * scale.y;
  m[1][1] = (sr * sp * sy + cr * cy) * scale.y;
  m[1][2] = (-sr * cp) * scale.y;
  m[1][3] = 0.f;

  m[2][0] = (-(cr * sp * cy + sr * sy)) * scale.z;
  m[2][1] = (cy * sr - cr * sp * sy) * scale.z;
  m[2][2] = (cr * cp) * scale.z;
  m[2][3] = 0.f;

  m[3][0] = origin.x;
  m[3][1] = origin.y;
  m[3][2] = origin.z;
  m[3][3] = 1.f;
}

} // namespace fun
