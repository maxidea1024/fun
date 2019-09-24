#pragma once

#include "fun/base/math/matrix.h"

namespace fun {

class OrthoMatrix : public Matrix {
 public:
  /**
   * Constructor
   *
   * \param width - view space width
   * \param height - view space height
   * \param z_scale - scale in the z axis
   * \param z_offset - offset in the z axis
   */
  OrthoMatrix(float width, float height, float z_scale, float z_offset);
};


class ReversedZOrthoMatrix : public Matrix {
 public:
  ReversedZOrthoMatrix(float width, float height, float z_scale, float z_offset);
};


//
// inlines
//

FUN_ALWAYS_INLINE OrthoMatrix::OrthoMatrix(float width, float height, float z_scale, float z_offset)
  : Matrix(
      Vector4(width ? (1.f / width) : 1.f,  0.f,                            0.f,                0.f),
      Vector4(0.f,                          height ? (1.f / height) : 1.f,  0.f,                0.f),
      Vector4(0.f,                          0.f,                            z_scale,            0.f),
      Vector4(0.f,                          0.f,                            z_offset * z_scale, 1.f))
{
}

FUN_ALWAYS_INLINE ReversedZOrthoMatrix::ReversedZOrthoMatrix(float width, float height, float z_scale, float z_offset)
  : Matrix(
      Vector4(width ? (1.f / width) : 1.f,  0.f,                            0.f,                      0.f),
      Vector4(0.f,                          height ? (1.f / height) : 1.f,  0.f,                      0.f),
      Vector4(0.f,                          0.f,                            -z_scale,                 0.f),
      Vector4(0.f,                          0.f,                            1.f - z_offset * z_scale, 1.f))
{
}

} // namespace fun
