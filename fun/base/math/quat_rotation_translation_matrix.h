#pragma once

#include "fun/base/base.h"
#include "fun/base/math/matrix.h"

namespace fun {

/**
 * Rotation and translation matrix using quaternion rotation
 */
class QuatRotationTranslationMatrix : public Matrix {
 public:
  /**
   * Constructor
   *
   * \param q - rotation
   * \param origin - translation to apply
   */
  QuatRotationTranslationMatrix(const Quat& q, const Vector& origin);

  /**
   * matrix factory. Return an Matrix so we don't have type conversion issues in expressions.
   */
  static Matrix Make(const Quat& q, const Vector& origin) {
    return QuatRotationTranslationMatrix(q, origin);
  }
};


/** Rotation matrix using quaternion rotation */
class QuatRotationMatrix : public QuatRotationTranslationMatrix {
 public:
  /**
   * Constructor
   *
   * \param q - Rotation
   */
  QuatRotationMatrix(const Quat& q)
    : QuatRotationTranslationMatrix(q, Vector::ZeroVector) {
  }

  /**
   * matrix factory. Return an Matrix so we don't have type conversion issues in expressions.
   */
  static Matrix Make(const Quat& q) {
    return QuatRotationMatrix(q);
  }
};


FUN_ALWAYS_INLINE QuatRotationTranslationMatrix::QuatRotationTranslationMatrix(
    const Quat& q, const Vector& origin) {
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST) && WITH_EDITORONLY_DATA
  // Make sure Quaternion is normalized
  fun_check(q.IsNormalized());
#endif

  const float x2 = q.x + q.x; const float y2 = q.y + q.y; const float z2 = q.z + q.z;
  const float xx = q.x * x2;  const float xy = q.x * y2;  const float xz = q.x * z2;
  const float yy = q.y * y2;  const float yz = q.y * z2;  const float zz = q.z * z2;
  const float wx = q.w * x2;  const float wy = q.w * y2;  const float wz = q.w * z2;

  m[0][0] = 1.f - (yy + zz); m[1][0] = xy - wz;         m[2][0] = xz + wy;         m[3][0] = origin.x;
  m[0][1] = xy + wz;         m[1][1] = 1.f - (xx + zz); m[2][1] = yz - wx;         m[3][1] = origin.y;
  m[0][2] = xz - wy;         m[1][2] = yz + wx;         m[2][2] = 1.f - (xx + yy); m[3][2] = origin.z;
  m[0][3] = 0.f;             m[1][3] = 0.f;             m[2][3] = 0.f;             m[3][3] = 1.f;
}

} // namespace fun
