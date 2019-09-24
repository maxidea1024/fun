#pragma once

namespace fun {

//
// Numeric constants
//

#define uint8_MIN   ((uint8)0x00)
#define uint16_MIN  ((uint16)0x0000)
#define uint32_MIN  ((uint32)0x00000000)
#define uint64_MIN  ((uint64)0x0000000000000000)
#define int8_MIN    ((int8) -128)
#define int16_MIN   ((int16)-32768)
#define int32_MIN   ((int32)0x80000000)
#define int64_MIN   ((int64)0x8000000000000000)

#define uint8_MAX   ((uint8)0xFF)
#define uint16_MAX  ((uint16)0xFFFF)
#define uint32_MAX  ((uint32)0xFFFFFFFF)
#define uint64_MAX  ((uint64)0xFFFFFFFFFFFFFFFF)
#define int8_MAX    ((int8)0x7F)
#define int16_MAX   ((int16)0x7FFF)
#define int32_MAX   ((int32)0x7FFFFFFF)
#define int64_MAX   ((int64)0x7FFFFFFFFFFFFFFF)

#define float_MIN   (1.175494351e-38F)          /* min positive value */
#define float_MAX   (3.402823466e+38F)
#define double_MIN  (2.2250738585072014e-308)   /* min positive value */
#define double_MAX  (1.7976931348623158e+308)


//
// Numeric type traits
//

/** Helper class to map a numeric type to its limits */
template <typename NumericType>
struct NumericLimits;

/** Numeric limits for const types */
template <typename NumericType>
struct NumericLimits<const NumericType>
  : public NumericLimits<NumericType> {};

/** Numeric limits for volatile types */
template <typename NumericType>
struct NumericLimits<volatile NumericType>
  : public NumericLimits<NumericType> {};

/** Numeric limits for const volatile types */
template <typename NumericType>
struct NumericLimits<const volatile NumericType>
  : public NumericLimits<NumericType> {};

#define __DECLARE_NUMERIC_INTEGER_MINMAX(Type) \
  template <> struct NumericLimits<Type> { \
    typedef Type NumericType; \
    static NumericType Min() { return Type##_MIN; } \
    static NumericType Max() { return Type##_MAX; } \
    static NumericType Lowest() { return Min(); } \
  };

#define __DECLARE_NUMERIC_FLOAT_MINMAX(Type) \
  template <> struct NumericLimits<Type> { \
    typedef Type NumericType; \
    static NumericType Min() { return Type##_MIN; } \
    static NumericType Max() { return Type##_MAX; } \
    static NumericType Lowest() { return -Max(); } \
  };

__DECLARE_NUMERIC_INTEGER_MINMAX(int8);
__DECLARE_NUMERIC_INTEGER_MINMAX(int16);
__DECLARE_NUMERIC_INTEGER_MINMAX(int32);
__DECLARE_NUMERIC_INTEGER_MINMAX(int64);
__DECLARE_NUMERIC_INTEGER_MINMAX(uint8);
__DECLARE_NUMERIC_INTEGER_MINMAX(uint16);
__DECLARE_NUMERIC_INTEGER_MINMAX(uint32);
__DECLARE_NUMERIC_INTEGER_MINMAX(uint64);

__DECLARE_NUMERIC_FLOAT_MINMAX(float);
__DECLARE_NUMERIC_FLOAT_MINMAX(double);

#undef __DECLARE_NUMERIC_INTEGER_MINMAX
#undef __DECLARE_NUMERIC_FLOAT_MINMAX

} // namespace fun
