#pragma once

namespace fun {

/**
 * N-bit integer. @todo: optimize
 * Data is stored as an array of 32-bit words from the least to the most significant
 * Doesn't handle overflows (not a big issue, we can always use a bigger bit count)
 * Minimum sanity checks.
 * Can convert from int64 and back (by truncating the result, this is mostly for testing)
 */
template <int32 NumBits>
class TBigInt
{
  enum
  {
    /** Word size. */
    BitsPerWord = 32,
    NumWords = NumBits / BitsPerWord
  };

  static_assert(NumBits >= 64, "TBigInt must have at least 64 bits.");

  /** All bits stored as an array of words. */
  uint32 Bits[NumWords];

  /**
  Makes sure both factors are positive integers and stores their original signs

  \param FactorA first factor
  \param SignA sign of the first factor
  \param FactorB second factor
  \param SignB sign of the second pactor
  */
  inline static void MakePositiveFactors(TBigInt<NumBits>& FactorA, int32& SignA, TBigInt<NumBits>& FactorB, int32& SignB)
  {
    SignA = FactorA.Sign();
    SignB = FactorB.Sign();
    if (SignA < 0)
    {
      FactorA.Negate();
    }
    if (SignB < 0)
    {
      FactorB.Negate();
    }
  }

  /**
  Restores a sign of a result based on the sign of two factors that produced the result.

  \param result math opration result
  \param SignA sign of the first factor
  \param SignB sign of the second factor
  */
  inline static void RestoreSign(TBigInt<NumBits>& result, int32 SignA, int32 SignB)
  {
    if ((SignA * SignB) < 0)
    {
      result.Negate();
    }
  }

 public:
  /** Sets this integer to 0. */
  inline void Zero()
  {
    UnsafeMemory::Memset(Bits, 0, sizeof(Bits));
  }

  /**
  Initializes this big int with a 64 bit integer value.

  \param value The value to set.
  */
  inline void Set(int64 value)
  {
    Zero();
    Bits[0] = (value & 0xFFFFFFFF);
    Bits[1] = (value >> BitsPerWord) & 0xFFFFFFFF;
  }

  /** Default constructor. Initializes the number to zero. */
  TBigInt()
  {
    Zero();
  }

  /**
  Constructor. Initializes this big int with a 64 bit integer value.

  \param other The value to set.
  */
  TBigInt(int64 other)
  {
    Set(other);
  }

  /**
  Constructor. Initializes this big int with an array of words.
  */
  explicit TBigInt(const uint32* InBits)
  {
    UnsafeMemory::Memcpy(Bits, InBits, sizeof(Bits));
  }

  /**
  Constructor. Initializes this big int with a string representing a hex value.
  */
  explicit TBigInt(const String& value)
  {
    Parse(value);
  }

  /**
  Shift left by the specified amount of bits. Does not check if BitCount is valid.

  \param BitCount the number of bits to shift.
  */
  inline void ShiftLeftInternal(const int32 BitCount)
  {
    fun_check_dbg(BitCount > 0);

    TBigInt<NumBits> result;
    if (BitCount & (BitsPerWord - 1))
    {
      const int32 LoWordOffset = (BitCount - 1) / BitsPerWord;
      const int32 HiWordOffset = LoWordOffset + 1;
      const int32 LoWordShift  = BitCount - LoWordOffset * BitsPerWord;
      const int32 HiWordShift  = BitsPerWord - LoWordShift;
      result.Bits[NumWords - 1] |= Bits[NumWords - HiWordOffset] << LoWordShift;
      for (int32 word_index = (NumWords - 1) - HiWordOffset; word_index >= 0; --word_index)
      {
        uint32 value = Bits[word_index];
        result.Bits[word_index + LoWordOffset] |= value << LoWordShift;
        result.Bits[word_index + HiWordOffset] |= value >> HiWordShift;
      }
    }
    else
    {
      const int32 ShiftWords = BitCount / BitsPerWord;
      for (int32 word_index = NumWords - 1; word_index >= ShiftWords; --word_index)
      {
        result.Bits[word_index] = Bits[word_index - ShiftWords];
      }
    }
    *this = result;
  }

