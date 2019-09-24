#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * 16 bit float components and conversion
 *
 *
 * IEEE float 16
 * Represented by 10-bit mantissa m, 5-bit exponent E, and 1-bit sign S
 *
 * Specials:
 *
 * E=0, m=0         == 0.0
 * E=0, m!=0        == Denormalized value (m / 2^10) * 2^-14
 * 0<E<31, m=any    == (1 + m / 2^10) * 2^(E-15)
 * E=31, m=0        == Infinity
 * E=31, m!=0       == NaN
 */
class Float16 {
 public:
  union {
    struct {
#if FUN_ARCH_LITTLE_ENDIAN
      uint16 mantissa : 10;
      uint16 exponent : 5;
      uint16 sign : 1;
#else
      uint16 sign : 1;
      uint16 exponent : 5;
      uint16 mantissa : 10;
#endif
    } components;

    uint16 encoded;
  };

  /** Default constructor */
  Float16();

  /** Copy constructor. */
  Float16(const Float16& fp16_value);

  /** Conversion constructor. Convert from fp32 to fp16. */
  Float16(float fp32_value);

  /** Assignment operator. Convert from fp32 to fp16. */
  Float16& operator = (float fp32_value);

  /** Assignment operator. Copy fp16 value. */
  Float16& operator = (const Float16& fp16_value);

  /** Convert from fp16 to fp32. */
  operator float() const;

  /** Convert from fp32 to fp16. */
  void Set(float fp32_value);

  /**
   * Convert from fp32 to fp16 without doing any checks if
   * the Fp32 exponent is too large or too small. This is a
   * faster alternative to Set() when you know the values
   * within the single precision float don't need the checks.
   *
   * \param fp32_value - Single precision float to be set as half precision.
   */
  void SetWithoutBoundsChecks(const float fp32_value);

  /** Convert from Fp16 to Fp32. */
  float GetFloat() const;

  /**
   * Serializes the Float16.
   *
   * \param ar - Reference to the serialization archive.
   * \param v - Reference to the Float16 being serialized.
   *
   * \return Reference to the Archive after serialization.
   */
  friend Archive& operator & (Archive& ar, Float16& v) {
    return ar & v.encoded;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE Float16::Float16() : encoded(0) {}

FUN_ALWAYS_INLINE Float16::Float16(const Float16& fp16_value) {
  encoded = fp16_value.encoded;
}


FUN_ALWAYS_INLINE Float16::Float16(float fp32_value) {
  Set(fp32_value);
}

FUN_ALWAYS_INLINE Float16& Float16::operator = (float fp32_value) {
  Set(fp32_value);
  return *this;
}

FUN_ALWAYS_INLINE Float16& Float16::operator = (const Float16& fp16_value) {
  encoded = fp16_value.encoded;
  return *this;
}

FUN_ALWAYS_INLINE Float16::operator float() const {
  return GetFloat();
}

FUN_ALWAYS_INLINE void Float16::Set(float fp32_value) {
  Float32 fp32(fp32_value);

  // Copy sign-bit
  components.sign = fp32.components.sign;

  // Check for zero, denormal or too small value.
  if (fp32.components.exponent <= 112) { // Too small exponent? (0+127-15)
    // Set to 0.
    components.exponent = 0;
    components.mantissa = 0;
  }
  // Check for INF or NaN, or too high value
  else if (fp32.components.exponent >= 143) { // Too large exponent? (31+127-15)
    // Set to 65504.0 (max value)
    components.exponent = 30;
    components.mantissa = 1023;
  }
  // Handle normal number.
  else {
    components.exponent = int32(fp32.components.exponent) - 127 + 15;
    components.mantissa = uint16(fp32.components.mantissa >> 13);
  }
}

FUN_ALWAYS_INLINE void Float16::SetWithoutBoundsChecks(const float fp32_value) {
  const Float32 fp32(fp32_value);

  // Make absolutely sure that you never pass in a single precision floating
  // point value that may actually need the checks. If you are not 100% sure
  // of that just use Set().

  components.sign = fp32.components.sign;
  components.exponent = int32(fp32.components.exponent) - 127 + 15;
  components.mantissa = uint16(fp32.components.mantissa >> 13);
}

FUN_ALWAYS_INLINE float Float16::GetFloat() const {
  Float32 result;

  result.components.sign = components.sign;
  if (components.exponent == 0) {
    uint32 mantissa = components.mantissa;
    if (mantissa == 0) {
      // Zero.
      result.components.exponent = 0;
      result.components.mantissa = 0;
    } else {
      // Denormal.
      uint32 mantissa_shift = 10 - (uint32)Math::TruncToInt(Math::Log2(mantissa));
      result.components.exponent = 127 - (15 - 1) - mantissa_shift;
      result.components.mantissa = mantissa << (mantissa_shift + 23 - 10);
    }
  } else if (components.exponent == 31) { // 2^5 - 1
    // Infinity or NaN. Set to 65504.0
    result.components.exponent = 142;
    result.components.mantissa = 8380416;
  } else {
    // Normal number.
    result.components.exponent = int32(components.exponent) - 15 + 127; // Stored exponents are biased by half their range.
    result.components.mantissa = uint32(components.mantissa) << 13;
  }

  return result.float_value;
}

} // namespace fun
