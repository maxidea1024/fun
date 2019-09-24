#pragma once

#include "fun/base/math/matrix.h"

namespace fun {

class PerspectiveMatrix : public Matrix {
 public:
// Note: the value of this must match the mirror in Common.usf!
#define Z_PRECISION  0.f

  /**
   * Constructor
   *
   * \param half_fov_x - Half FOV in the x axis
   * \param half_fov_y - Half FOV in the y axis
   * \param mult_fov_x - multiplier on the x axis
   * \param mult_fov_y - multiplier on the y axis
   * \param min_z - distance to the near z plane
   * \param max_z - distance to the far z plane
   */
  PerspectiveMatrix(float half_fov_x, float half_fov_y, float mult_fov_x, float mult_fov_y, float min_z, float max_z);

  /**
   * Constructor
   *
   * \param half_fov - half Field of View in the y direction
   * \param width - view space width
   * \param height - view space height
   * \param min_z - distance to the near z plane
   * \param max_z - distance to the far z plane
   *
   * @note that the FOV you pass in is actually half the FOV, unlike most perspective matrix functions (D3DXMatrixPerspectiveFovLH).
   */
  PerspectiveMatrix(float half_fov, float width, float height, float min_z, float max_z);

  /**
   * Constructor
   *
   * \param half_fov - half Field of View in the y direction
   * \param width - view space width
   * \param height - view space height
   * \param min_z - distance to the near z plane
   *
   * @note that the FOV you pass in is actually half the FOV, unlike most perspective matrix functions (D3DXMatrixPerspectiveFovLH).
   */
  PerspectiveMatrix(float half_fov, float width, float height, float min_z);
};


class ReversedZPerspectiveMatrix : public Matrix {
 public:
  ReversedZPerspectiveMatrix(float half_fov_x, float half_fov_y, float mult_fov_x, float mult_fov_y, float min_z, float max_z);
  ReversedZPerspectiveMatrix(float half_fov, float width, float height, float min_z, float max_z);
  ReversedZPerspectiveMatrix(float half_fov, float width, float height, float min_z);
};


#if _MSC_VER
#pragma warning (push)
// Disable possible division by 0 warning
#pragma warning (disable : 4723)
#endif

FUN_ALWAYS_INLINE PerspectiveMatrix::PerspectiveMatrix(float half_fov_x, float half_fov_y, float mult_fov_x, float mult_fov_y, float min_z, float max_z)
  : Matrix(
    Plane(mult_fov_x / Math::Tan(half_fov_x), 0.f,                                0.f,                                                                         0.f),
    Plane(0.f,                                mult_fov_y / Math::Tan(half_fov_y), 0.f,                                                                         0.f),
    Plane(0.f,                                0.f,                                ((min_z == max_z) ? (1.f - Z_PRECISION) : max_z / (max_z - min_z)),          1.f),
    Plane(0.f,                                0.f,                                -min_z * ((min_z == max_z) ? (1.f - Z_PRECISION) : max_z / (max_z - min_z)), 0.f))
{
}

FUN_ALWAYS_INLINE PerspectiveMatrix::PerspectiveMatrix(float half_fov, float width, float height, float min_z, float max_z)
  : Matrix(
    Plane(1.f / Math::Tan(half_fov), 0.f,                                   0.f,                                                                         0.f),
    Plane(0.f,                       width / Math::Tan(half_fov) / height,  0.f,                                                                         0.f),
    Plane(0.f,                       0.f,                                   ((min_z == max_z) ? (1.f - Z_PRECISION) : max_z / (max_z - min_z)),          1.f),
    Plane(0.f,                       0.f,                                   -min_z * ((min_z == max_z) ? (1.f - Z_PRECISION) : max_z / (max_z - min_z)), 0.f))
{
}

FUN_ALWAYS_INLINE PerspectiveMatrix::PerspectiveMatrix(float half_fov, float width, float height, float min_z)
  : Matrix(
    Plane(1.f / Math::Tan(half_fov),  0.f,                                   0.f,                           0.f),
    Plane(0.f,                        width / Math::Tan(half_fov) / height,   0.f,                          0.f),
    Plane(0.f,                        0.f,                                   (1.f - Z_PRECISION),           1.f),
    Plane(0.f,                        0.f,                                   -min_z * (1.f - Z_PRECISION),  0.f))
{
}

FUN_ALWAYS_INLINE ReversedZPerspectiveMatrix::ReversedZPerspectiveMatrix(float half_fov_x, float half_fov_y, float mult_fov_x, float mult_fov_y, float min_z, float max_z)
  : Matrix(
    Plane(mult_fov_x / Math::Tan(half_fov_x), 0.f,                               0.f,                                                           0.f),
    Plane(0.f,                               mult_fov_y / Math::Tan(half_fov_y), 0.f,                                                           0.f),
    Plane(0.f,                               0.f,                                ((min_z == max_z) ? 0.f : min_z / (min_z - max_z)),            1.f),
    Plane(0.f,                               0.f,                                ((min_z == max_z) ? min_z : -max_z * min_z / (min_z - max_z)), 0.f))
{
}

FUN_ALWAYS_INLINE ReversedZPerspectiveMatrix::ReversedZPerspectiveMatrix(float half_fov, float width, float height, float min_z, float max_z)
  : Matrix(
    Plane(1.f / Math::Tan(half_fov),  0.f,                                   0.f,                                                           0.f),
    Plane(0.f,                        width / Math::Tan(half_fov) / height,  0.f,                                                           0.f),
    Plane(0.f,                        0.f,                                   ((min_z == max_z) ? 0.f : min_z / (min_z - max_z)),            1.f),
    Plane(0.f,                        0.f,                                   ((min_z == max_z) ? min_z : -max_z * min_z / (min_z - max_z)), 0.f))
{
}

FUN_ALWAYS_INLINE ReversedZPerspectiveMatrix::ReversedZPerspectiveMatrix(float half_fov, float width, float height, float min_z)
  : Matrix(
      Plane(1.f / Math::Tan(half_fov),  0.f,                                   0.f,   0.f),
      Plane(0.f,                        width / Math::Tan(half_fov) / height,  0.f,   0.f),
      Plane(0.f,                        0.f,                                   0.f,   1.f),
      Plane(0.f,                        0.f,                                   min_z, 0.f))
{
}

#if _MSC_VER
#pragma warning (pop)
#endif

} // namespace fun