  /**
  Shift right by the specified amount of bits. Does not check if BitCount is valid.

  \param BitCount - ㅆTe number of bits to shift.
  */
  inline void ShiftRightInternal(const int32 BitCount)
  {
    fun_check_dbg(BitCount > 0);

    TBigInt<NumBits> result;
    if (BitCount & (BitsPerWord - 1))
    {
      const int32 HiWordOffset = (BitCount - 1) / BitsPerWord;
      const int32 LoWordOffset = HiWordOffset + 1;
      const int32 HiWordShift = BitCount - HiWordOffset * BitsPerWord;
      const int32 LoWordShift = BitsPerWord - HiWordShift;
      result.Bits[0] |= Bits[HiWordOffset] >> HiWordShift;
      for (int32 word_index = LoWordOffset; word_index < NumWords; ++word_index)
      {
        uint32 value = Bits[word_index];
        result.Bits[word_index - HiWordOffset] |= value >> HiWordShift;
        result.Bits[word_index - LoWordOffset] |= value << LoWordShift;
      }
    }
    else
    {
      const int32 ShiftWords = BitCount / BitsPerWord;
      for (int32 word_index = NumWords - 1; word_index >= ShiftWords; --word_index)
      {
        result.Bits[word_index - ShiftWords] = Bits[word_index];
      }
    }
    *this = result;
  }

  /**
  Adds two integers.
  */
  inline void Add(const TBigInt<NumBits>& other)
  {
    int64 CarryOver = 0;
    for (int32 word_index = 0; word_index < NumWords; ++word_index)
    {
      int64 WordSum = (int64)other.Bits[word_index] + (int64)Bits[word_index] + CarryOver;
      CarryOver = WordSum >> BitsPerWord;
      WordSum &= 0xFFFFFFFF;
      Bits[word_index] = (uint32)WordSum;
    }
  }

  /**
  Subtracts two integers.
  */
  inline void Subtract(const TBigInt<NumBits>& other)
  {
    TBigInt<NumBits> NegativeOther(other);
    NegativeOther.Negate();
    Add(NegativeOther);
  }

  /**
  Checks if this integer is negative.
  */
  inline bool IsNegative() const
  {
    return !!(Bits[NumWords - 1] & (1U << (BitsPerWord - 1)));
  }

  /**
  Returns the sign of this integer.
  */
  inline int32 Sign() const
  {
    return IsNegative() ? -1 : 1;
  }

  /**
  Negates this integer. value = -value
  */
  void Negate()
  {
    static const TBigInt<NumBits> One(1LL);
    for (int32 word_index = 0; word_index < NumWords; ++word_index)
    {
      Bits[word_index] = ~Bits[word_index];
    }
    Add(One);
  }


  /**
  Multiplies two integers.
  */
  void Multiply(const TBigInt<NumBits>& Factor)
  {
    TBigInt<NumBits> result;
    TBigInt<NumBits> temp = *this;
    TBigInt<NumBits> other = Factor;

    int32 ThisSign;
    int32 OtherSign;
    MakePositiveFactors(temp, ThisSign, other, OtherSign);

    int32 ShiftCount = 0;
    for (int32 word_index = 0; word_index < NumWords; ++word_index)
    {
      if (other.Bits[word_index])
      {
        for (int32 BitIndex = 0; BitIndex < BitsPerWord; ++BitIndex)
        {
          if (!!(other.Bits[word_index] & (1 << BitIndex)))
          {
            if (ShiftCount)
            {
              temp.ShiftLeftInternal(ShiftCount);
              ShiftCount = 0;
            }
            result += temp;
          }
          ShiftCount++;
        }
      }
      else
      {
        ShiftCount += BitsPerWord;
      }
    }
    // Restore the sign if necessary
    RestoreSign(result, OtherSign, ThisSign);
    *this = result;
  }

