#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Used to read/write a bit in the static array as a bool.
 */
template <typename T>
class StaticBitReference
{
 public:
  StaticBitReference(T& data,T mask)
    : data_(data)
    , mask_(mask)
  {
  }

  FUN_ALWAYS_INLINE explicit operator bool () const
  {
     return (data_ & mask_) != 0;
  }

  FUN_ALWAYS_INLINE void operator = (const bool value)
  {
    if (value) {
      data_ |= mask_;
    }
    else {
      data_ &= ~mask_;
    }
  }

 private:
  T& data_;
  T mask_;
};


/**
 * Used to read a bit in the static array as a bool.
 */
template <typename T>
class ConstStaticBitReference
{
 public:
  ConstStaticBitReference(const T& data,T mask)
    : data_(data)
    , mask_(mask)
  {
  }

  FUN_ALWAYS_INLINE explicit operator bool () const
  {
     return (data_ & mask_) != 0;
  }

 private:
  const T& data_;
  T mask_;
};


/**
 * A statically sized bit array.
 */
template <uint32 NumBits>
class StaticBitArray
{
  typedef uint64 WordType;

  struct BoolType;
  typedef int32* BoolType::* UnspecifiedBoolType;
  typedef float* BoolType::* UnspecifiedZeroType;

 public:
  /**
   * Minimal initialization constructor
   */
  FUN_ALWAYS_INLINE StaticBitArray()
  {
    Clear_internal();
  }

  /**
   * Constructor that allows initializing by assignment from 0
   */
  FUN_ALWAYS_INLINE StaticBitArray(UnspecifiedZeroType)
  {
    Clear_internal();
  }

  /**
   * Constructor to initialize to a single bit
   */
  FUN_ALWAYS_INLINE StaticBitArray(bool, uint32 bit_index)
  {
    // If this line fails to compile you are attempting to construct a bit
    // array with an out-of bounds bit index. Follow the compiler errors to
    // the initialization point
    //static_assert(bit_index >= 0 && bit_index < NumBits, "Invalid bit value.");

    fun_check((NumBits > 0) ? (bit_index < NumBits) : 1);

    const uint32 dest_word_index = bit_index / kNumBitsPerWord;
    const WordType word = (WordType)1 << (bit_index & (kNumBitsPerWord - 1));

    for (int32 i = 0; i < kNumWords; ++i) {
      words_[i] = i == dest_word_index ? word : (WordType)0;
    }
  }

  //TODO
  //TODO
  //TODO
  //TODO
  /**
   * Constructor to initialize from string
   */
  //explicit StaticBitArray(const String& str)
  //{
  //  int32 len = str.Len();
  //
  //  // Trim count to length of bit array
  //  if (NumBits < len) {
  //    len = NumBits;
  //  }
  //  Clear_internal();
  //
  //  int32 pos = len;
  //  for (int32 i = 0; i < len; ++i) {
  //    const char ch = str[--pos];
  //    if (ch == '1') {
  //      operator[](i) = true;
  //    }
  //    else if (ch != '0') {
  //      ErrorInvalid_INTERNAL();
  //    }
  //  }
  //}

  // Conversion to bool
  FUN_ALWAYS_INLINE operator UnspecifiedBoolType () const
  {
    WordType a = 0;
    for (int32 i = 0; i < kNumWords; ++i) {
      a |= words_[i];
    }

    return a ? &BoolType::valid : nullptr;
  }

  // Accessors.
  FUN_ALWAYS_INLINE static int32 Count()
  {
    return NumBits;
  }

  FUN_ALWAYS_INLINE StaticBitReference<WordType> operator[](int32 i)
  {
    fun_check(i >= 0 && i < NumBits);
    return StaticBitReference<WordType>(
                  words_[i / kNumBitsPerWord],
                  (WordType)1 << (i & (kNumBitsPerWord - 1)));
  }

  FUN_ALWAYS_INLINE const ConstStaticBitReference<WordType> operator[](int32 i) const
  {
    fun_check(i >= 0 && i < NumBits);
    return ConstStaticBitReference<WordType>(
                  words_[i / kNumBitsPerWord],
                  (WordType)1 << (i & (kNumBitsPerWord - 1)));
  }

  // Modifiers.
  FUN_ALWAYS_INLINE StaticBitArray& operator |= (const StaticBitArray& other)
  {
    for (int32 i = 0; i < kNumWords; ++i) {
      words_[i] |= other.words_[i];
    }
    return *this;
  }

  FUN_ALWAYS_INLINE StaticBitArray& operator &= (const StaticBitArray& other)
  {
    for (int32 i = 0; i < kNumWords; ++i) {
      words_[i] &= other.words_[i];
    }
    return *this;
  }

  FUN_ALWAYS_INLINE StaticBitArray& operator ^= (const StaticBitArray& other)
  {
    for (int32 i = 0; i < kNumWords; ++i) {
      words_[i] ^= other.words_[i];
    }
    return *this;
  }

  friend FUN_ALWAYS_INLINE StaticBitArray<NumBits> operator ~ (const StaticBitArray<NumBits>& other)
  {
    StaticBitArray result;
    for (int32 i = 0; i < kNumWords; ++i) {
      result.words_[i] = ~other.words_[i];
    }
    result.Trim_internal();
    return result;
  }

