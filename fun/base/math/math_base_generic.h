#pragma once

#include "fun/base/math/math_defs.h"
#include <cmath>

namespace fun {

class FUN_BASE_API GenericMathBase {
 public:
  static constexpr FUN_ALWAYS_INLINE int32 TruncToInt(float x) {
    return (int32)x;
  }

  static constexpr FUN_ALWAYS_INLINE float TruncToFloat(float x) {
    return (float)TruncToInt(x);
  }

  static FUN_ALWAYS_INLINE int32 FloorToInt(float x) {
    return TruncToInt(std::floorf(x));
  }

  static FUN_ALWAYS_INLINE float FloorToFloat(float x) {
    return std::floorf(x);
  }

  static FUN_ALWAYS_INLINE double FloorToDouble(double x) {
    return std::floor(x);
  }

  static FUN_ALWAYS_INLINE int32 RoundToInt(float x) {
    return FloorToInt(x + 0.5f);
  }

  static FUN_ALWAYS_INLINE float RoundToFloat(float x) {
    return FloorToFloat(x + 0.5f);
  }

  static FUN_ALWAYS_INLINE double RoundToDouble(double x) {
    return FloorToDouble(x + 0.5);
  }

  static FUN_ALWAYS_INLINE int32 CeilToInt(float x) {
    return TruncToInt(std::ceilf(x));
  }

  static FUN_ALWAYS_INLINE float CeilToFloat(float x) {
    return std::ceilf(x);
  }

  static FUN_ALWAYS_INLINE double CeilToDouble(double x) {
    return std::ceil(x);
  }

  static FUN_ALWAYS_INLINE float Fractional(float x) {
    return x - TruncToFloat(x);
  }

  static FUN_ALWAYS_INLINE float Frac(float x) {
    return x - FloorToFloat(x);
  }

  static FUN_ALWAYS_INLINE float Modf(const float x, float* int_part) {
    return std::modff(x, int_part);
  }

  static FUN_ALWAYS_INLINE double Modf(const double x, double* int_part) {
    return std::modf(x, int_part);
  }

  // Returns e^x
  static FUN_ALWAYS_INLINE float Exp(float x) {
    return std::expf(x);
  }

  // Returns 2^x
  static FUN_ALWAYS_INLINE float Exp2(float x) {
    return std::powf(2.f, x); // exp2f(x);
  }

  static FUN_ALWAYS_INLINE float Loge(float x) {
    return std::logf(x);
  }

  static FUN_ALWAYS_INLINE float LogX(float base, float x) {
    return Loge(x) / Loge(base);
  }

  static FUN_ALWAYS_INLINE float Log2(float x) {
    // Cached value for fast conversions
    static const float LOG_TO_LOG2 = 1.f / Loge(2.f);
    // Do the platform specific log and convert using the cached value
    return Loge(x) * LOG_TO_LOG2;
  }

  static FUN_ALWAYS_INLINE float Fmod(float x, float y) {
#ifdef FUN_PLATFORM_WINDOWS_FAMILY
    // There's a compiler bug on Windows, where fmodf will
    // start returning NaNs randomly with valid inputs.
    // Until this is resolved, we implement our own version.
    const float int_part = TruncToFloat(x / y);
    const float result = x - y * int_part;
    return result;
#else
    return std::fmodf(x, y);
#endif
  }

  static FUN_ALWAYS_INLINE float Sin(float x) {
    return std::sinf(x);
  }

  static FUN_ALWAYS_INLINE float Asin(float x) {
    return std::asinf((x < -1.f) ? -1.f : ((x < 1.f) ? x : 1.f));
  }

  static FUN_ALWAYS_INLINE float Cos(float x) {
    return std::cosf(x);
  }

  static FUN_ALWAYS_INLINE float Acos(float x) {
    return std::acosf((x < -1.f) ? -1.f : ((x < 1.f) ? x : 1.f));
  }

  static FUN_ALWAYS_INLINE float Tan(float x) {
    return std::tanf(x);
  }

  static FUN_ALWAYS_INLINE float Atan(float x) {
    return std::atanf(x);
  }

  static FUN_BASE_API float Atan2(float y, float x);

  static FUN_ALWAYS_INLINE float Sqrt(float x) {
    return std::sqrtf(x);
  }

  static FUN_ALWAYS_INLINE float Pow(float a, float b) {
    return std::powf(a, b);
  }

  static FUN_ALWAYS_INLINE float InvSqrt(float x) {
    return 1.f / std::sqrtf(x);
  }

  static FUN_ALWAYS_INLINE float InvSqrtEst(float x) {
    return InvSqrt(x);
  }