  /**
  Divides two integers with remainder.
  */
  void DivideWithRemainder(const TBigInt<NumBits>& divisor, TBigInt<NumBits>& Remainder)
  {
    static const TBigInt<NumBits> One(1LL);
    TBigInt<NumBits> Denominator(divisor);
    TBigInt<NumBits> dividend(*this);

    int32 DenominatorSign;
    int32 DividendSign;
    MakePositiveFactors(Denominator, DenominatorSign, dividend, DividendSign);

    if (Denominator > dividend)
    {
      Remainder = *this;
      Zero();
      return;
    }
    if (Denominator == dividend)
    {
      Remainder.Zero();
      *this = One;
      RestoreSign(*this, DenominatorSign, DividendSign);
      return;
    }

    TBigInt<NumBits> current(One);
    TBigInt<NumBits> Quotient;

    while (Denominator <= dividend)
    {
      Denominator.ShiftLeftInternal(1);
      current.ShiftLeftInternal(1);
    }
    Denominator.ShiftRightInternal(1);
    current.ShiftRightInternal(1);

    while (!current.IsZero())
    {
      if (dividend >= Denominator)
      {
        dividend -= Denominator;
        Quotient |= current;
      }
      current.ShiftRightInternal(1);
      Denominator.ShiftRightInternal(1);
    }
    RestoreSign(Quotient, DenominatorSign, DividendSign);
    Remainder = dividend;
    *this = Quotient;
  }

  /**
  Divides two integers.
  */
  void Divide(const TBigInt<NumBits>& divisor)
  {
    TBigInt<NumBits> Remainder;
    DivideWithRemainder(divisor, Remainder);
  }

  /**
  Performs modulo operation on this integer.
  */
  void Modulo(const TBigInt<NumBits>& Modulus)
  {
    // a mod b = a - floor(a/b)*b
    fun_check(!IsNegative());
    TBigInt<NumBits> dividend(*this);
    dividend.Divide(Modulus);
    dividend.Multiply(Modulus);
    Subtract(dividend);
  }

  /**
  Calculates square root of this integer.
  */
  void Sqrt()
  {
    TBigInt<NumBits> Number(*this);
    TBigInt<NumBits> result;

    TBigInt<NumBits> Bit(1);
    Bit.ShiftLeftInternal(NumBits - 2);
    while (Bit > Number)
    {
      Bit.ShiftRightInternal(2);
    }

    TBigInt<NumBits> temp;
    while (!Bit.IsZero())
    {
      temp = result;
      temp.Add(Bit);
      if (Number >= temp)
      {
        Number -= temp;
        result.ShiftRightInternal(1);
        result += Bit;
      }
      else
      {
        result.ShiftRightInternal(1);
      }
      Bit.ShiftRightInternal(2);
    }
    *this = result;
  }

  /**
  Assignment operator for int64 values.
  */
  inline TBigInt& operator = (int64 other)
  {
    Set(other);
    return *this;
  }

  /**
  Returns the index of the highest word that is not zero. -1 if no such word exists.
  */
  inline int32 GetHighestNonZeroWord() const
  {
    int32 word_index;
    for (word_index = NumWords - 1; word_index >= 0 && Bits[word_index] == 0; --word_index);
    return word_index;
  }

  /**
  Returns the index of the highest non-zero bit. -1 if no such bit exists.
  */
  inline int32 GetHighestNonZeroBit() const
  {
    for (int32 word_index = NumWords - 1; word_index >= 0; --word_index)
    {
      if (!!Bits[word_index])
      {
        int32 BitIndex;
        for (BitIndex = BitsPerWord - 1; BitIndex >= 0 && !(Bits[word_index] & (1 << BitIndex)); --BitIndex);
        return BitIndex + word_index * BitsPerWord;
      }
    }
    return -1;
  }

  /**
  Returns a bit value as an integer value (0 or 1).
  */
  inline int32 GetBit(int32 BitIndex) const
  {
    const int32 word_index = BitIndex / BitsPerWord;
    BitIndex -= word_index * BitsPerWord;
    return (Bits[word_index] & (1 << BitIndex)) ? 1 : 0;
  }

  /**
  Sets a bit value.
  */
  inline void SetBit(int32 BitIndex, int32 value)
  {
    const int32 word_index = BitIndex / BitsPerWord;
    BitIndex -= word_index * BitsPerWord;
    if (!!value)
    {
      Bits[word_index] |= (1 << BitIndex);
    }
    else
    {
      Bits[word_index] &= ~(1 << BitIndex);
    }
  }

