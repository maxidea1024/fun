#ifndef FUN_NUMERIC_LIMITS_H
#define FUN_NUMERIC_LIMITS_H


#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>
#include <limits.h>                 // C limits.h header
#include <float.h>
#if defined(_CPPLIB_VER)            // Dinkumware.
  #include <ymath.h>
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
  #pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


// Disable Warnings:
//   cast truncates constant value / expression is always false
EA_DISABLE_VC_WARNING(4310 4296)

// FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED
//
// Defined as 0 or 1.
// Indicates whether we need to define our own implementations of inf, nan, snan, denorm floating point constants. 
//
#if !defined(FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED)
  #if (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG) && defined(__FLT_MIN__)) || defined(_CPPLIB_VER) // __FLT_MIN__ detects if it's really GCC/clang and not a mimic. _CPPLIB_VER (Dinkumware) covers VC++, and Microsoft platforms.
    #define FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED 0
  #else
    #define FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED 1
  #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// min/max workaround
//
// MSVC++ has #defines for min/max which collide with the min/max algorithm
// declarations. The following may still not completely resolve some kinds of
// problems with MSVC++ #defines, though it deals with most cases in production
// game code.
//
#if FUN_NOMINMAX
  #ifdef min
    #undef min
  #endif
  #ifdef max
    #undef max
  #endif
#endif


// constexpr
// constexpr is defined in EABase 2.00.38 and later.
#if !defined(constexpr)
  #define constexpr
#endif

// constexpr
// constexpr is defined in EABase 2.00.39 and later.
#if !defined(constexpr)
  #define constexpr const
#endif


///////////////////////////////////////////////////////////////////////////////
// FUN_LIMITS macros
// These apply to integral types only.
///////////////////////////////////////////////////////////////////////////////

// true or false.
#define FUN_LIMITS_IS_SIGNED(T)    ((T)(-1) < 0)

// The min possible value of T. 
#define FUN_LIMITS_MIN_S(T) ((T)((T)1 << FUN_LIMITS_DIGITS_S(T)))
#define FUN_LIMITS_MIN_U(T) ((T)0)
#define FUN_LIMITS_MIN(T)   ((FUN_LIMITS_IS_SIGNED(T) ? FUN_LIMITS_MIN_S(T) : FUN_LIMITS_MIN_U(T)))

// The max possible value of T. 
#define FUN_LIMITS_MAX_S(T) ((T)(((((T)1 << (FUN_LIMITS_DIGITS(T) - 1)) - 1) << 1) + 1))
#define FUN_LIMITS_MAX_U(T) ((T)~(T)0)
#define FUN_LIMITS_MAX(T)   ((FUN_LIMITS_IS_SIGNED(T) ? FUN_LIMITS_MAX_S(T) : FUN_LIMITS_MAX_U(T)))

// The number of bits in the representation of T.
#define FUN_LIMITS_DIGITS_S(T) ((sizeof(T) * CHAR_BIT) - 1)
#define FUN_LIMITS_DIGITS_U(T) ((sizeof(T) * CHAR_BIT))
#define FUN_LIMITS_DIGITS(T)   ((FUN_LIMITS_IS_SIGNED(T) ? FUN_LIMITS_DIGITS_S(T) : FUN_LIMITS_DIGITS_U(T)))

// The number of decimal digits that can be represented by T.
#define FUN_LIMITS_DIGITS10_S(T) ((FUN_LIMITS_DIGITS_S(T) * 643L) / 2136) // (643 / 2136) ~= log10(2).
#define FUN_LIMITS_DIGITS10_U(T) ((FUN_LIMITS_DIGITS_U(T) * 643L) / 2136)
#define FUN_LIMITS_DIGITS10(T)   ((FUN_LIMITS_IS_SIGNED(T) ? FUN_LIMITS_DIGITS10_S(T) : FUN_LIMITS_DIGITS10_U(T)))


namespace fun {
  // See C++11 18.3.2.5
  enum float_round_style {
    round_indeterminate       = -1,    /// Intermediate.
    round_toward_zero         =  0,    /// To zero.
    round_to_nearest          =  1,    /// To the nearest representable value.
    round_toward_infinity     =  2,    /// To infinity.
    round_toward_neg_infinity =  3     /// To negative infinity.
  };

  // See C++11 18.3.2.6
  enum float_denorm_style {
    denorm_indeterminate = -1,          /// It cannot be determined whether or not the type allows denormalized values.
    denorm_absent        =  0,          /// The type does not allow denormalized values.
    denorm_present       =  1           /// The type allows denormalized values.
  };