  friend FUN_ALWAYS_INLINE StaticBitArray<NumBits> operator | (const StaticBitArray<NumBits>& a, const StaticBitArray<NumBits>& b)
  {
    // is not calling |= because doing it in here has less LoadHitStores and is therefore faster.
    StaticBitArray results(0);

    const WordType* __restrict a_ptr = (const WordType* __restrict)a.words_;
    const WordType* __restrict b_ptr = (const WordType* __restrict)b.words_;
    WordType* __restrict result_ptr = (WordType* __restrict)results.words_;
    for (int32 i = 0; i < kNumWords; ++i) {
      result_ptr[i] = a_ptr[i] | b_ptr[i];
    }

    return results;
  }

  friend FUN_ALWAYS_INLINE StaticBitArray<NumBits> operator & (const StaticBitArray<NumBits>& a, const StaticBitArray<NumBits>& b)
  {
    // is not calling &= because doing it in here has less LoadHitStores and is therefore faster.
    StaticBitArray results(0);

    const WordType* __restrict a_ptr = (const WordType* __restrict)a.words_;
    const WordType* __restrict b_ptr = (const WordType* __restrict)b.words_;
    WordType* __restrict result_ptr = (WordType* __restrict)results.words_;
    for (int32 i = 0; i < kNumWords; ++i) {
      result_ptr[i] = a_ptr[i] & b_ptr[i];
    }

    return results;
  }

  friend FUN_ALWAYS_INLINE StaticBitArray<NumBits> operator ^ (const StaticBitArray<NumBits>& a, const StaticBitArray<NumBits>& b)
  {
    StaticBitArray results(a);
    results ^= b;
    return results;
  }

  friend FUN_ALWAYS_INLINE bool operator == (const StaticBitArray<NumBits>& a, const StaticBitArray<NumBits>& b)
  {
    for (int32 i = 0; i < a.kNumWords; ++i) {
      if (a.words_[i] != b.words_[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   * This operator only exists to disambiguate == in statements of the form (flags == 0)
   */
  friend FUN_ALWAYS_INLINE bool operator == (const StaticBitArray<NumBits>& array, UnspecifiedBoolType value)
  {
    return (UnspecifiedBoolType)array == value;
  }

  /**
   * != simple maps to ==
   */
  friend FUN_ALWAYS_INLINE bool operator != (const StaticBitArray<NumBits>& a, const StaticBitArray<NumBits>& b)
  {
    return !(a == b);
  }

  /**
   * != simple maps to ==
   */
  friend FUN_ALWAYS_INLINE bool operator != (const StaticBitArray<NumBits>& array, UnspecifiedBoolType value)
  {
    return !(array == value);
  }

  /**
   * Serializer.
   */
  friend Archive& operator & (Archive& ar, StaticBitArray& bit_array)
  {
    uint32 archive_word_count = bit_array.kNumWords;
    ar & archive_word_count;

    if (ar.IsLoading()) {
      UnsafeMemory::Memzero(bit_array.words_, sizeof(bit_array.words_));
      archive_word_count = MathBase::Min(bit_array.kNumWords, archive_word_count);
    }

    ar.Serialize(bit_array.words_, archive_word_count * sizeof(bit_array.words_[0]));
    return ar;
  }

  //TODO
  //TODO
  //TODO
  ///**
  // * Converts the bitarray to a String representing the binary representation of the array
  // */
  //String ToString() const
  //{
  //  String str;
  //  str.Clear(NumBits);
  //
  //  for (int32 i = NumBits - 1; i >= 0; --i) {
  //    str += operator[](i) ? '1' : '0';
  //  }
  //
  //  return str;
  //}

  static const uint32 NumOfBits = NumBits;

 private:
  //static_assert(NumBits > 0, "Must have at least 1 bit.");
  static const uint32 kNumBitsPerWord = sizeof(WordType) * 8;
  static const uint32 kNumWords = ((NumBits + kNumBitsPerWord - 1) & ~(kNumBitsPerWord - 1)) / kNumBitsPerWord;
  WordType words_[kNumWords];

  // Helper class for bool conversion
  struct BoolType
  {
    int32* valid;
  };

  /**
   * Resets the bit array to a 0 value
   */
  FUN_ALWAYS_INLINE void Clear_internal()
  {
    for (int32 i = 0; i < kNumWords; ++i) {
      words_[i] = 0;
    }
  }

  /**
   * Clears any trailing bits in the last word
   */
  void Trim_internal()
  {
    if (NumBits % kNumBitsPerWord != 0) {
      words_[kNumWords-1] &= (WordType(1) << (NumBits % kNumBitsPerWord)) - 1;
    }
  }

  /**
   * Reports an invalid String element in the bitset conversion
   */
  void ErrorInvalid_INTERNAL() const
  {
    //TODO 차라리 로그 fatal로 하는게 좋을듯??
    //LowLevelFatalError("invalid StaticBitArray<NumBits> character");
  }
};

} // namespace fun
