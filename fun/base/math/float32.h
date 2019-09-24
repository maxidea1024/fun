#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * 32 bit float components
 */
class Float32 {
 public:
  union {
    struct {
#if FUN_ARCH_LITTLE_ENDIAN
      uint32 mantissa : 23;
      uint32 exponent : 8;
      uint32 sign     : 1;
#else
      uint32 sign     : 1;
      uint32 exponent : 8;
      uint32 mantissa : 23;
#endif
    } components;

    float float_value;
  };

  FUN_ALWAYS_INLINE Float32(float value = 0.f) : float_value(value) {}
};

} // namespace fun