  static FUN_ALWAYS_INLINE bool IsNaN(float x) {
    return ((*(const uint32*)&x) & 0x7FFFFFFF) > 0x7F800000;
  }

  static FUN_ALWAYS_INLINE bool IsInfinite(float x) {
    return isinf(x) != 0;
  }

  static FUN_ALWAYS_INLINE bool IsFinite(float x) {
    return ((*(const uint32*)&x) & 0x7F800000) != 0x7F800000;
  }

  static FUN_ALWAYS_INLINE bool IsNegativeFloat(const float& x) {
    return ((*(const uint32*)&x) >= (uint32)0x80000000); // Detects sign bit.
  }

  static FUN_ALWAYS_INLINE bool IsNegativeDouble(const double& x) {
    return ((*(const uint64*)&x) >= (uint64)0x8000000000000000); // Detects sign bit.
  }

  //static FUN_ALWAYS_INLINE int32 Rand() { return rand(); }
  //static FUN_ALWAYS_INLINE void RandInit(int32 Seed) { srand(Seed); }
  //static FUN_ALWAYS_INLINE float FRand() { return Rand() / (float)RAND_MAX; }
  //static FUN_BASE_API void SRandInit(int32 Seed);
  //static FUN_BASE_API int32 GetRandSeed();
  //static FUN_BASE_API float SRand();

  static FUN_ALWAYS_INLINE uint32 FloorLog2(uint32 x) {
    uint32 pos = 0;
    if (x >= 1 << 16) { x >>= 16; pos += 16; }
    if (x >= 1 <<  8) { x >>=  8; pos +=  8; }
    if (x >= 1 <<  4) { x >>=  4; pos +=  4; }
    if (x >= 1 <<  2) { x >>=  2; pos +=  2; }
    if (x >= 1 <<  1) {           pos +=  1; }
    return (x == 0) ? 0 : pos;
  }

  static FUN_ALWAYS_INLINE uint32 CountLeadingZeros(uint32 x) {
    return (x != 0) ? (31 - FloorLog2(x)) : 32;
  }

  static FUN_ALWAYS_INLINE uint32 CountTrailingZeros(uint32 x) {
    if (x == 0) {
      return 32;
    }

    uint32 result = 0;
    while ((x & 1) == 0) {
      x >>= 1;
      ++result;
    }
    return result;
  }

  static FUN_ALWAYS_INLINE uint32 CeilLogTwo(uint32 x) {
    const int32 mask = ((int32)(CountLeadingZeros(x) << 26)) >> 31;
    return (32 - CountLeadingZeros(x - 1)) & (~mask);
  }

  static FUN_ALWAYS_INLINE uint32 RoundUpToPowerOfTwo(uint32 x) {
    return 1 << CeilLogTwo(x);
  }

  static FUN_ALWAYS_INLINE uint32 MortonCode2(uint32 x) {
    x &= 0x0000FFFF;
    x = (x ^ (x << 8)) & 0x00FF00FF;
    x = (x ^ (x << 4)) & 0x0F0F0F0F;
    x = (x ^ (x << 2)) & 0x33333333;
    x = (x ^ (x << 1)) & 0x55555555;
    return x;
  }

  static FUN_ALWAYS_INLINE uint32 ReverseMortonCode2(uint32 x) {
    x &= 0x55555555;
    x = (x ^ (x >> 1)) & 0x33333333;
    x = (x ^ (x >> 2)) & 0x0F0F0F0F;
    x = (x ^ (x >> 4)) & 0x00FF00FF;
    x = (x ^ (x >> 8)) & 0x0000FFFF;
    return x;
  }

  static FUN_ALWAYS_INLINE uint32 MortonCode3(uint32 x) {
    x &= 0x000003FF;
    x = (x ^ (x << 16)) & 0xFF0000FF;
    x = (x ^ (x <<  8)) & 0x0300F00F;
    x = (x ^ (x <<  4)) & 0x030C30C3;
    x = (x ^ (x <<  2)) & 0x09249249;
    return x;
  }

  static FUN_ALWAYS_INLINE uint32 ReverseMortonCode3(uint32 x) {
    x &= 0x09249249;
    x = (x ^ (x >>  2)) & 0x030C30C3;
    x = (x ^ (x >>  4)) & 0x0300F00F;
    x = (x ^ (x >>  8)) & 0xFF0000FF;
    x = (x ^ (x >> 16)) & 0x000003FF;
    return x;
  }

  static constexpr FUN_ALWAYS_INLINE float FloatSelect(float comparand, float value_ge_zero, float value_lt_zero) {
    return comparand >= 0.f ? value_ge_zero : value_lt_zero;
  }

  static constexpr FUN_ALWAYS_INLINE double FloatSelect(double comparand, double value_ge_zero, double value_lt_zero) {
    return comparand >= 0.f ? value_ge_zero : value_lt_zero;
  }

