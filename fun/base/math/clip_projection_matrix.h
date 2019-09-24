#pragma once

#include "fun/base/math/matrix.h"

namespace fun {

/**
 * Realigns the near plane for an existing projection matrix
 * with an arbitrary clip plane
 *
 * from: http://sourceforge.net/mailarchive/message.php?msg_id=000901c26324%242181ea90%24a1e93942%40firefly
 *
 * Updated for the fact that our Plane uses Ax+By+Cz=D.
 */
class ClipProjectionMatrix : public Matrix {
 public:
  /**
   * Constructor
   *
   * \param src_proj_mat - source projection matrix to premultiply with the clip matrix
   * \param plane - clipping plane used to build the clip matrix (assumed to be in camera space)
   */
  ClipProjectionMatrix(const Matrix& src_proj_mat, const Plane& plane);

 private:
  /** return sign of a number */
  FUN_ALWAYS_INLINE float Sign(float a);
};


//
// inlines
//

FUN_ALWAYS_INLINE ClipProjectionMatrix::ClipProjectionMatrix(const Matrix& src_proj_mat, const Plane& plane)
  : Matrix(src_proj_mat) {
  // Calculate the clip-space corner point opposite the clipping plane
  // as (Sign(clipPlane.x), Sign(clipPlane.y), 1, 1) and
  // transform it into camera space by multiplying it
  // by the inverse of the projection matrix
  Plane corner_plane(
      Sign(plane.x) / src_proj_mat.m[0][0],
      Sign(plane.y) / src_proj_mat.m[1][1],
      1.f,
      -(1.f - src_proj_mat.m[2][2]) / src_proj_mat.m[3][2]);

  // Calculate the scaled plane vector
  Plane proj_plane(plane * (1.f / (plane | corner_plane)));

  // use the projected space clip plane in z column
  // Note: (account for our negated w coefficient)
  m[0][2] =  proj_plane.x;
  m[1][2] =  proj_plane.y;
  m[2][2] =  proj_plane.z;
  m[3][2] = -proj_plane.w;
}

FUN_ALWAYS_INLINE float ClipProjectionMatrix::Sign(float a) {
  if (a > 0.f) return +1.f;
  if (a < 0.f) return -1.f;
  return 0.f;
}

} // namespace fun