  /**
  Shift left by the specified amount of bits.

  \param BitCount the number of bits to shift.
  */
  void ShiftLeft(const int32 BitCount)
  {
    // Early out in the trivial cases
    if (BitCount == 0)
    {
      return;
    }
    else if (BitCount < 0)
    {
      ShiftRight(-BitCount);
      return;
    }
    else if (BitCount >= NumBits)
    {
      Zero();
      return;
    }
    ShiftLeftInternal(BitCount);
  }

  /**
  Shift right by the specified amount of bits.

  \param BitCount the number of bits to shift.
  */
  void ShiftRight(const int32 BitCount)
  {
    // Early out in the trivial cases
    if (BitCount == 0)
    {
      return;
    }
    else if (BitCount < 0)
    {
      ShiftLeft(-BitCount);
      return;
    }
    else if (BitCount >= NumBits)
    {
      Zero();
      return;
    }
    ShiftRightInternal(BitCount);
  }

  /**
  Bitwise 'or'
  */
  inline void BitwiseOr(const TBigInt<NumBits>& other)
  {
    for (int32 word_index = 0; word_index < NumWords; ++word_index)
    {
      Bits[word_index] |= other.Bits[word_index];
    }
  }

  /**
  Bitwise 'and'
  */
  inline void BitwiseAnd(const TBigInt<NumBits>& other)
  {
    for (int32 word_index = 0; word_index < NumWords; ++word_index)
    {
      Bits[word_index] &= other.Bits[word_index];
    }
  }

  /**
  Bitwise 'not'
  */
  inline void BitwiseNot()
  {
    for (int32 word_index = 0; word_index < NumWords; ++word_index)
    {
      Bits[word_index] = ~Bits[word_index];
    }
  }

  /**
  Checks if two integers are equal.
  */
  inline bool IsEqual(const TBigInt<NumBits>& other) const
  {
    for (int32 word_index = 0; word_index < NumWords; ++word_index)
    {
      if (Bits[word_index] != other.Bits[word_index])
      {
        return false;
      }
    }

    return true;
  }

  /**
  this < other
  */
  bool IsLess(const TBigInt<NumBits>& other) const
  {
    if (IsNegative())
    {
      if (!other.IsNegative())
      {
        return true;
      }
      else
      {
        return IsGreater(other);
      }
    }
    int32 word_index;
    for (word_index = NumWords - 1; word_index >= 0 && other.Bits[word_index] == Bits[word_index]; --word_index);
    return word_index >= 0 && Bits[word_index] < other.Bits[word_index];
  }

  /**
  this <= other
  */
  bool IsLessOrEqual(const TBigInt<NumBits>& other) const
  {
    if (IsNegative())
    {
      if (!other.IsNegative())
      {
        return true;
      }
      else
      {
        return IsGreaterOrEqual(other);
      }
    }
    int32 word_index;
    for (word_index = NumWords - 1; word_index >= 0 && other.Bits[word_index] == Bits[word_index]; --word_index);
    return word_index < 0 || Bits[word_index] < other.Bits[word_index];
  }

  /**
  this > other
  */
  bool IsGreater(const TBigInt<NumBits>& other) const
  {
    if (IsNegative())
    {
      if (!other.IsNegative())
      {
        return false;
      }
      else
      {
        return IsLess(other);
      }
    }
    int32 word_index;
    for (word_index = NumWords - 1; word_index >= 0 && other.Bits[word_index] == Bits[word_index]; --word_index);
    return word_index >= 0 && Bits[word_index] > other.Bits[word_index];
  }

  /**
  this >= other
  */
  bool IsGreaterOrEqual(const TBigInt<NumBits>& other) const
  {
    if (IsNegative())
    {
      if (other.IsNegative())
      {
        return false;
      }
      else
      {
        return IsLessOrEqual(other);
      }
    }
    int32 word_index;
    for (word_index = NumWords - 1; word_index >= 0 && other.Bits[word_index] == Bits[word_index]; --word_index);
    return word_index < 0 || Bits[word_index] > other.Bits[word_index];
  }

