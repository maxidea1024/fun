#pragma once

#include "fun/base/math/math_base_generic.h"

#if FUN_ENABLE_VECTORINTRINSICS
#include "fun/base/math/math_sse.h"
#endif

namespace fun {

struct WindowsMathBase : public GenericMathBase {
#if FUN_ENABLE_VECTORINTRINSICS
  static FUN_ALWAYS_INLINE int32 TruncToInt(float x) {
    return _mm_cvtt_ss2si(_mm_set_ss(x));
  }

  static FUN_ALWAYS_INLINE float TruncToFloat(float x) {
    return (float)TruncToInt(x); // same as generic implementation, but this will call the faster trunc
  }

  static FUN_ALWAYS_INLINE int32 RoundToInt(float x) {
    // Note: the x2 is to workaround the rounding-to-nearest-even-number issue when the fraction is .5
    return _mm_cvt_ss2si(_mm_set_ss(x + x + 0.5f)) >> 1;
  }

  static FUN_ALWAYS_INLINE float RoundToFloat(float x) {
    return (float)RoundToInt(x);
  }

  static FUN_ALWAYS_INLINE int32 FloorToInt(float x) {
    return _mm_cvt_ss2si(_mm_set_ss(x + x - 0.5f)) >> 1;
  }

  static FUN_ALWAYS_INLINE float FloorToFloat(float x) {
    return (float)FloorToInt(x);
  }

  static FUN_ALWAYS_INLINE int32 CeilToInt(float x) {
    // Note: the x2 is to workaround the rounding-to-nearest-even-number issue when the fraction is .5
    return -(_mm_cvt_ss2si(_mm_set_ss(-0.5f - (x + x))) >> 1);
  }

  static FUN_ALWAYS_INLINE float CeilToFloat(float x) {
    // Note: the x2 is to workaround the rounding-to-nearest-even-number issue when the fraction is .5
    return (float)CeilToInt(x);
  }

  static FUN_ALWAYS_INLINE bool IsNaN(float x) {
    return _isnan(x) != 0;
  }

  static FUN_ALWAYS_INLINE bool IsInfinite(float x) {
    return isinf(x) != 0;
  }

  static FUN_ALWAYS_INLINE bool IsFinite(float x) {
    return _finite(x) != 0;
  }

  static FUN_ALWAYS_INLINE float InvSqrt(float x) {
    return Math_SSE::InvSqrt(x);
  }

  static FUN_ALWAYS_INLINE float InvSqrtEst(float x) {
    return Math_SSE::InvSqrtEst(x);
  }

  #pragma intrinsic(_BitScanReverse)
  static FUN_ALWAYS_INLINE uint32 FloorLog2(uint32 x) {
    // Use BSR to return the log2 of the integer
    DWORD log2;
    if (_BitScanReverse(&log2, x) != 0) {
      return log2;
    }

    return 0;
  }

  static FUN_ALWAYS_INLINE uint32 CountLeadingZeros(uint32 x) {
    // Use BSR to return the log2 of the integer
    DWORD log2;
    if (_BitScanReverse(&log2, x) != 0) {
      return 31 - log2;
    }

    return 32;
  }

  static FUN_ALWAYS_INLINE uint32 CountTrailingZeros(uint32 x) {
    if (x == 0) {
      return 32;
    }
    else {
      uint32 bit_index; // 0-based, where the LSB is 0 and MSB is 31
      _BitScanForward((DWORD*)&bit_index, x); // Scans from LSB to MSB
      return bit_index;
    }
  }

  static FUN_ALWAYS_INLINE uint32 CeilLogTwo(uint32 x) {
    const int32 mask = ((int32)(CountLeadingZeros(x) << 26)) >> 31;
    return (32 - CountLeadingZeros(x - 1)) & (~mask);
  }

  static FUN_ALWAYS_INLINE uint32 RoundUpToPowerOfTwo(uint32 x) {
    return 1 << CeilLogTwo(x);
  }
#endif //FUN_ENABLE_VECTORINTRINSICS
};

typedef WindowsMathBase MathBase;

} // namespace fun
