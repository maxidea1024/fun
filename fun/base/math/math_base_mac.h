#pragma once

#include "fun/base/math/math_base_generic.h"
#include <xmmintrin.h>
#include "fun/base/math/math_sse.h"

namespace fun {

class FUN_BASE_API MacMathBase : public GenericMathBase {
 public:
  static FUN_ALWAYS_INLINE int32 TruncToInt(float x) {
    return _mm_cvtt_ss2si(_mm_set_ss(x));
  }

  static FUN_ALWAYS_INLINE float TruncToFloat(float x) {
    return (float)TruncToInt(x);
  }

  static FUN_ALWAYS_INLINE int32 RoundToInt(float x) {
    return _mm_cvt_ss2si(_mm_set_ss(x + x + 0.5f)) >> 1;
  }

  static FUN_ALWAYS_INLINE float RoundToFloat(float x) {
    return (float)RoundToInt(x);
  }

  static FUN_ALWAYS_INLINE int32 FloorToInt(float F) {
    return _mm_cvt_ss2si(_mm_set_ss(x + x - 0.5f)) >> 1;
  }

  static FUN_ALWAYS_INLINE float FloorToFloat(float x) {
    return (float)FloorToInt(x);
  }

  static FUN_ALWAYS_INLINE int32 CeilToInt(float x) {
    return -(_mm_cvt_ss2si(_mm_set_ss(-0.5f - (x + x))) >> 1);
  }

  static FUN_ALWAYS_INLINE float CeilToFloat(float x) {
    return (float)CeilToInt(x);
  }

  static FUN_ALWAYS_INLINE bool IsNaN(float x) { return isnan(x) != 0; }
  static FUN_ALWAYS_INLINE bool IsFinite(float x) { return isfinite(x); }

#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  static FUN_ALWAYS_INLINE float InvSqrt(float x) {
    return MathSSE::InvSqrt(x);
  }

  static FUN_ALWAYS_INLINE float InvSqrtEst(float x) {
    return MathSSE::InvSqrtEst(x);
  }
#endif

  static FUN_ALWAYS_INLINE uint32 CountLeadingZeros(uint32 x) {
    return x == 0 ? 32 : __builtin_clz(x);
  }

  static FUN_ALWAYS_INLINE uint32 CountTrailingZeros(uint32 x) {
    return x == 0 ? 32 : __builtin_ctz(x);
  }
};

typedef MacMathBase MathBase;

} // namespace fun