  template <typename T>
  static constexpr FUN_ALWAYS_INLINE T Abs(const T x) {
    return (x >= (T)0) ? x : -x;
  }

  template <typename T>
  static constexpr FUN_ALWAYS_INLINE T Sign(const T x) {
    return (x > (T)0) ? (T)1 : ((x < (T)0) ? (T)-1 : (T)0);
  }

  template <typename T>
  static constexpr FUN_ALWAYS_INLINE T Max(const T a, const T b) {
    return (a >= b) ? a : b;
  }

  template <typename T>
  static constexpr FUN_ALWAYS_INLINE T Min(const T a, const T b) {
    return (a <= b) ? a : b;
  }

  /** Returns highest of 3 values */
  template <typename T>
  static FUN_ALWAYS_INLINE T Max(const T a, const T b, const T c) {
    return Max(Max(a, b), c);
  }

  /** Returns highest of 4 values */
  template <typename T>
  static FUN_ALWAYS_INLINE T Max(const T a, const T b, const T c, const T d) {
    return Max(Max(Max(a, b), c), d);
  }

  /** Returns lowest of 3 values */
  template <typename T>
  static FUN_ALWAYS_INLINE T Min(const T a, const T b, const T c) {
    return Min(Min(a, b), c);
  }

  /** Returns lowest of 4 values */
  template <typename T>
  static FUN_ALWAYS_INLINE T Min(const T a, const T b, const T c, const T d) {
    return Min(Min(Min(a, b), c), d);
  }

  /** Multiples value by itself */
  template <typename T>
  static FUN_ALWAYS_INLINE T Square(const T x) {
    return x * x;
  }

  /** Clamps x to be between Min and Max, inclusive */
  template <typename T>
  static FUN_ALWAYS_INLINE T Clamp(const T x, const T min, const T max) {
    return x < min ? min : x < max ? x : max;
  }

  template <typename T>
  static FUN_ALWAYS_INLINE T Clamp01(const T x) {
    return Clamp<T>(x, (T)0, (T)1);
  }

  template <typename T>
  static FUN_ALWAYS_INLINE T Clamp0255(const T x) {
    return Clamp<T>(x, (T)0, (T)255);
  }

  /** Checks if value is within a range, exclusive on max_value) */
  template <typename T>
  static FUN_ALWAYS_INLINE bool IsWithin(const T& value, const T& min_value, const T& max_value) {
    return value >= min_value && value < max_value;
  }

  /** Checks if value is within a range, inclusive on max_value) */
  template <typename T>
  static FUN_ALWAYS_INLINE bool IsWithinInclusive(const T& value, const T& min_value, const T& max_value) {
    return value >= min_value && value <= max_value;
  }

  /**
   * Checks if two floating point numbers are nearly equal.
   *
   * \param a - First number to compare
   * \param b - Second number to compare
   * \param error_tolerance - Maximum allowed difference for considering them as 'nearly equal'
   *
   * \return true if a and b are nearly equal
   */
  static FUN_ALWAYS_INLINE bool IsNearlyEqual(float a, float b, float error_tolerance = SMALL_NUMBER) {
    return Abs<float>(a - b) <= error_tolerance;
  }

  /**
   * Checks if two floating point numbers are nearly equal.
   *
   * \param a - First number to compare
   * \param b - Second number to compare
   * \param error_tolerance - Maximum allowed difference for considering them as 'nearly equal'
   *
   * \return true if a and b are nearly equal
   */
  static FUN_ALWAYS_INLINE bool IsNearlyEqual(double a, double b, double error_tolerance = SMALL_NUMBER) {
    return Abs<double>(a - b) <= error_tolerance;
  }

  /**
   * Checks if a floating point number is nearly zero.
   *
   * \param value - Number to compare
   * \param error_tolerance - Maximum allowed difference for considering value as 'nearly zero'
   *
   * \return true if value is nearly zero
   */
  static FUN_ALWAYS_INLINE bool IsNearlyZero(float value, float error_tolerance = SMALL_NUMBER) {
    return Abs<float>(value) <= error_tolerance;
  }

  /**
   * Checks if a floating point number is nearly zero.
   *
   * \param value - Number to compare
   * \param error_tolerance - Maximum allowed difference for considering value as 'nearly zero'
   *
   * \return true if value is nearly zero
   */
  static FUN_ALWAYS_INLINE bool IsNearlyZero(double value, double error_tolerance = SMALL_NUMBER) {
    return Abs<double>(value) <= error_tolerance;
  }

