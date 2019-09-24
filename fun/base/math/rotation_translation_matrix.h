#pragma once

#include "fun/base/base.h"
#include "fun/base/math/matrix.h"
#include "fun/base/math/rotator.h"

namespace fun {

/**
 * Combined rotation and translation matrix
 */
class RotationTranslationMatrix : public Matrix {
 public:
  RotationTranslationMatrix(const Rotator& rot, const Vector& origin);

  static Matrix Make(const Rotator& rot, const Vector& origin);
};


//
// inlines
//

FUN_ALWAYS_INLINE RotationTranslationMatrix::RotationTranslationMatrix(
      const Rotator& rot, const Vector& origin) {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS

  const VectorRegister angles_v = MakeVectorRegister(rot.pitch, rot.yaw, rot.roll, 0.f);
  const VectorRegister half_angles_v = VectorMultiply(angles_v, VectorizeConstants::DEG_TO_RAD);

  union { VectorRegister v; float f[4]; } sin_angles, cos_angles;
  VectorSinCos(&sin_angles.v, &cos_angles.v, &half_angles_v);

  const float sp = sin_angles.f[0];
  const float sy = sin_angles.f[1];
  const float sr = sin_angles.f[2];
  const float cp = cos_angles.f[0];
  const float cy = cos_angles.f[1];
  const float cr = cos_angles.f[2];

#else

  float sp, sy, sr;
  float cp, cy, cr;
  Math::SinCos(&sp, &cp, Math::DegreesToRadians(rot.pitch));
  Math::SinCos(&sy, &cy, Math::DegreesToRadians(rot.yaw));
  Math::SinCos(&sr, &cr, Math::DegreesToRadians(rot.roll));

#endif //FUN_PLATFORM_ENABLE_VECTORINTRINSICS

  m[0][0] = cp * cy;
  m[0][1] = cp * sy;
  m[0][2] = sp;
  m[0][3] = 0.f;

  m[1][0] = sr * sp * cy - cr * sy;
  m[1][1] = sr * sp * sy + cr * cy;
  m[1][2] = - sr * cp;
  m[1][3] = 0.f;

  m[2][0] = -(cr * sp * cy + sr * sy);
  m[2][1] = cy * sr - cr * sp * sy;
  m[2][2] = cr * cp;
  m[2][3] = 0.f;

  m[3][0] = origin.x;
  m[3][1] = origin.y;
  m[3][2] = origin.z;
  m[3][3] = 1.f;
}

static Matrix RotationTranslationMatrix::Make(const Rotator& rot, const Vector& origin) {
  return RotationTranslationMatrix(rot, origin);
}

} // namespace fun
