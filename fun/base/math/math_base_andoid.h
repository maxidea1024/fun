#pragma once

#include "fun/base/math/math_base_generic.h"

namespace fun {

class FUN_BASE_API AndroidMathBase : public GenericMathBase {
 public:
  static FUN_ALWAYS_INLINE uint32 CountLeadingZeros(uint32 x) {
    return x == 0 ? 32 : __builtin_clz(x);
  }

  static FUN_ALWAYS_INLINE uint32 CountTrailingZeros(uint32 x) {
    return x == 0 ? 32 : __builtin_ctz(x);
  }
};

typedef AndroidMathBase MathBase;

} // namespace fun