  namespace internal {
    // Defines default values for NumericLimits, which can be overridden by class specializations.
    // See C++11 18.3.2.3
    struct NumericLimitsBase {
      // true if the type has an explicit specialization defined in the template class; false if not. 
      static constexpr bool is_specialized = false;

      // Integer types: the number of *bits* in the representation of T.
      // Floating types: the number of digits in the mantissa of T (same as FLT_MANT_DIG, DBL_MANT_DIG or LDBL_MANT_DIG).
      static constexpr int digits = 0;

      // The number of decimal digits that can be represented by T.
      // Equivalent to FLT_DIG, DBL_DIG or LDBL_DIG for floating types.
      static constexpr int digits10 = 0;

      // The number of decimal digits required to make sure that two distinct values of the type have distinct decimal representations.
      static constexpr int max_digits10 = 0;

      // True if the type is signed.
      static constexpr bool is_signed = false;

      // True if the type is integral.
      static constexpr bool is_integer = false;

      // True if the type uses an exact representation. All integral types are
      // exact, but other types can be exact as well.
      static constexpr bool is_exact = false;

      // Integer types: the base of the representation. Always 2 for integers.
      // Floating types: the base of the exponent representation. Always FLT_RADIX (typically 2) for float.
      static constexpr int radix = 0;

      // The minimum integral radix-based exponent representable by the type.
      static constexpr int min_exponent = 0;

      // The minimum integral base 10 exponent representable by the type.
      static constexpr int min_exponent10 = 0;

      // The maximum integral radix-based exponent representable by the type.
      static constexpr int max_exponent = 0;

      // The maximum integral base 10 exponent representable by the type.
      static constexpr int max_exponent10 = 0;

      // True if the type has a representation for positive infinity.
      static constexpr bool has_infinity = false;

      //  True if the type has a representation for a quiet (non-signaling) NaN.
      static constexpr bool has_quiet_NaN = false;

      // True if the type has a representation for a signaling NaN.
      static constexpr bool has_signaling_NaN = false;

      // An enumeration which identifies denormalization behavior.
      // In practice the application can change this at runtime via hardware-specific commands.
      static constexpr float_denorm_style has_denorm = denorm_absent;

      // True if the loss of accuracy is detected as a denormalization loss.
      // Typically false for integer types and true for floating point types.
      static constexpr bool has_denorm_loss = false;

      // True if the type has a bounded set of representable values. Typically true for 
      // all built-in numerial types (integer and floating point).
      static constexpr bool is_bounded = false;

      // True if the type has a modulo representation (if it's possible to add two
      // positive numbers and have a result that wraps around to a third number
      // that is less. Typically true for integers and false for floating types.
      static constexpr bool is_modulo = false;

      // True if trapping (arithmetic exception generation) is implemented for this type.
      // Typically true for integer types (div by zero) and false for floating point types,
      // though in practice the application may be able to change floating point to trap at runtime.
      static constexpr bool traps = false;

      // True if tiny-ness is detected before rounding.
      static constexpr bool tinyness_before = false;

      // An enumeration which identifies default rounding behavior.
      // In practice the application can change this at runtime via hardware-specific commands.
      static constexpr float_round_style round_style = round_toward_zero;

      // True if the type is floating point and follows the IEC 559 standard (IEEE 754).
      // In practice the application or OS can change this at runtime via hardware-specific commands or via compiler optimizations.
      static constexpr bool is_iec559 = false;
    };


    #if FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED
      extern FUN_API float gFloatInfinity;
      extern FUN_API float gFloatNaN;
      extern FUN_API float gFloatSNaN;
      extern FUN_API float gFloatDenorm;

      extern FUN_API double gDoubleInfinity;
      extern FUN_API double gDoubleNaN;
      extern FUN_API double gDoubleSNaN;
      extern FUN_API double gDoubleDenorm;

      extern FUN_API long double gLongDoubleInfinity;
      extern FUN_API long double gLongDoubleNaN;
      extern FUN_API long double gLongDoubleSNaN;
      extern FUN_API long double gLongDoubleDenorm;
    #endif

  } // namespace internal


  // Default NumericLimits.
  // See C++11 18.3.2.3
  template <typename T>
  class NumericLimits : public internal::NumericLimitsBase
  {
   public:
    typedef T ValueType;

    static ValueType Min()
      { return ValueType(0); }

    static ValueType Max()
      { return ValueType(0); }

    static ValueType Lowest()
      { return Min(); }

