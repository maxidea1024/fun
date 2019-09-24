#pragma once

#include "fun/base/base.h"
#include "fun/base/math/matrix.h"

namespace fun {

class TranslationMatrix : public Matrix {
 public:
  TranslationMatrix(const Vector& delta);

  static Matrix Make(Vector const& delta);
};


//
// inlines
//

FUN_ALWAYS_INLINE TranslationMatrix::TranslationMatrix(const Vector& delta)
  : Matrix(
      Vector4(1.f, 0.f, 0.f, 0.f),
      Vector4(0.f, 1.f, 0.f, 0.f),
      Vector4(0.f, 0.f, 1.f, 0.f),
      Vector4(delta.x, delta.y, delta.z, 1.f)) {}

FUN_ALWAYS_INLINE Matrix TranslationMatrix::Make(Vector const& delta) {
  return TranslationMatrix(delta);
}

} // namespace fun
