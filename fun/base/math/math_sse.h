#pragma once

// Code including this header is responsible for including the correct platform-specific header for SSE intrinsics.

namespace fun {

namespace MathSSE {

  static FUN_ALWAYS_INLINE float InvSqrt(float f) {
    // Performs two passes of Newton-Raphson iteration on the hardware estimate
    //    v^-0.5 = x
    // => x^2 = v^-1
    // => 1/(x^2) = v
    // => f(x) = x^-2 - v
    //    f'(x) = -2x^-3

    //    x1 = x0 - f(x0)/f'(x0)
    // => x1 = x0 + 0.5 * (x0^-2 - vec) * x0^3
    // => x1 = x0 + 0.5 * (x0 - vec * x0^3)
    // => x1 = x0 + x0 * (0.5 - 0.5 * vec * x0^2)
    //
    // This final form has one more operation than the legacy factorization (X1 = 0.5*X0*(3-(y*X0)*X0)
    // but retains better accuracy (namely InvSqrt(1) = 1 exactly).

    const __m128 fOneHalf = _mm_set_ss(0.5f);
    __m128 Y0, X0, X1, X2, FOver2;
    float tmp;

    Y0 = _mm_set_ss(f);
    X0 = _mm_rsqrt_ss(Y0);  // 1/sqrt estimate (12 bits)
    FOver2 = _mm_mul_ss(Y0, fOneHalf);

    // 1st Newton-Raphson iteration
    X1 = _mm_mul_ss(X0, X0);
    X1 = _mm_sub_ss(fOneHalf, _mm_mul_ss(FOver2, X1));
    X1 = _mm_add_ss(X0, _mm_mul_ss(X0, X1));

    // 2nd Newton-Raphson iteration
    X2 = _mm_mul_ss(X1, X1);
    X2 = _mm_sub_ss(fOneHalf, _mm_mul_ss(FOver2, X2));
    X2 = _mm_add_ss(X1, _mm_mul_ss(X1, X2));

    _mm_store_ss(&tmp, X2);
    return tmp;
  }

  static FUN_ALWAYS_INLINE float InvSqrtEst(float f) {
    // Performs one pass of Newton-Raphson iteration on the hardware estimate
    const __m128 fOneHalf = _mm_set_ss(0.5f);
    __m128 Y0, X0, X1, FOver2;
    float tmp;

    Y0 = _mm_set_ss(f);
    X0 = _mm_rsqrt_ss(Y0); // 1/sqrt estimate (12 bits)
    FOver2 = _mm_mul_ss(Y0, fOneHalf);

    // 1st Newton-Raphson iteration
    X1 = _mm_mul_ss(X0, X0);
    X1 = _mm_sub_ss(fOneHalf, _mm_mul_ss(FOver2, X1));
    X1 = _mm_add_ss(X0, _mm_mul_ss(X0, X1));

    _mm_store_ss(&tmp, X1);
    return tmp;
  }

} // namespace MathSSE

} // namespace fun