  /**
  this == 0
  */
  inline bool IsZero() const
  {
    int32 word_index;
    for (word_index = NumWords - 1; word_index >= 0 && !Bits[word_index]; --word_index);
    return word_index < 0;
  }

  /**
  this > 0
  */
  inline bool IsGreaterThanZero() const
  {
    return !IsNegative() && !IsZero();
  }

  /**
  this < 0
  */
  inline bool IsLessThanZero() const
  {
    return IsNegative() && !IsZero();
  }

  /**
  Bit indexing operator
  */
  inline bool operator [] (int32 BitIndex) const
  {
    return GetBit(BitIndex);
  }

  // Begin operator overloads
  inline TBigInt<NumBits> operator >> (int32 Count) const
  {
    TBigInt<NumBits> result = *this;
    result.ShiftRight(Count);
    return result;
  }

  inline TBigInt<NumBits>& operator >>= (int32 Count)
  {
    ShiftRight(Count);
    return *this;
  }

  inline TBigInt<NumBits> operator << (int32 Count) const
  {
    TBigInt<NumBits> result = *this;
    result.ShiftLeft(Count);
    return result;
  }

  inline TBigInt<NumBits>& operator <<= (int32 Count)
  {
    ShiftLeft(Count);
    return *this;
  }

  inline TBigInt<NumBits> operator + (const TBigInt<NumBits>& other) const
  {
    TBigInt<NumBits> result(*this);
    result.Add(other);
    return result;
  }

  inline TBigInt<NumBits>& operator ++ ()
  {
    static const TBigInt<NumBits> One(1);
    Add(One);
    return *this;
  }

  inline TBigInt<NumBits>& operator += (const TBigInt<NumBits>& other)
  {
    Add(other);
    return *this;
  }

  inline TBigInt<NumBits> operator - (const TBigInt<NumBits>& other) const
  {
    TBigInt<NumBits> result(*this);
    result.Subtract(other);
    return result;
  }

  inline TBigInt<NumBits>& operator --()
  {
    static const TBigInt<NumBits> One(1);
    Subtract(One);
    return *this;
  }

  inline TBigInt<NumBits>& operator -= (const TBigInt<NumBits>& other)
  {
    Subtract(other);
    return *this;
  }

  inline TBigInt<NumBits> operator * (const TBigInt<NumBits>& other) const
  {
    TBigInt<NumBits> result(*this);
    result.Multiply(other);
    return result;
  }

  inline TBigInt<NumBits>& operator *= (const TBigInt<NumBits>& other)
  {
    Multiply(other);
    return *this;
  }

  inline TBigInt<NumBits> operator / (const TBigInt<NumBits>& Divider) const
  {
    TBigInt<NumBits> result(*this);
    result.Divide(Divider);
    return result;
  }

  inline TBigInt<NumBits>& operator /= (const TBigInt<NumBits>& Divider)
  {
    Divide(Divider);
    return *this;
  }

  inline TBigInt<NumBits> operator % (const TBigInt<NumBits>& Modulus) const
  {
    TBigInt<NumBits> result(*this);
    result.Modulo(Modulus);
    return result;
  }

  inline TBigInt<NumBits>& operator %= (const TBigInt<NumBits>& Modulus)
  {
    Modulo(Modulus);
    return *this;
  }

  inline bool operator < (const TBigInt<NumBits>& other) const
  {
    return IsLess(other);
  }

  inline bool operator <= (const TBigInt<NumBits>& other) const
  {
    return IsLessOrEqual(other);
  }

  inline bool operator > (const TBigInt<NumBits>& other) const
  {
    return IsGreater(other);
  }

  inline bool operator >= (const TBigInt<NumBits>& other) const
  {
    return IsGreaterOrEqual(other);
  }

  inline bool operator == (const TBigInt<NumBits>& other) const
  {
    return IsEqual(other);
  }

  inline bool operator != (const TBigInt<NumBits>& other) const
  {
    return !IsEqual(other);
  }

  inline TBigInt<NumBits> operator & (const TBigInt<NumBits>& other) const
  {
    TBigInt<NumBits> result(*this);
    result.BitwiseAnd(other);
    return result;
  }