  /**
  Checks whether a number is a power of two.

  \param value - Number to check

  \return true if value is a power of two
  */
  template <typename T>
  static FUN_ALWAYS_INLINE bool IsPowerOfTwo(T value) {
    return (value & (value - 1)) == (T)0;
  }

  /**
   * Snaps a value to the nearest grid multiple
   */
  static FUN_ALWAYS_INLINE float GridSnap(float location, float grid) {
    if (grid == 0.f) {
      return location;
    } else {
      return FloorToFloat((location + 0.5f * grid) / grid) * grid;
    }
  }

  /**
   * Snaps a value to the nearest grid multiple
   */
  static FUN_ALWAYS_INLINE double GridSnap(double location, double grid) {
    if (grid == 0.0) {
      return location;
    } else {
      return FloorToDouble((location + 0.5 * grid) / grid) * grid;
    }
  }

  /**
   * Divides two integers and rounds up.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE T DivideAndRoundUp(T dividend, T divisor) {
    return (dividend + divisor - 1) / divisor;
  }

  /**
   * Divides two integers and rounds down.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE T DivideAndRoundDown(T dividend, T divisor) {
    return dividend / divisor;
  }

  /**
   * Computes the sine and cosine of a scalar float.
   *
   * \param scalar_sin - Pointer to where the Sin result should be stored
   * \param scalar_cos - Pointer to where the Cos result should be stored
   *
   * \param value - input angles
   */
  static FUN_ALWAYS_INLINE void SinCos(float* scalar_sin, float* scalar_cos, float value) {
    // Map value to y in [-pi, pi], x = 2*pi*quotient + remainder.
    float quotient = (INV_PI * 0.5f) * value;
    if (value >= 0.f) {
      quotient = (float)((int32)(quotient + 0.5f));
    } else {
      quotient = (float)((int32)(quotient - 0.5f));
    }
    float y = value - TWO_PI * quotient;

    // Map y to [-pi/2, pi/2] with sin(y) = sin(value).
    float sign;
    if (y > HALF_PI) {
      y = PI - y;
      sign = -1.f;
    } else if (y < -HALF_PI) {
      y = -PI - y;
      sign = -1.f;
    } else {
      sign = +1.f;
    }

    const float y2 = y * y;

    // 11-degree minimax approximation
    *scalar_sin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.f) * y;

    // 10-degree minimax approximation
    const float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.f;
    *scalar_cos = sign * p;
  }

  // Note:  We use FASTASIN_HALF_PI instead of HALF_PI inside of FastASin(), since it was the value that accompanied the minimax coefficients below.
  // It is important to use exactly the same value in all places inside this function to ensure that FastASin(0.f) == 0.f.
  // For comparison:
  //      HALF_PI          == 1.57079632679f == 0x3fC90FDB
  //      FASTASIN_HALF_PI == 1.5707963050f  == 0x3fC90FDA
#define FASTASIN_HALF_PI  (1.5707963050f)
  /**
   * Computes the ASin of a scalar float.
   *
   * \param value - input angle
   *
   * \return ASin of value
   */
  static FUN_ALWAYS_INLINE float FastAsin(float value) {
    // Clamp input to [-1, 1].
    const bool is_non_negative = (value >= 0.f);
    const float x = Abs(value);
    float omx = 1.f - x;
    if (omx < 0.f) {
      omx = 0.f;
    }

    const float root = Sqrt(omx);
    // 7-degree minimax approximation
    float result = ((((((-0.0012624911f * x + 0.0066700901f) * x - 0.0170881256f) * x + 0.0308918810f) * x - 0.0501743046f) * x + 0.0889789874f) * x - 0.2145988016f) * x + FASTASIN_HALF_PI;
    result *= root; // acos(|x|)
    // acos(x) = PI - acos(-x) when x < 0, asin(x) = PI/2 - acos(x)
    return (is_non_negative ? FASTASIN_HALF_PI - result : result - FASTASIN_HALF_PI);
  }
#undef FASTASIN_HALF_PI

  // Conversion Functions

  /**
   * Converts radians to degrees.
   *
   * \param radians - value in radians.
   *
   * \return value in degrees.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE decltype(auto) RadiansToDegrees(T const& radians) {
    return radians * RADIANS_TO_DEGREES;
  }

  /**
   * Converts degrees to radians.
   *
   * \param degress - value in degrees.
   *
   * \return value in radians.
   */
  template <typename T>
  static FUN_ALWAYS_INLINE decltype(auto) DegreesToRadians(T const& degress) {
    return degress * DEGREES_TO_RADIANS;
  }
};

//TODO 왜 문법 오류가 나지... 차후에 확인해보도록 하자!!
//template <>
//FUN_ALWAYS_INLINE float GenericMathBase::Abs(const float x) {
//  return std::fabsf(x);
//}

} // namespace fun
