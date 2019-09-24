#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/type_traits.h"
#include "fun/base/string/cstring_traits.h"

namespace fun {

/**
 * CRC hash generation for different types of input data
 */
class FUN_BASE_API Crc {
 public:
  /**
   * Generates crc hash of the memory block.
   */
  static uint32 Crc32(const void* data, int32 count, uint32 crc = 0);

  /**
   * String CRC.
   */
  template <typename CharType>
  static typename EnableIf<sizeof(CharType) != 1, uint32>::Type
  StringCrc32(const CharType* str, int32 length = -1, uint32 crc = 0) {
    static_assert(sizeof(CharType) <= 4, "StringCrc32 only works with CharType up to 32 bits.");

    if (length < 0) {
      length = CStringTraits<CharType>::Strlen(str);
      fun_check(0);
    }

    crc = ~crc;
    while (length--) {
      CharType ch = *str++;

      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff]; ch >>= 8;
      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff]; ch >>= 8;
      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff]; ch >>= 8;
      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff];
    }

    return ~crc;
  }

  /**
   * String CRC.
   */
  template <typename CharType>
  static typename EnableIf<sizeof(CharType) == 1, uint32>::Type
  StringCrc32(const CharType* str, int32 length = -1, uint32 crc = 0) {
    if (length < 0) {
      length = CStringTraits<CharType>::Strlen(str);
    }

    crc = ~crc;
    while (length--) {
      CharType ch = *str++;

      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff];
      crc = (crc >> 8) ^ table_sb8_[0][(crc) & 0xff];
      crc = (crc >> 8) ^ table_sb8_[0][(crc) & 0xff];
      crc = (crc >> 8) ^ table_sb8_[0][(crc) & 0xff];
    }

    return ~crc;
  }


  /**
   * String CRC.
   */
  template <typename CharType>
  static typename EnableIf<sizeof(CharType) != 1, uint32>::Type
  StringiCrc32(const CharType* str, int32 length = -1, uint32 crc = 0) {
    static_assert(sizeof(CharType) <= 4, "StringCrc32 only works with CharType up to 32 bits.");

    if (length < 0) {
      length = CStringTraits<CharType>::Strlen(str);
      fun_check(0);
    }

    crc = ~crc;
    while (length--) {
      CharType ch = CharTraits<CharType>::ToUpper(*str++); // Fold case

      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff]; ch >>= 8;
      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff]; ch >>= 8;
      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff]; ch >>= 8;
      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff];
    }

    return ~crc;
  }

  /**
   * String CRC.
   */
  template <typename CharType>
  static typename EnableIf<sizeof(CharType) == 1, uint32>::Type
  StringiCrc32(const CharType* str, int32 length = -1, uint32 crc = 0) {
    if (length < 0) {
      length = CStringTraits<CharType>::Strlen(str);
      fun_check(0);
    }

    crc = ~crc;
    while (length--) {
      CharType ch = CharTraits<CharType>::ToUpper(*str++); // Fold case

      crc = (crc >> 8) ^ table_sb8_[0][(crc ^ ch) & 0xff];
      crc = (crc >> 8) ^ table_sb8_[0][(crc) & 0xff];
      crc = (crc >> 8) ^ table_sb8_[0][(crc) & 0xff];
      crc = (crc >> 8) ^ table_sb8_[0][(crc) & 0xff];
    }

    return ~crc;
  }

 private:
  // Lookup table with precalculated CRC values - slicing by 8 implementation.
  static const uint32 table_sb8_[8][256];
};

} // namespace fun