  inline TBigInt<NumBits>& operator &= (const TBigInt<NumBits>& other)
  {
    BitwiseAnd(other);
    return *this;
  }

  inline TBigInt<NumBits> operator | (const TBigInt<NumBits>& other) const
  {
    TBigInt<NumBits> result(*this);
    result.BitwiseOr(other);
    return result;
  }

  inline TBigInt<NumBits>& operator |= (const TBigInt<NumBits>& other)
  {
    BitwiseOr(other);
    return *this;
  }

  inline TBigInt<NumBits> operator ~ () const
  {
    TBigInt<NumBits> result(*this);
    result.BitwiseNot();
    return result;
  }
  // end operator overloads

  /**
  Returns the value of this big int as a 64-bit integer. If the value is greater, the higher bits are truncated.
  */
  int64 ToInt() const
  {
    int64 result;
    if (!IsNegative())
    {
      result = (int64)Bits[0] + ((int64)Bits[1] << BitsPerWord);
    }
    else
    {
      TBigInt<NumBits> Positive(*this);
      Positive.Negate();
      result = (int64)Positive.Bits[0] + ((int64)Positive.Bits[1] << BitsPerWord);
    }
    return (int64)Bits[0] + ((int64)Bits[1] << BitsPerWord);
  }

  /**
  Returns this big int as a string.
  */
  String ToString() const
  {
    String Text(TEXT("0x"));
    int32 word_index;
    for (word_index = NumWords - 1; word_index > 0; --word_index)
    {
      if (Bits[word_index])
      {
        break;
      }
    }
    for (; word_index >= 0; --word_index) {
      Text += String::Format(TEXT("%08x"), Bits[word_index]);
    }
    return Text;
  }

  /**
  Parses a string representing a hex value
  */
  void Parse(const String& value)
  {
    fun_check(value.Len() <= NumBits / 8);
    Zero();
    int32 ValueStartIndex = 0;
    if (value.Len() >= 2 && value[0] == '0' && CCharTraits::ToUpper(value[1]) == 'x')
    {
      ValueStartIndex = 2;
    }
    const int32 BytesPerWord = BitsPerWord / 8;
    for (int32 CharIndex = value.Len() - 1; CharIndex >= ValueStartIndex; --CharIndex)
    {
      const char ByteChar = value[CharIndex];
      const int32 ByteIndex = (value.Len() - CharIndex - 1);
      const int32 word_index = ByteIndex / BytesPerWord;
      const int32 ByteValue = ByteChar > '9' ? (CCharTraits::ToUpper(ByteChar) - 'A' + 10) : (ByteChar - '0');
      fun_check(ByteValue >= 0 && ByteValue <= 15);
      Bits[word_index] |= (ByteValue << (ByteIndex - word_index * 4) * 4);
    }
  }

  /**
  Serialization.
  */
  friend Archive& operator & (Archive& ar, TBigInt<NumBits>& value)
  {
    for (int32 Index = 0; Index < NumWords; ++Index)
    {
      ar & value.Bits[Index];
    }
    return ar;
  }
};

// Predefined big int types
typedef TBigInt<256> int256;
typedef TBigInt<512> int512;


/**
 * Encyption key - exponent and modulus pair
 */
template <typename IntType>
struct TEncryptionKey
{
  IntType Exponent;
  IntType Modulus;
};
typedef TEncryptionKey<int256> CEncryptionKey;


/**
 * Math utils for encryption.
 */
struct CEncryption
{
  /**
  Greatest common divisor of ValueA and ValueB.
  */
  template <typename IntType>
  static IntType CalculateGCD(IntType ValueA, IntType ValueB)
  {
    // Early out in obvious cases
    if (ValueA == 0)
    {
      return ValueA;
    }
    if (ValueB == 0)
    {
      return ValueB;
    }

    // Shift is log(n) where n is the greatest power of 2 dividing both A and B.
    int32 Shift;
    for (Shift = 0; ((ValueA | ValueB) & 1) == 0; ++Shift)
    {
      ValueA >>= 1;
      ValueB >>= 1;
    }

    while ((ValueA & 1) == 0)
    {
      ValueA >>= 1;
    }

    do
    {
      // Remove all factors of 2 in B.
      while ((ValueB & 1) == 0)
      {
        ValueB >>= 1;
      }

      // Make sure A <= B
      if (ValueA > ValueB)
      {
        Swap(ValueA, ValueB);
      }
      ValueB = ValueB - ValueA;
    }
    while (ValueB != 0);

    // Restore common factors of 2.
    return ValueA << Shift;
  }