    static ValueType Epsilon()
      { return ValueType(0); }

    static ValueType RoundError()
      { return ValueType(0); }

    static ValueType DenormMin()
      { return ValueType(0); }

    static ValueType Infinity()
      { return ValueType(0); }

    static ValueType QuietNaN()
      { return ValueType(0); }

    static ValueType Signaling_NaN()
      { return ValueType(0); }
  };


  // Const/volatile variations of NumericLimits.
  template <typename T>
  class NumericLimits<const T> : public NumericLimits<T>
  {
  };

  template <typename T>
  class NumericLimits<volatile T> : public NumericLimits<T>
  {
  };

  template <typename T>
  class NumericLimits<const volatile T> : public NumericLimits<T>
  {
  };


  // NumericLimits<bool>
  template <>
  struct NumericLimits<bool> {
    typedef bool ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = 1;      // In practice bool is stores as a byte, or sometimes an int.
    static constexpr int                digits10          = 0;
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = false;  // In practice bool may be implemented as signed char.
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = false;
    static constexpr bool               traps             = true;   // Should this be true or false? Given that it's implemented in hardware as an integer type, we use true.
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return false; }

    static constexpr ValueType Max()
      { return true; }

    static constexpr ValueType Lowest() 
      { return false; }

    static constexpr ValueType Epsilon() 
      { return false; }

