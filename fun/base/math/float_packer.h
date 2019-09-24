#pragma once

namespace fun {

DECLARE_LOG_CATEGORY_EXTERN(LogFloatPacker, Info, All);

/**
 * FloatInfo_IEEE32
 */
class FloatInfo_IEEE32 {
 public:
  enum { MantissaBits = 23 };
  enum { ExponentBits = 8 };
  enum { SignShift = 31 };
  enum { ExponentBias = 127 };

  enum { MantissaMask = 0x007fffff };
  enum { ExponentMask = 0x7f800000 };
  enum { SignMask = 0x80000000 };

  typedef float FloatType;
  typedef uint32 PackedType;

  static PackedType ToPackedType(FloatType value) {
    return *(PackedType*)&value;
  }

  static FloatType ToFloatType(PackedType value) {
    return *(FloatType*)&value;
  }
};


/**
 * TFloatPacker
 */
template <uint32 NumExponentBits, uint32 NumMantissaBits, bool bRound, typename FloatInfo = FloatInfo_IEEE32>
class TFloatPacker {
 public:
  enum { NumOutputsBits = NumExponentBits + NumMantissaBits + 1};

  enum { MantissaShift = FloatInfo::MantissaBits - NumMantissaBits };
  enum { ExponentBias = (1 << (NumExponentBits-1)) - 1 };
  enum { SignShift = NumExponentBits + NumMantissaBits };

  enum { MantissaMask = (1 << NumMantissaBits) - 1 };
  enum { ExponentMask = ((1 << NumExponentBits)-1) << NumMantissaBits };
  enum { SignMask =  1 << SignShift };

  enum { MinExponent = -ExponentBias - 1 };
  enum { MaxExponent = ExponentBias };

  typedef typename FloatInfo::PackedType PackedType;
  typedef typename FloatInfo::FloatType FloatType;

  PackedType Encode(FloatType value) const {
    if (value == (FloatType)0.0) {
      return (PackedType)0;
    }

    const PackedType value_packed = FloatInfo::ToPackedType(value);

    // Extract mantissa, exponent, sign.
    PackedType mantissa = value_packed & FloatInfo::MantissaMask;
    int32 exponent = (value_packed & FloatInfo::ExponentMask) >> FloatInfo::MantissaBits;
    const PackedType sign = value_packed >> FloatInfo::SignShift;

    // Subtract IEEE's bias.
    exponent -= FloatInfo::ExponentBias;

    if (bRound) {
      mantissa += (1 << (MantissaShift-1));
      if (mantissa & (1 << FloatInfo::MantissaBits)) {
        mantissa = 0;
        ++exponent;
      }
    }

    // Shift the mantissa to the right
    mantissa >>= MantissaShift;

    //fun_log(LogFloatPacker, Info, "fp: exp: %i (%i, %i)", exponent, (int32)MinExponent, (int32)MaxExponent);
    if (exponent < MinExponent) {
      return (PackedType) 0;
    }
    if (exponent > MaxExponent) {
      exponent = MaxExponent;
    }

    // Add our bias.
    exponent -= MinExponent;

    return (sign << SignShift) | (exponent << NumMantissaBits) | (mantissa);
  }

  FloatType Decode(PackedType Value) const {
    if (Value == (PackedType)0) {
      return (FloatType)0.0;
    }

    // Extract mantissa, exponent, sign.
    PackedType mantissa = Value & MantissaMask;
    int32 exponent = (Value & ExponentMask) >> NumMantissaBits;
    const PackedType sign = Value >> SignShift;

    // Subtract our bias.
    exponent += MinExponent;
    // Add IEEE's bias.
    exponent += FloatInfo::ExponentBias;

    mantissa <<= MantissaShift;

    return FloatInfo::ToFloatType((sign << FloatInfo::SignShift) | (exponent << FloatInfo::MantissaBits) | (mantissa));
  }
#if 0
  PackedType EncodeNoSign(FloatType Value) {
    if (Value == (FloatType)0.0) {
      return (PackedType)0;
    }

    const PackedType value_packed = FloatInfo::ToPackedType(Value);

    // Extract mantissa, exponent, sign.
    PackedType mantissa = value_packed & FloatInfo::MantissaMask;
    int32 exponent = (value_packed & FloatInfo::ExponentMask) >> FloatInfo::MantissaBits;
    //const PackedType sign = value_packed >> FloatInfo::SignShift;

    // Subtract IEEE's bias.
    exponent -= FloatInfo::ExponentBias;

    if (bRound) {
      mantissa += (1 << (MantissaShift-1));
      if (mantissa & (1 << FloatInfo::MantissaBits)) {
        mantissa = 0;
        ++exponent;
      }
    }

    // Shift the mantissa to the right
    mantissa >>= MantissaShift;

    //fun_log(LogFloatPacker, Info, TEXT("fp: exp: %i (%i, %i)"), exponent, (int32)MinExponent, (int32)MaxExponent);
    if (exponent < MinExponent) {
      if (exponent < MinExponent-1) {
        return (PackedType) 0;
      }
      exponent = MinExponent;
    }
    if (exponent > MaxExponent) {
      exponent = MaxExponent;
    }

    // Add our bias.
    exponent -= MinExponent;

    return (exponent << NumMantissaBits) | (mantissa);
  }

  FloatType DecodeNoSign(PackedType Value) {
    if (Value == (PackedType)0) {
      return (FloatType)0.0;
    }

    // Extract mantissa, exponent, sign.
    PackedType mantissa = Value & MantissaMask;
    int32 exponent = (Value & ExponentMask) >> NumMantissaBits;
    //const PackedType sign = Value >> SignShift;

    // Subtract our bias.
    exponent += MinExponent;
    // Add IEEE's bias.
    exponent += FloatInfo::ExponentBias;

    mantissa <<= MantissaShift;

    return FloatInfo::ToFloatType((exponent << FloatInfo::MantissaBits) | (mantissa));
  }
#endif
};

} // namespace fun