  /**
  Multiplicative inverse of exponent using extended GCD algorithm.

  Extended gcd: ax + by = gcd(a, b), where a = exponent, b = fi(n), gcd(a, b) = gcd(e, fi(n)) = 1, fi(n) is the euler's totient function of n
  We only care to find d = x, which is our multiplicatve inverse of e (a).
  */
  template <typename IntType>
  static IntType CalculateMultiplicativeInverseOfExponent(IntType Exponent, IntType Totient)
  {
    IntType x0 = 1;
    IntType y0 = 0;
    IntType x1 = 0;
    IntType y1 = 1;
    IntType a0 = Exponent;
    IntType b0 = Totient;
    while (b0 != 0)
    {
      // Quotient = Exponent / Totient
      IntType Quotient = a0 / b0;

      // (Exponent, Totient) = (Totient, Exponent mod Totient)
      IntType b1 = a0 % b0;
      a0 = b0;
      b0 = b1;

      // (x, lastx) = (lastx - Quotient*x, x)
      IntType x2 = x0 - Quotient * x1;
      x0 = x1;
      x1 = x2;

      // (y, lasty) = (lasty - Quotient*y, y)
      IntType y2 = y0 - Quotient * y1;
      y0 = y1;
      y1 = y2;
    }
    // If x0 is negative, find the corresponding positive element in (mod Totient)
    while (x0 < 0)
    {
      x0 += Totient;
    }
    return x0;
  }

  /**
  Generate Key Pair for encryption and decryption.
  */
  template <typename IntType>
  static void GenerateKeyPair(const IntType& P, const IntType& q, CEncryptionKey& PublicKey, CEncryptionKey& PrivateKey)
  {
    const IntType Modulus = P * q;
    const IntType Fi = (P - 1) * (q - 1);

    IntType EncodeExponent = Fi;
    IntType DecodeExponent = 0;
    do
    {
      for (--EncodeExponent; EncodeExponent > 1 && CalculateGCD(EncodeExponent, Fi) > 1; --EncodeExponent);
      DecodeExponent = CalculateMultiplicativeInverseOfExponent(EncodeExponent, Fi);
    }
    while (DecodeExponent == EncodeExponent);

    PublicKey.Exponent = DecodeExponent;
    PublicKey.Modulus = Modulus;

    PrivateKey.Exponent = EncodeExponent;
    PrivateKey.Modulus = Modulus;
  }

  /**
  Raise Base to power of Exponent in mod Modulus.
  */
  template <typename IntType>
  inline static IntType ModularPow(IntType Base, IntType Exponent, IntType Modulus)
  {
    const IntType One(1);
    IntType result(One);
    while (Exponent > 0)
    {
      if ((Exponent & One) != 0)
      {
        result = (result * Base) % Modulus;
      }
      Exponent >>= 1;
      Base = (Base * Base) % Modulus;
    }
    return result;
  }
};

/**
 * Specialization for int256 (performance). Avoids using temporary results and most of the operations are inplace.
 */
template <>
inline int256 CEncryption::ModularPow(int256 Base, int256 Exponent, int256 Modulus)
{
  static const int256 One(1);
  int256 result(One);
  while (Exponent.IsGreaterThanZero())
  {
    if (!(Exponent & One).IsZero())
    {
      result.Multiply(Base);
      result.Modulo(Modulus);
    }
    Exponent.ShiftRightInternal(1);
    Base.Multiply(Base);
    Base.Modulo(Modulus);
  }
  return result;
}


struct CSignature
{
  int256 Data[20];

  void Serialize(Archive& ar)
  {
    for (int32 Index = 0; Index < countof(Data); ++Index)
    {
      ar & Data[Index];
    }
  }

  static int64 Size()
  {
    return sizeof(int256) * 20;
  }
};

} // namespace fun