    static constexpr ValueType RoundError() 
      { return false; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return ValueType(); }
  };


  // NumericLimits<char>
  template <>
  struct NumericLimits<char> {
    typedef char ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = FUN_LIMITS_IS_SIGNED(ValueType);
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return FUN_LIMITS_MIN(ValueType); }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX(ValueType); }

    static constexpr ValueType Lowest() 
      { return FUN_LIMITS_MIN(ValueType); }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }    // Question: Should we return 0 here or ValueType()?

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return (ValueType)0; }
  };


  // NumericLimits<unsigned char>
  template <>
  struct NumericLimits<unsigned char>
  {
    typedef unsigned char ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_U(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_U(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = false;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return 0; }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_U(ValueType); }

    static constexpr ValueType Lowest() 
      { return 0; }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return (ValueType)0; }
  };


  // NumericLimits<signed char>
  template <>
  struct NumericLimits<signed char>
  {
    typedef signed char ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_S(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_S(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return FUN_LIMITS_MIN_S(ValueType); }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_S(ValueType); }

    static constexpr ValueType Lowest() 
      { return FUN_LIMITS_MIN_S(ValueType); }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return (ValueType)0; }
  };


  // NumericLimits<wchar_t>
  // VC++ has the option of making wchar_t simply be unsigned short. If that's enabled then  
  // the code below could possibly cause compile failures due to redundancy. The best resolution 
  // may be to use __wchar_t here for VC++ instead of wchar_t, as __wchar_t is always a true 
  // unique type under VC++. http://social.msdn.microsoft.com/Forums/en-US/vclanguage/thread/9059330a-7cce-4d0d-a8e0-e1dcb63322bd/
  template <>
  struct NumericLimits<wchar_t>
  {
    typedef wchar_t ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = FUN_LIMITS_IS_SIGNED(ValueType);
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return FUN_LIMITS_MIN(ValueType); }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX(ValueType); }

    static constexpr ValueType Lowest() 
      { return FUN_LIMITS_MIN(ValueType); }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return (ValueType)0; }
  };


  #if EA_CHAR16_NATIVE // If char16_t is a true unique type (as called for by the C++11 Standard)...

    // NumericLimits<char16_t>
    template <>
    struct NumericLimits<char16_t>
    {
      typedef char16_t ValueType;

      static constexpr bool               is_specialized    = true;
      static constexpr int                digits            = FUN_LIMITS_DIGITS(ValueType);
      static constexpr int                digits10          = FUN_LIMITS_DIGITS10(ValueType);
      static constexpr int                max_digits10      = 0;
      static constexpr bool               is_signed         = FUN_LIMITS_IS_SIGNED(ValueType);
      static constexpr bool               is_integer        = true;
      static constexpr bool               is_exact          = true;
      static constexpr int                radix             = 2;
      static constexpr int                min_exponent      = 0;
      static constexpr int                min_exponent10    = 0;
      static constexpr int                max_exponent      = 0;
      static constexpr int                max_exponent10    = 0;
      static constexpr bool               is_bounded        = true;
      static constexpr bool               is_modulo         = true;
      static constexpr bool               traps             = true;
      static constexpr bool               tinyness_before   = false;
      static constexpr float_round_style  round_style       = round_toward_zero;
      static constexpr bool               has_infinity      = false;
      static constexpr bool               has_quiet_NaN     = false;
      static constexpr bool               has_signaling_NaN = false;
      static constexpr float_denorm_style has_denorm        = denorm_absent;
      static constexpr bool               has_denorm_loss   = false;
      static constexpr bool               is_iec559         = false;

      static constexpr ValueType Min() 
        { return FUN_LIMITS_MIN(ValueType); }

      static constexpr ValueType Max()
        { return FUN_LIMITS_MAX(ValueType); }

      static constexpr ValueType Lowest() 
        { return FUN_LIMITS_MIN(ValueType); }

      static constexpr ValueType Epsilon() 
        { return 0; }

      static constexpr ValueType RoundError() 
        { return 0; }

      static constexpr ValueType Infinity() 
        { return ValueType(); }

      static constexpr ValueType QuietNaN() 
        { return ValueType(); }

      static constexpr ValueType Signaling_NaN()
        { return ValueType(); }

      static constexpr ValueType DenormMin() 
        { return (ValueType)0; }
    };

  #endif


  #if EA_CHAR32_NATIVE // If char32_t is a true unique type (as called for by the C++11 Standard)...

    // NumericLimits<char32_t>
    template <>
    struct NumericLimits<char32_t>
    {
      typedef char32_t ValueType;

      static constexpr bool               is_specialized    = true;
      static constexpr int                digits            = FUN_LIMITS_DIGITS(ValueType);
      static constexpr int                digits10          = FUN_LIMITS_DIGITS10(ValueType);
      static constexpr int                max_digits10      = 0;
      static constexpr bool               is_signed         = FUN_LIMITS_IS_SIGNED(ValueType);
      static constexpr bool               is_integer        = true;
      static constexpr bool               is_exact          = true;
      static constexpr int                radix             = 2;
      static constexpr int                min_exponent      = 0;
      static constexpr int                min_exponent10    = 0;
      static constexpr int                max_exponent      = 0;
      static constexpr int                max_exponent10    = 0;
      static constexpr bool               is_bounded        = true;
      static constexpr bool               is_modulo         = true;
      static constexpr bool               traps             = true;
      static constexpr bool               tinyness_before   = false;
      static constexpr float_round_style  round_style       = round_toward_zero;
      static constexpr bool               has_infinity      = false;
      static constexpr bool               has_quiet_NaN     = false;
      static constexpr bool               has_signaling_NaN = false;
      static constexpr float_denorm_style has_denorm        = denorm_absent;
      static constexpr bool               has_denorm_loss   = false;
      static constexpr bool               is_iec559         = false;

      static constexpr ValueType Min() 
        { return FUN_LIMITS_MIN(ValueType); }

      static constexpr ValueType Max()
        { return FUN_LIMITS_MAX(ValueType); }

      static constexpr ValueType Lowest() 
        { return FUN_LIMITS_MIN(ValueType); }

      static constexpr ValueType Epsilon() 
        { return 0; }

      static constexpr ValueType RoundError() 
        { return 0; }

      static constexpr ValueType Infinity() 
        { return ValueType(); }

      static constexpr ValueType QuietNaN() 
        { return ValueType(); }

      static constexpr ValueType Signaling_NaN()
        { return ValueType(); }

      static constexpr ValueType DenormMin() 
        { return (ValueType)0; }
    };

  #endif


  // NumericLimits<unsigned short>
  template <>
  struct NumericLimits<unsigned short>
  {
    typedef unsigned short ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_U(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_U(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = false;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return 0; }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_U(ValueType); }

    static constexpr ValueType Lowest() 
      { return 0; }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };


  // NumericLimits<signed short>
  template <>
  struct NumericLimits<signed short>
  {
    typedef signed short ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_S(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_S(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return FUN_LIMITS_MIN_S(ValueType); }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_S(ValueType); }

    static constexpr ValueType Lowest() 
      { return FUN_LIMITS_MIN_S(ValueType); }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };



  // NumericLimits<unsigned int>
  template <>
  struct NumericLimits<unsigned int>
  {
    typedef unsigned int ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_U(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_U(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = false;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return 0; }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_U(ValueType); }

    static constexpr ValueType Lowest() 
      { return 0; }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };


  // NumericLimits<signed int>
  template <>
  struct NumericLimits<signed int>
  {
    typedef signed int ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_S(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_S(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return INT_MIN; } // It's hard to get FUN_LIMITS_MIN_S to work with all compilers here.

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_S(ValueType); }

    static constexpr ValueType Lowest() 
      { return INT_MIN; }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };


  // NumericLimits<unsigned long>
  template <>
  struct NumericLimits<unsigned long>
  {
    typedef unsigned long ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_U(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_U(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = false;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return 0; }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_U(ValueType); }

    static constexpr ValueType Lowest() 
      { return 0; }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };


  // NumericLimits<signed long>
  template <>
  struct NumericLimits<signed long>
  {
    typedef signed long ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_S(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_S(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return LONG_MIN; }  // It's hard to get FUN_LIMITS_MIN_S to work with all compilers here.

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_S(ValueType); }

    static constexpr ValueType Lowest() 
      { return LONG_MIN; }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };


  // NumericLimits<unsigned long long>
  template <>
  struct NumericLimits<unsigned long long>
  {
    typedef unsigned long long ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_U(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_U(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = false;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return 0; }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_U(ValueType); }

    static constexpr ValueType Lowest() 
      { return 0; }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };


  // NumericLimits<signed long long>
  template <>
  struct NumericLimits<signed long long>
  {
    typedef signed long long ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FUN_LIMITS_DIGITS_S(ValueType);
    static constexpr int                digits10          = FUN_LIMITS_DIGITS10_S(ValueType);
    static constexpr int                max_digits10      = 0;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = true;
    static constexpr bool               is_exact          = true;
    static constexpr int                radix             = 2;
    static constexpr int                min_exponent      = 0;
    static constexpr int                min_exponent10    = 0;
    static constexpr int                max_exponent      = 0;
    static constexpr int                max_exponent10    = 0;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = true;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_toward_zero;
    static constexpr bool               has_infinity      = false;
    static constexpr bool               has_quiet_NaN     = false;
    static constexpr bool               has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm        = denorm_absent;
    static constexpr bool               has_denorm_loss   = false;
    static constexpr bool               is_iec559         = false;

    static constexpr ValueType Min() 
      { return FUN_LIMITS_MIN_S(ValueType); }

    static constexpr ValueType Max()
      { return FUN_LIMITS_MAX_S(ValueType); }

    static constexpr ValueType Lowest() 
      { return FUN_LIMITS_MIN_S(ValueType); }

    static constexpr ValueType Epsilon() 
      { return 0; }

    static constexpr ValueType RoundError() 
      { return 0; }

    static constexpr ValueType Infinity() 
      { return ValueType(); }

    static constexpr ValueType QuietNaN() 
      { return ValueType(); }

    static constexpr ValueType Signaling_NaN()
      { return ValueType(); }

    static constexpr ValueType DenormMin() 
      { return static_cast<ValueType>(0); }
  };


  #if (EA_COMPILER_INTMAX_SIZE >= 16) && (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) // If __int128_t/__uint128_t is supported...
    // NumericLimits<__uint128_t>
    template <>
    struct NumericLimits<__uint128_t>
    {
      typedef __uint128_t ValueType;

      static constexpr bool               is_specialized    = true;
      static constexpr int                digits            = FUN_LIMITS_DIGITS_U(ValueType);
      static constexpr int                digits10          = FUN_LIMITS_DIGITS10_U(ValueType);
      static constexpr int                max_digits10      = 0;
      static constexpr bool               is_signed         = false;
      static constexpr bool               is_integer        = true;
      static constexpr bool               is_exact          = true;
      static constexpr int                radix             = 2;
      static constexpr int                min_exponent      = 0;
      static constexpr int                min_exponent10    = 0;
      static constexpr int                max_exponent      = 0;
      static constexpr int                max_exponent10    = 0;
      static constexpr bool               is_bounded        = true;
      static constexpr bool               is_modulo         = true;
      static constexpr bool               traps             = true;
      static constexpr bool               tinyness_before   = false;
      static constexpr float_round_style  round_style       = round_toward_zero;
      static constexpr bool               has_infinity      = false;
      static constexpr bool               has_quiet_NaN     = false;
      static constexpr bool               has_signaling_NaN = false;
      static constexpr float_denorm_style has_denorm        = denorm_absent;
      static constexpr bool               has_denorm_loss   = false;
      static constexpr bool               is_iec559         = false;

      static constexpr ValueType Min() 
        { return 0; }

      static constexpr ValueType Max()
        { return FUN_LIMITS_MAX_U(ValueType); }

      static constexpr ValueType Lowest() 
        { return 0; }

      static constexpr ValueType Epsilon() 
        { return 0; }

      static constexpr ValueType RoundError() 
        { return 0; }

      static constexpr ValueType Infinity() 
        { return ValueType(); }

      static constexpr ValueType QuietNaN() 
        { return ValueType(); }

      static constexpr ValueType Signaling_NaN()
        { return ValueType(); }

      static constexpr ValueType DenormMin() 
        { return static_cast<ValueType>(0); }
    };


    // NumericLimits<__int128_t>
    template <>
    struct NumericLimits<__int128_t>
    {
      typedef __int128_t ValueType;

      static constexpr bool               is_specialized    = true;
      static constexpr int                digits            = FUN_LIMITS_DIGITS_S(ValueType);
      static constexpr int                digits10          = FUN_LIMITS_DIGITS10_S(ValueType);
      static constexpr int                max_digits10      = 0;
      static constexpr bool               is_signed         = true;
      static constexpr bool               is_integer        = true;
      static constexpr bool               is_exact          = true;
      static constexpr int                radix             = 2;
      static constexpr int                min_exponent      = 0;
      static constexpr int                min_exponent10    = 0;
      static constexpr int                max_exponent      = 0;
      static constexpr int                max_exponent10    = 0;
      static constexpr bool               is_bounded        = true;
      static constexpr bool               is_modulo         = true;
      static constexpr bool               traps             = true;
      static constexpr bool               tinyness_before   = false;
      static constexpr float_round_style  round_style       = round_toward_zero;
      static constexpr bool               has_infinity      = false;
      static constexpr bool               has_quiet_NaN     = false;
      static constexpr bool               has_signaling_NaN = false;
      static constexpr float_denorm_style has_denorm        = denorm_absent;
      static constexpr bool               has_denorm_loss   = false;
      static constexpr bool               is_iec559         = false;

      static constexpr ValueType Min() 
        { return FUN_LIMITS_MIN_S(ValueType); }

      static constexpr ValueType Max()
        { return FUN_LIMITS_MAX_S(ValueType); }

      static constexpr ValueType Lowest() 
        { return FUN_LIMITS_MIN_S(ValueType); }

      static constexpr ValueType Epsilon() 
        { return 0; }

      static constexpr ValueType RoundError() 
        { return 0; }

      static constexpr ValueType Infinity() 
        { return ValueType(); }

      static constexpr ValueType QuietNaN() 
        { return ValueType(); }

      static constexpr ValueType Signaling_NaN()
        { return ValueType(); }

      static constexpr ValueType DenormMin() 
        { return static_cast<ValueType>(0); }
    };
  #endif


  // NumericLimits<float>
  template <>
  struct NumericLimits<float>
  {
    typedef float ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = FLT_MANT_DIG;
    static constexpr int                digits10          = FLT_DIG;
    static constexpr int                max_digits10      = FLT_MANT_DIG;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = false;
    static constexpr bool               is_exact          = false;
    static constexpr int                radix             = FLT_RADIX;
    static constexpr int                min_exponent      = FLT_MIN_EXP;
    static constexpr int                min_exponent10    = FLT_MIN_10_EXP;
    static constexpr int                max_exponent      = FLT_MAX_EXP;
    static constexpr int                max_exponent10    = FLT_MAX_10_EXP;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = false;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_to_nearest;
    static constexpr bool               has_infinity      = true;
    static constexpr bool               has_quiet_NaN     = true;                   // This may be wrong for some platforms.
    static constexpr bool               has_signaling_NaN = true;                   // This may be wrong for some platforms.
    static constexpr float_denorm_style has_denorm        = denorm_present;         // This may be wrong for some platforms.
    static constexpr bool               has_denorm_loss   = false;                  // This may be wrong for some platforms.
    static constexpr bool               is_iec559         = has_infinity && has_quiet_NaN && (has_denorm == denorm_present);

    #if FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED
      static ValueType Min()
        { return FLT_MIN; }

      static ValueType Max()
        { return FLT_MAX; }

      static ValueType Lowest() 
        { return -FLT_MAX; }

      static ValueType Epsilon() 
        { return FLT_EPSILON; }

      static ValueType RoundError() 
        { return 0.5f; }

      static ValueType Infinity() 
        { return internal::gFloatInfinity; }

      static ValueType QuietNaN() 
        { return internal::gFloatNaN; }

      static ValueType Signaling_NaN()
        { return internal::gFloatSNaN; }

      static ValueType DenormMin() 
        { return internal::gFloatDenorm; }

    #elif (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) && defined(__FLT_MIN__)
      static constexpr ValueType Min()
        { return __FLT_MIN__; }

      static constexpr ValueType Max()
        { return __FLT_MAX__; }

      static constexpr ValueType Lowest() 
        { return -__FLT_MAX__; }

      static constexpr ValueType Epsilon() 
        { return __FLT_EPSILON__; }

      static constexpr ValueType RoundError() 
        { return 0.5f; }

      static constexpr ValueType Infinity() 
        { return __builtin_huge_valf(); }

      static constexpr ValueType QuietNaN() 
        { return __builtin_nanf(""); }

      static constexpr ValueType Signaling_NaN()
        { return __builtin_nansf(""); }

      static constexpr ValueType DenormMin() 
        { return __FLT_DENORM_MIN__; }

    #elif defined(_CPPLIB_VER) // If using the Dinkumware Standard library...
      static ValueType Min()
        { return FLT_MIN; }

      static ValueType Max()
        { return FLT_MAX; }

      static ValueType Lowest() 
        { return -FLT_MAX; }

      static ValueType Epsilon() 
        { return FLT_EPSILON; }

      static ValueType RoundError() 
        { return 0.5f; }

      static ValueType Infinity() 
        { return _CSTD _FInf._Float; }

      static ValueType QuietNaN() 
        { return _CSTD _FNan._Float; }

      static ValueType Signaling_NaN()
        { return _CSTD _FSnan._Float; } 

      static ValueType DenormMin() 
        { return _CSTD _FDenorm._Float; }

    #endif
  };


  // NumericLimits<double>
  template <>
  struct NumericLimits<double>
  {
    typedef double ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = DBL_MANT_DIG;
    static constexpr int                digits10          = DBL_DIG;
    static constexpr int                max_digits10      = DBL_MANT_DIG;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = false;
    static constexpr bool               is_exact          = false;
    static constexpr int                radix             = FLT_RADIX;              // FLT_RADIX applies to all floating point types.
    static constexpr int                min_exponent      = DBL_MIN_EXP;
    static constexpr int                min_exponent10    = DBL_MIN_10_EXP;
    static constexpr int                max_exponent      = DBL_MAX_EXP;
    static constexpr int                max_exponent10    = DBL_MAX_10_EXP;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = false;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_to_nearest;
    static constexpr bool               has_infinity      = true;
    static constexpr bool               has_quiet_NaN     = true;                   // This may be wrong for some platforms.
    static constexpr bool               has_signaling_NaN = true;                   // This may be wrong for some platforms.
    static constexpr float_denorm_style has_denorm        = denorm_present;         // This may be wrong for some platforms.
    static constexpr bool               has_denorm_loss   = false;                  // This may be wrong for some platforms.
    static constexpr bool               is_iec559         = has_infinity && has_quiet_NaN && (has_denorm == denorm_present);

    #if FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED
      static ValueType Min()
        { return DBL_MIN; }

      static ValueType Max()
        { return DBL_MAX; }

      static ValueType Lowest() 
        { return -DBL_MAX; }

      static ValueType Epsilon() 
        { return DBL_EPSILON; }

      static ValueType RoundError() 
        { return 0.5f; }

      static ValueType Infinity() 
        { return internal::gDoubleInfinity; }

      static ValueType QuietNaN() 
        { return internal::gDoubleNaN; }

      static ValueType Signaling_NaN()
        { return internal::gDoubleSNaN; }

      static ValueType DenormMin() 
        { return internal::gDoubleDenorm; }

    #elif (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) && defined(__DBL_MIN__)
      static constexpr ValueType Min()
        { return __DBL_MIN__; }

      static constexpr ValueType Max()
        { return __DBL_MAX__; }

      static constexpr ValueType Lowest() 
        { return -__DBL_MAX__; }

      static constexpr ValueType Epsilon() 
        { return __DBL_EPSILON__; }

      static constexpr ValueType RoundError() 
        { return 0.5f; }

      static constexpr ValueType Infinity() 
        { return __builtin_huge_val(); }

      static constexpr ValueType QuietNaN() 
        { return __builtin_nan(""); }

      static constexpr ValueType Signaling_NaN()
        { return __builtin_nans(""); }

      static constexpr ValueType DenormMin() 
        { return __DBL_DENORM_MIN__; }

    #elif defined(_CPPLIB_VER) // If using the Dinkumware Standard library...
      static ValueType Min()
        { return DBL_MIN; }

      static ValueType Max()
        { return DBL_MAX; }

      static ValueType Lowest() 
        { return -DBL_MAX; }

      static ValueType Epsilon() 
        { return DBL_EPSILON; }

      static ValueType RoundError() 
        { return 0.5f; }

      static ValueType Infinity() 
        { return _CSTD _Inf._Double; }

      static ValueType QuietNaN() 
        { return _CSTD _Nan._Double; }

      static ValueType Signaling_NaN()
        { return _CSTD _Snan._Double; } 

      static ValueType DenormMin() 
        { return _CSTD _Denorm._Double; }

    #endif
  };


  // NumericLimits<long double>
  template <>
  struct NumericLimits<long double>
  {
    typedef long double ValueType;

    static constexpr bool               is_specialized    = true;
    static constexpr int                digits            = LDBL_MANT_DIG;
    static constexpr int                digits10          = LDBL_DIG;
    static constexpr int                max_digits10      = LDBL_MANT_DIG;
    static constexpr bool               is_signed         = true;
    static constexpr bool               is_integer        = false;
    static constexpr bool               is_exact          = false;
    static constexpr int                radix             = FLT_RADIX;              // FLT_RADIX applies to all floating point types.
    static constexpr int                min_exponent      = LDBL_MIN_EXP;
    static constexpr int                min_exponent10    = LDBL_MIN_10_EXP;
    static constexpr int                max_exponent      = LDBL_MAX_EXP;
    static constexpr int                max_exponent10    = LDBL_MAX_10_EXP;
    static constexpr bool               is_bounded        = true;
    static constexpr bool               is_modulo         = false;
    static constexpr bool               traps             = true;
    static constexpr bool               tinyness_before   = false;
    static constexpr float_round_style  round_style       = round_to_nearest;
    static constexpr bool               has_infinity      = true;
    static constexpr bool               has_quiet_NaN     = true;                   // This may be wrong for some platforms.
    static constexpr bool               has_signaling_NaN = true;                   // This may be wrong for some platforms.
    static constexpr float_denorm_style has_denorm        = denorm_present;         // This may be wrong for some platforms.
    static constexpr bool               has_denorm_loss   = false;                  // This may be wrong for some platforms.
    static constexpr bool               is_iec559         = has_infinity && has_quiet_NaN && (has_denorm == denorm_present);

    #if FUN_CUSTOM_FLOAT_CONSTANTS_REQUIRED
      static ValueType Min()
        { return LDBL_MIN; }

      static ValueType Max()
        { return LDBL_MAX; }

      static ValueType Lowest() 
        { return -LDBL_MAX; }

      static ValueType Epsilon() 
        { return LDBL_EPSILON; }

      static ValueType RoundError() 
        { return 0.5f; }

      static ValueType Infinity() 
        { return internal::gLongDoubleInfinity; }

      static ValueType QuietNaN() 
        { return internal::gLongDoubleNaN; }

      static ValueType Signaling_NaN()
        { return internal::gLongDoubleSNaN; }

      static ValueType DenormMin() 
        { return internal::gLongDoubleDenorm; }

    #elif (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) && defined(__LDBL_MIN__)
      static constexpr ValueType Min()
        { return __LDBL_MIN__; }

      static constexpr ValueType Max()
        { return __LDBL_MAX__; }

      static constexpr ValueType Lowest() 
        { return -__LDBL_MAX__; }

      static constexpr ValueType Epsilon() 
        { return __LDBL_EPSILON__; }

      static constexpr ValueType RoundError() 
        { return 0.5f; }

      static constexpr ValueType Infinity() 
        { return __builtin_huge_val(); }

      static constexpr ValueType QuietNaN() 
        { return __builtin_nan(""); }

      static constexpr ValueType Signaling_NaN()
        { return __builtin_nans(""); }

      static constexpr ValueType DenormMin() 
        { return __LDBL_DENORM_MIN__; }

    #elif defined(_CPPLIB_VER) // If using the Dinkumware Standard library...
      static ValueType Min()
        { return LDBL_MIN; }

      static ValueType Max()
        { return LDBL_MAX; }

      static ValueType Lowest() 
        { return -LDBL_MAX; }

      static ValueType Epsilon() 
        { return LDBL_EPSILON; }

      static ValueType RoundError() 
        { return 0.5f; }

      static ValueType Infinity() 
        { return _CSTD _LInf._Long_double; }

      static ValueType QuietNaN() 
        { return _CSTD _LNan._Long_double; }

      static ValueType Signaling_NaN()
        { return _CSTD _LSnan._Long_double; } 

      static ValueType DenormMin() 
        { return _CSTD _LDenorm._Long_double; }

    #endif
  };

} // namespace fun


EA_RESTORE_VC_WARNING()

#endif // Header include guard
