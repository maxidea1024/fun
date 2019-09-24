#pragma once

#include "fun/base/base.h"
#include "fun/base/math/matrix.h"

namespace fun {

/**
 * Mirrors a point about an abitrary plane
 */
class MirrorMatrix : public Matrix {
 public:
  /**
   * Constructor. Updated for the fact that our plane uses Ax+By+Cz=D.
   * 
   * \param plane - source plane for mirroring (assumed normalized)
   */
  MirrorMatrix(const Plane& plane);
};


//
// inlines
//

FUN_ALWAYS_INLINE MirrorMatrix::MirrorMatrix(const Plane& plane)
  : Matrix(
      Vector4(-2.f*plane.x*plane.x + 1.f, -2.f*plane.y*plane.x,       -2.f*plane.z*plane.x,       0.f),
      Vector4(-2.f*plane.x*plane.y,       -2.f*plane.y*plane.y + 1.f, -2.f*plane.z*plane.y,       0.f),
      Vector4(-2.f*plane.x*plane.z,       -2.f*plane.y*plane.z,       -2.f*plane.z*plane.z + 1.f, 0.f),
      Vector4( 2.f*plane.x*plane.w,        2.f*plane.y*plane.w,        2.f*plane.z*plane.w,       1.f))
{
}

} // namespace fun
