#pragma once

#include "fun/base/math/matrix.h"

namespace fun {

/**
 * Inverse Rotation matrix
 */
class InverseRotationMatrix : public Matrix {
 public:
  InverseRotationMatrix(const Rotator& rot);
};


//
// inlines
//

FUN_ALWAYS_INLINE InverseRotationMatrix::InverseRotationMatrix(const Rotator& rot)
  : Matrix(
      Matrix( // yaw
        Vector4(+Math::Cos(rot.yaw * DEGREES_TO_RADIANS), -Math::Sin(rot.yaw * DEGREES_TO_RADIANS), 0.f, 0.f),
        Vector4(+Math::Sin(rot.yaw * DEGREES_TO_RADIANS), +Math::Cos(rot.yaw * DEGREES_TO_RADIANS), 0.f, 0.f),
        Vector4(0.f, 0.f, 1.f, 0.f),
        Vector4(0.f, 0.f, 0.f, 1.f)) *
      Matrix( // pitch
        Vector4(+Math::Cos(rot.pitch * DEGREES_TO_RADIANS), 0.f, -Math::Sin(rot.pitch * DEGREES_TO_RADIANS), 0.f),
        Vector4(0.f, 1.f, 0.f, 0.f),
        Vector4(+Math::Sin(rot.pitch * DEGREES_TO_RADIANS), 0.f, +Math::Cos(rot.pitch * DEGREES_TO_RADIANS), 0.f),
        Vector4(0.f, 0.f, 0.f, 1.f)) *
      Matrix( // roll
        Vector4(1.f, 0.f, 0.f, 0.f),
        Vector4(0.f, +Math::Cos(rot.roll * DEGREES_TO_RADIANS), +Math::Sin(rot.roll * DEGREES_TO_RADIANS), 0.f),
        Vector4(0.f, -Math::Sin(rot.roll * DEGREES_TO_RADIANS), +Math::Cos(rot.roll * DEGREES_TO_RADIANS), 0.f),
        Vector4(0.f, 0.f, 0.f, 1.f)))
{
}

} // namespace fun
