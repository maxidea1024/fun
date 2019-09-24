#pragma once

#include "fun/base/base.h"
#include "fun/base/container/allocation_policies.h"
#include "fun/base/container/array_view.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

template <typename T>
struct IsUtf8Char : FalseType {};
template <>
struct IsUtf8Char<char> : TrueType {};
template <>
struct IsUtf8Char<int8> : TrueType {};
template <>
struct IsUtf8Char<uint8> : TrueType {};

template <typename T>
struct IsUtf16Char : FalseType {};
template <>
struct IsUtf16Char<int16> : TrueType {};
template <>
struct IsUtf16Char<uint16> : TrueType {};
template <>
struct IsUtf16Char<char16_t> : TrueType {};

// template <> struct IsUtf16Char<UNICHAR>  : TrueType {};

template <typename T>
struct IsUtf32Char : FalseType {};
template <>
struct IsUtf32Char<int32> : TrueType {};
template <>
struct IsUtf32Char<uint32> : TrueType {};
template <>
struct IsUtf32Char<char32_t> : TrueType {};

template <>
struct IsUtf16Char<wchar_t> {
  enum { Value = sizeof(wchar_t) == 2 };
};
template <>
struct IsUtf32Char<wchar_t> {
  enum { Value = sizeof(wchar_t) == 4 };
};

template <typename T>
struct UtfCharTypeResolver {};

template <>
struct UtfCharTypeResolver<char> {
  typedef UTF8CHAR Type;
};
template <>
struct UtfCharTypeResolver<int8> {
  typedef UTF8CHAR Type;
};
template <>
struct UtfCharTypeResolver<uint8> {
  typedef UTF8CHAR Type;
};

template <>
struct UtfCharTypeResolver<int16> {
  typedef UTF16CHAR Type;
};
template <>
struct UtfCharTypeResolver<uint16> {
  typedef UTF16CHAR Type;
};
template <>
struct UtfCharTypeResolver<char16_t> {
  typedef UTF16CHAR Type;
};

template <>
struct UtfCharTypeResolver<int32> {
  typedef UTF32CHAR Type;
};
template <>
struct UtfCharTypeResolver<uint32> {
  typedef UTF32CHAR Type;
};
template <>
struct UtfCharTypeResolver<char32_t> {
  typedef UTF32CHAR Type;
};

template <>
struct UtfCharTypeResolver<wchar_t> {
  typedef
      typename Conditional<sizeof(wchar_t) == 2, UTF16CHAR, UTF32CHAR>::Result
          Type;
};

class FUN_BASE_API Utf {
 public:
  enum ConversionResult {
    ConversionOK,
    SourceExhausted,
    TargetExhausted,
    SourceIllegal,
  };

  template <typename CharType>
  struct CountingIterator {
    FUN_ALWAYS_INLINE CountingIterator() : ptr(nullptr) {}
    FUN_ALWAYS_INLINE operator CharType*() { return ptr; }
    FUN_ALWAYS_INLINE CountingIterator& operator=(const CountingIterator& rhs) {
      ptr = rhs.ptr;
      return *this;
    }
    FUN_ALWAYS_INLINE CountingIterator& operator=(CharType* ptr) {
      this->ptr = ptr;
      return *this;
    }
    FUN_ALWAYS_INLINE const CountingIterator& operator*() const {
      return *this;
    }
    FUN_ALWAYS_INLINE const CountingIterator& operator++() {
      ++ptr;
      return *this;
    }
    FUN_ALWAYS_INLINE const CountingIterator& operator++(int) {
      ++ptr;
      return *this;
    }
    FUN_ALWAYS_INLINE const CountingIterator& operator--() {
      --ptr;
      return *this;
    }
    FUN_ALWAYS_INLINE const CountingIterator& operator--(int) {
      --ptr;
      return *this;
    }
    FUN_ALWAYS_INLINE CountingIterator& operator+=(int distance) {
      ptr += distance;
      return *this;
    }
    FUN_ALWAYS_INLINE CountingIterator& operator-=(int distance) {
      ptr -= distance;
      return *this;
    }
    FUN_ALWAYS_INLINE CharType operator=(CharType val) const { return val; }
    FUN_ALWAYS_INLINE friend CountingIterator operator+(
        const CountingIterator& lhs, int32 distance) {
      CountingIterator ret;
      ret.ptr += distance;
      return ret;
    }
    FUN_ALWAYS_INLINE friend int32 operator-(const CountingIterator& lhs,
                                             const CountingIterator& rhs) {
      return int32(lhs.ptr - rhs.ptr);
    }
    FUN_ALWAYS_INLINE static CountingIterator begin() {
      return CountingIterator(((CharType*)0));
    }
    FUN_ALWAYS_INLINE static CountingIterator end() {
      return CountingIterator((CharType*)-1);
    }

   private:
    CharType* ptr;
    FUN_ALWAYS_INLINE CountingIterator(CharType* p) : ptr(p) {}
  };

  template <typename FromCharType>
  static const FromCharType* GetSourceEnd(const FromCharType** src,
                                          int32 src_len) {
    if (src == nullptr || *src == nullptr) {
      return *src;
    }

    return src_len < 0 ? (*src) + Strlen(*src) : (*src) + src_len;
  }

  template <typename FromCharType>
  static const FromCharType* GetSourceEnd(const FromCharType* src,
                                          int32 src_len) {
    if (src == nullptr) {
      return src;
    }

    return src_len < 0 ? src + Strlen(src) : src + src_len;
  }

  // TODO 아래것 같은것들이 필요할까??
  //그냥 더미를 두어서 잡아내는 코드만 만들어줄까??

  // UTF8CHAR -> UTF8CHAR
  // UTF16CHAR -> UTF16CHAR
  // UTF32CHAR -> UTF32CHAR

  //
  // UTF8CHAR -> UTF16CHAR
  //

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf8Char<FromCharType>::Value &&
                               IsUtf16Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType* src, int32 src_len, ToCharType* dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = dst + dst_len;
    return Convert_8_TO_16((const UTF8CHAR**)&src, (const UTF8CHAR*)src_end,
                           (UTF16CHAR**)&dst, (UTF16CHAR*)dst_end,
                           (UTF16CHAR)bogus_char, strict);
  }

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf8Char<FromCharType>::Value &&
                               IsUtf16Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType** src, int32 src_len, ToCharType** dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = *dst + dst_len;
    return Convert_8_TO_16((const UTF8CHAR**)src, (const UTF8CHAR*)src_end,
                           (UTF16CHAR**)dst, (UTF16CHAR*)dst_end,
                           (UTF16CHAR)bogus_char, strict);
  }

  template <typename FromCharType>
  static typename EnableIf<IsUtf8Char<FromCharType>::Value, int32>::Type
  LengthAsUtf16(const FromCharType* src, int32 src_len = -1,
                bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    CountingIterator<UTF16CHAR> output_begin;
    CountingIterator<UTF16CHAR> output;
    Convert_8_TO_16((const UTF8CHAR**)&src, (const UTF8CHAR*)src_end, &output,
                    output.end(), '?', strict);
    return output - output_begin;
  }

  //
  // UTF16CHAR -> UTF8CHAR
  //

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf16Char<FromCharType>::Value &&
                               IsUtf8Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType* src, int32 src_len, ToCharType* dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = dst + dst_len;
    return Convert_16_TO_8((const UTF16CHAR**)&src, (const UTF16CHAR*)src_end,
                           (UTF8CHAR**)&dst, (UTF8CHAR*)dst_end,
                           (UTF8CHAR)bogus_char, strict);
  }

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf16Char<FromCharType>::Value &&
                               IsUtf8Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType** src, int32 src_len, ToCharType** dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = *dst + dst_len;
    return Convert_16_TO_8((const UTF16CHAR**)src, (const UTF16CHAR*)src_end,
                           (UTF8CHAR**)dst, (UTF8CHAR*)dst_end,
                           (UTF8CHAR)bogus_char, strict);
  }

  template <typename FromCharType>
  static typename EnableIf<IsUtf16Char<FromCharType>::Value, int32>::Type
  LengthAsUtf8(const FromCharType* src, int32 src_len = -1,
               bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    CountingIterator<UTF8CHAR> output_begin;
    CountingIterator<UTF8CHAR> output;
    Convert_16_TO_8((const UTF16CHAR**)&src, (const UTF16CHAR*)src_end, &output,
                    output.end(), '?', strict);
    return output - output_begin;
  }

  //
  // UTF8CHAR -> UTF32CHAR
  //

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf8Char<FromCharType>::Value &&
                               IsUtf32Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType* src, int32 src_len, ToCharType* dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = dst + dst_len;
    return Convert_8_TO_32((const UTF8CHAR**)&src, (const UTF8CHAR*)src_end,
                           (UTF32CHAR**)&dst, (UTF32CHAR*)dst_end,
                           (UTF32CHAR)bogus_char, strict);
  }

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf8Char<FromCharType>::Value &&
                               IsUtf32Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType** src, int32 src_len, ToCharType** dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = *dst + dst_len;
    return Convert_8_TO_32((const UTF8CHAR**)src, (const UTF8CHAR*)src_end,
                           (UTF32CHAR**)dst, (UTF32CHAR*)dst_end,
                           (UTF32CHAR)bogus_char, strict);
  }

  template <typename FromCharType>
  static typename EnableIf<IsUtf8Char<FromCharType>::Value, int32>::Type
  LengthAsUtf32(const FromCharType* src, int32 src_len = -1,
                bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    CountingIterator<UTF32CHAR> output_begin;
    CountingIterator<UTF32CHAR> output;
    Convert_8_TO_32((const UTF8CHAR**)&src, (const UTF8CHAR*)src_end, &output,
                    output.end(), '?', strict);
    return output - output_begin;
  }

  //
  // UTF32CHAR -> UTF8CHAR
  //

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf32Char<FromCharType>::Value &&
                               IsUtf8Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType* src, int32 src_len, ToCharType* dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = dst + dst_len;
    return Convert_32_TO_8((const UTF32CHAR**)&src, (const UTF32CHAR*)src_end,
                           (UTF8CHAR**)&dst, (UTF8CHAR*)dst_end,
                           (UTF8CHAR)bogus_char, strict);
  }

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf32Char<FromCharType>::Value &&
                               IsUtf8Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType** src, int32 src_len, ToCharType** dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = *dst + dst_len;
    return Convert_32_TO_8((const UTF32CHAR**)src, (const UTF32CHAR*)src_end,
                           (UTF8CHAR**)dst, (UTF8CHAR*)dst_end,
                           (UTF8CHAR)bogus_char, strict);
  }

  template <typename FromCharType>
  static typename EnableIf<IsUtf32Char<FromCharType>::Value, int32>::Type
  LengthAsUtf8(const FromCharType* src, int32 src_len = -1,
               bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    CountingIterator<UTF8CHAR> output_begin;
    CountingIterator<UTF8CHAR> output;
    Convert_32_TO_8((const UTF32CHAR**)&src, (const UTF32CHAR*)src_end, &output,
                    output.end(), '?', strict);
    return output - output_begin;
  }

  //
  // UTF16CHAR -> UTF32CHAR
  //

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf16Char<FromCharType>::Value &&
                               IsUtf32Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType* src, int32 src_len, ToCharType* dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = dst + dst_len;
    return Convert_16_TO_32((const UTF16CHAR**)&src, (const UTF16CHAR*)src_end,
                            (UTF32CHAR**)&dst, (UTF32CHAR*)dst_end,
                            (UTF32CHAR)bogus_char, strict);
  }

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf16Char<FromCharType>::Value &&
                               IsUtf32Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType** src, int32 src_len, ToCharType** dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = *dst + dst_len;
    return Convert_16_TO_32((const UTF16CHAR**)src, (const UTF16CHAR*)src_end,
                            (UTF32CHAR**)dst, (UTF32CHAR*)dst_end,
                            (UTF32CHAR)bogus_char, strict);
  }

  template <typename FromCharType>
  static typename EnableIf<IsUtf16Char<FromCharType>::Value, int32>::Type
  LengthAsUtf32(const FromCharType* src, int32 src_len = -1,
                bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    CountingIterator<UTF32CHAR> output_begin;
    CountingIterator<UTF32CHAR> output;
    Convert_16_TO_32((const UTF16CHAR**)&src, (const UTF16CHAR*)src_end,
                     &output, output.end(), '?', strict);
    return output - output_begin;
  }

  //
  // UTF32CHAR -> UTF16CHAR
  //

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf32Char<FromCharType>::Value &&
                               IsUtf16Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType* src, int32 src_len, ToCharType* dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = dst + dst_len;
    return Convert_32_TO_16((const UTF32CHAR**)&src, (UTF32CHAR*)src_end,
                            (UTF16CHAR**)&dst, (UTF16CHAR*)dst_end,
                            (UTF16CHAR)bogus_char, strict);
  }

  template <typename FromCharType, typename ToCharType>
  static typename EnableIf<IsUtf32Char<FromCharType>::Value &&
                               IsUtf16Char<ToCharType>::Value,
                           ConversionResult>::Type
  Convert(const FromCharType** src, int32 src_len, ToCharType** dst,
          int32 dst_len, const char bogus_char = '?', bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    ToCharType* dst_end = *dst + dst_len;
    return Convert_32_TO_16((const UTF32CHAR**)src, (UTF32CHAR*)src_end,
                            (UTF16CHAR**)dst, (UTF16CHAR*)dst_end,
                            (UTF16CHAR)bogus_char, strict);
  }

  template <typename FromCharType>
  static typename EnableIf<IsUtf32Char<FromCharType>::Value, int32>::Type
  LengthAsUtf16(const FromCharType* src, int32 src_len = -1,
                bool strict = false) {
    const FromCharType* src_end = GetSourceEnd(src, src_len);
    CountingIterator<UTF16CHAR> output_begin;
    CountingIterator<UTF16CHAR> output;
    Convert_32_TO_16((const UTF32CHAR**)&src, (const UTF32CHAR*)src_end,
                     &output, output.end(), '?', strict);
    return output - output_begin;
  }

  template <typename CharType>
  static int32 Strlen(const CharType* str) {
    if (str == nullptr) {
      return 0;
    }

    const CharType* s = str;
    while (*s++)
      ;
    return int32(s - str);
  }

  //
  // Low level routines
  //

  static const int32 HALF_SHIFT = 10;  // used for shifting by 10 bits
  static const UTF32CHAR HALF_BASE = 0x0010000UL;
  static const UTF32CHAR HALF_MASK = 0x3FFUL;

  static const UTF32CHAR SURROGATE_HIGH_START = 0xD800;
  static const UTF32CHAR SURROGATE_HIGH_END = 0xDBFF;
  static const UTF32CHAR SURROGATE_LOW_START = 0xDC00;
  static const UTF32CHAR SURROGATE_LOW_END = 0xDFFF;

  static const UTF32CHAR MAX_BMP = 0x0000FFFF;
  static const UTF32CHAR MAX_UTF16 = 0x0010FFFF;
  static const UTF32CHAR MAX_UTF32 = 0x7FFFFFFF;
  static const UTF32CHAR MAX_LEGAL_UTF32 = 0x0010FFFF;

  // Index into the table below with the first byte of a UTF-8 sequence to
  // get the number of trailing bytes that are supposed to follow it.
  // Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
  // left as-is for anyone who may want to do such conversion, which was
  // allowed in earlier algorithms.
  static const char TRAILING_BYTES_FOR_UTF8[256];

  // Magic values subtracted from a buffer value during UTF8CHAR conversion.
  // This table contains as many values as there might be trailing bytes
  // in a UTF-8 sequence.
  static const UTF32CHAR OFFSETS_FROM_UTF8[6];

  // Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
  // into the first byte, depending on how many bytes follow.  There are
  // as many entries in this table as there are UTF-8 sequence types.
  // (I.e., one byte sequence, two byte... etc.). Remember that sequencs
  // for *legal* UTF-8 will be 4 or fewer bytes total.
  static const UTF8CHAR FIRST_BYTE_MARK[7];

  /**
   * static ConversionResult Convert_8_TO_16(
   *   const UTF8CHAR** src_begin, const UTF8CHAR* src_end,
   *   UTF16CHAR** dst_begin, UTF16CHAR* dst_end,
   *   const UTF16CHAR bogus_char = '?',
   *   bool strict = false);
   */
  template <typename OutputIt>
  static ConversionResult Convert_8_TO_16(const UTF8CHAR** src_begin,
                                          const UTF8CHAR* src_end,
                                          OutputIt* dst_begin, OutputIt dst_end,
                                          const UTF16CHAR bogus_char = '?',
                                          bool strict = false) {
    ConversionResult result = ConversionOK;
    const UTF8CHAR* src = *src_begin;
    OutputIt dst = *dst_begin;
    while (src < src_end) {
      UTF32CHAR ch = 0;
      uint16 extra_bytes_to_read = TRAILING_BYTES_FOR_UTF8[*src];
      if (src + extra_bytes_to_read >= src_end) {
        result = SourceExhausted;
        break;
      }
      // Do this check whether lenient or strict
      if (!IsLegalUtf8(src, extra_bytes_to_read + 1)) {
        result = SourceIllegal;
        break;
      }

      // The cases all fall through. See "Note A" below.
      switch (extra_bytes_to_read) {
        case 5:
          ch += *src++;
          ch <<= 6;  // remember, illegal UTF-8
        case 4:
          ch += *src++;
          ch <<= 6;  // remember, illegal UTF-8
        case 3:
          ch += *src++;
          ch <<= 6;  // fall through
        case 2:
          ch += *src++;
          ch <<= 6;  // fall through
        case 1:
          ch += *src++;
          ch <<= 6;  // fall through
        case 0:
          ch += *src++;
          break;
      }
      ch -= OFFSETS_FROM_UTF8[extra_bytes_to_read];

      if (dst >= dst_end) {
        src -= (extra_bytes_to_read + 1);  // Back up src pointer!
        result = TargetExhausted;
        break;
      }

      if (ch <= MAX_BMP) {  // dst is a ch <= 0xFFFF
        // UTF-16 surrogate values are illegal in UTF-32
        if (ch >= SURROGATE_HIGH_START && ch <= SURROGATE_LOW_END) {
          if (strict) {
            src -= (extra_bytes_to_read +
                    1);  // return to the illegal value itself
            result = SourceIllegal;
            break;
          } else {
            *dst++ = bogus_char;
          }
        } else {
          *dst++ = UTF16CHAR(ch);  // normal case
        }
      } else if (ch > MAX_UTF16) {
        if (strict) {
          result = SourceIllegal;
          src -= (extra_bytes_to_read + 1);  // return to the start
          break;                             // Bail out; shouldn't continue
        } else {
          *dst++ = bogus_char;
        }
      } else {
        // dst is a ch in range 0xFFFF - 0x10FFFF.
        if (dst + 1 >= dst_end) {
          src -= (extra_bytes_to_read + 1);  // Back up src pointer!
          result = TargetExhausted;
          break;
        }
        ch -= HALF_BASE;
        *dst++ = UTF16CHAR((ch >> HALF_SHIFT) + SURROGATE_HIGH_START);
        *dst++ = UTF16CHAR((ch & HALF_MASK) + SURROGATE_LOW_START);
      }
    }

    *src_begin = src;
    *dst_begin = dst;
    return result;
  }

  /**
   * static ConversionResult Convert_16_TO_8(
   *   const UTF16CHAR** src_begin, const UTF16CHAR* src_end,
   *   UTF8CHAR** dst_begin, UTF8CHAR* dst_end,
   *   const UTF8CHAR bogus_char = '?',
   *   bool strict = false);
   */
  template <typename OutputIt>
  static ConversionResult Convert_16_TO_8(const UTF16CHAR** src_begin,
                                          const UTF16CHAR* src_end,
                                          OutputIt* dst_begin, OutputIt dst_end,
                                          const UTF8CHAR bogus_char = '?',
                                          bool strict = false) {
    ConversionResult result = ConversionOK;
    const UTF16CHAR* src = *src_begin;
    OutputIt dst = *dst_begin;
    while (src < src_end) {
      UTF32CHAR ch;
      uint16 bytes_to_write = 0;
      const UTF32CHAR BYTE_MASK = 0xBF;
      const UTF32CHAR BYTE_MARK = 0x80;
      const UTF16CHAR* old_src =
          src;  // In case we have to back up because of dst overflow.

      ch = *src++;

      // If we have a surrogate pair, convert to UTF32CHAR first.
      if (ch >= SURROGATE_HIGH_START && ch <= SURROGATE_HIGH_END) {
        // If the 16 bits following the high surrogate are in the src buffer.
        if (src < src_end) {
          const UTF32CHAR ch2 = *src;

          // If it's a low surrogate, convert to UTF32CHAR.
          if (ch2 >= SURROGATE_LOW_START && ch2 <= SURROGATE_LOW_END) {
            ch = ((ch - SURROGATE_HIGH_START) << HALF_SHIFT) +
                 (ch2 - SURROGATE_LOW_START) + HALF_BASE;
            ++src;
          } else if (strict) {  // it's an unpaired high surrogate
            --src;              // return to the illegal value itself
            result = SourceIllegal;
            break;
          }
        } else {  // We don't have the 16 bits following the high surrogate.
          --src;  // return to the high surrogate
          result = SourceExhausted;
          break;
        }
      } else if (strict) {
        // UTF-16 surrogate values are illegal in UTF-32
        if (ch >= SURROGATE_LOW_START && ch <= SURROGATE_LOW_END) {
          --src;  // return to the illegal value itself
          result = SourceIllegal;
          break;
        }
      }

      // Figure out how many bytes the result will require
      if (ch < 0x80)
        bytes_to_write = 1;  // 1 byte
      else if (ch < 0x800)
        bytes_to_write = 2;  // 2 bytes
      else if (ch < 0x10000)
        bytes_to_write = 3;  // 3 bytes
      else if (ch < 0x110000)
        bytes_to_write = 4;  // 4 bytes
      else {
        bytes_to_write = 3;
        ch = bogus_char;
      }

      dst += bytes_to_write;
      if (dst > dst_end) {
        src = old_src;  // Back up src pointer!
        dst -= bytes_to_write;
        result = TargetExhausted;
        break;
      }

      switch (bytes_to_write) {  // note: everything falls through.
        case 4:
          *--dst = UTF8CHAR((ch | BYTE_MARK) & BYTE_MASK);
          ch >>= 6;  // fall through
        case 3:
          *--dst = UTF8CHAR((ch | BYTE_MARK) & BYTE_MASK);
          ch >>= 6;  // fall through
        case 2:
          *--dst = UTF8CHAR((ch | BYTE_MARK) & BYTE_MASK);
          ch >>= 6;  // fall through
        case 1:
          *--dst = UTF8CHAR(ch | FIRST_BYTE_MARK[bytes_to_write]);
          break;
      }
      dst += bytes_to_write;
    }

    *src_begin = src;
    *dst_begin = dst;
    return result;
  }

  /**
   * static ConversionResult Convert_8_TO_32(
   *   const UTF8CHAR** src_begin, const UTF8CHAR* src_end,
   *   UTF32CHAR** dst_begin, UTF32CHAR* dst_end,
   *   const UTF32CHAR bogus_char = '?',
   *   bool strict = false);
   */
  template <typename OutputIt>
  static ConversionResult Convert_8_TO_32(const UTF8CHAR** src_begin,
                                          const UTF8CHAR* src_end,
                                          OutputIt* dst_begin, OutputIt dst_end,
                                          const UTF32CHAR bogus_char = '?',
                                          bool strict = false) {
    ConversionResult result = ConversionOK;
    const UTF8CHAR* src = *src_begin;
    OutputIt dst = *dst_begin;
    while (src < src_end) {
      UTF32CHAR ch = 0;
      uint16 extra_bytes_to_read = TRAILING_BYTES_FOR_UTF8[*src];
      if (src + extra_bytes_to_read >= src_end) {
        result = SourceExhausted;
        break;
      }

      // Do this check whether lenient or strict
      if (!IsLegalUtf8(src, extra_bytes_to_read + 1)) {
        result = SourceIllegal;
        break;
      }

      // The cases all fall through. See "Note A" below.
      switch (extra_bytes_to_read) {
        case 5:
          ch += *src++;
          ch <<= 6;  // fall through
        case 4:
          ch += *src++;
          ch <<= 6;  // fall through
        case 3:
          ch += *src++;
          ch <<= 6;  // fall through
        case 2:
          ch += *src++;
          ch <<= 6;  // fall through
        case 1:
          ch += *src++;
          ch <<= 6;  // fall through
        case 0:
          ch += *src++;
          break;
      }
      ch -= OFFSETS_FROM_UTF8[extra_bytes_to_read];

      if (dst >= dst_end) {
        src -= (extra_bytes_to_read + 1);  // Back up the src pointer!
        result = TargetExhausted;
        break;
      }

      if (ch <= MAX_LEGAL_UTF32) {
        // UTF-16 surrogate values are illegal in UTF-32, and anything
        // over Plane 17 (> 0x10FFFF) is illegal.
        if (ch >= SURROGATE_HIGH_START && ch <= SURROGATE_LOW_END) {
          if (strict) {
            src -= (extra_bytes_to_read +
                    1);  // return to the illegal value itself
            result = SourceIllegal;
            break;
          } else {
            *dst++ = bogus_char;
          }
        } else {
          *dst++ = ch;
        }
      } else {  // i.e., ch > MAX_LEGAL_UTF32
        result = SourceIllegal;
        *dst++ = bogus_char;
      }
    }

    *src_begin = src;
    *dst_begin = dst;
    return result;
  }

  /**
   * static ConversionResult Convert_32_TO_8(
   *   const UTF32CHAR** src_begin, const UTF32CHAR* src_end,
   *   UTF8CHAR** dst_begin, UTF8CHAR* dst_end,
   *   const UTF8CHAR bogus_char = '?',
   *   bool strict = false);
   */
  template <typename OutputIt>
  static ConversionResult Convert_32_TO_8(const UTF32CHAR** src_begin,
                                          const UTF32CHAR* src_end,
                                          OutputIt* dst_begin, OutputIt dst_end,
                                          const UTF8CHAR bogus_char = '?',
                                          bool strict = false) {
    ConversionResult result = ConversionOK;
    const UTF32CHAR* src = *src_begin;
    OutputIt dst = *dst_begin;
    while (src < src_end) {
      UTF32CHAR ch;
      uint16 bytes_to_write = 0;
      const UTF32CHAR BYTE_MASK = 0xBF;
      const UTF32CHAR BYTE_MARK = 0x80;
      ch = *src++;
      if (strict) {
        // UTF-16 surrogate values are illegal in UTF-32
        if (ch >= SURROGATE_HIGH_START && ch <= SURROGATE_LOW_END) {
          --src;  // return to the illegal value itself
          result = SourceIllegal;
          break;
        }
      }

      // Figure out how many bytes the result will require. Turn any
      // illegally large UTF32CHAR things (> Plane 17) into replacement chars.

      if (ch < 0x80)
        bytes_to_write = 1;  // 1 byte
      else if (ch < 0x800)
        bytes_to_write = 2;  // 2 bytes
      else if (ch < 0x10000)
        bytes_to_write = 3;  // 3 bytes
      else if (ch <= MAX_LEGAL_UTF32)
        bytes_to_write = 4;  // 4 bytes
      else {
        bytes_to_write = 3;
        ch = bogus_char;
        result = SourceIllegal;
      }

      dst += bytes_to_write;
      if (dst > dst_end) {
        --src;  // Back up src pointer!
        dst -= bytes_to_write;
        result = TargetExhausted;
        break;
      }

      switch (bytes_to_write) {  // note: everything falls through.
        case 4:
          *--dst = UTF8CHAR((ch | BYTE_MARK) & BYTE_MASK);
          ch >>= 6;  // fall through
        case 3:
          *--dst = UTF8CHAR((ch | BYTE_MARK) & BYTE_MASK);
          ch >>= 6;  // fall through
        case 2:
          *--dst = UTF8CHAR((ch | BYTE_MARK) & BYTE_MASK);
          ch >>= 6;  // fall through
        case 1:
          *--dst = UTF8CHAR(ch | FIRST_BYTE_MARK[bytes_to_write]);
          break;
      }
      dst += bytes_to_write;
    }

    *src_begin = src;
    *dst_begin = dst;
    return result;
  }

  /**
   * static ConversionResult Convert_16_TO_32(
   *   const UTF16CHAR** src_begin, const UTF16CHAR* src_end,
   *   UTF32CHAR** dst_begin, UTF32CHAR* dst_end,
   *   const UTF32CHAR bogus_char = '?',
   *   bool strict = false);
   */
  template <typename OutputIt>
  static ConversionResult Convert_16_TO_32(const UTF16CHAR** src_begin,
                                           const UTF16CHAR* src_end,
                                           OutputIt* dst_begin,
                                           OutputIt dst_end,
                                           const UTF32CHAR bogus_char = '?',
                                           bool strict = false) {
    ConversionResult result = ConversionOK;
    const UTF16CHAR* src = *src_begin;
    OutputIt dst = *dst_begin;
    UTF32CHAR ch, ch2;
    while (src < src_end) {
      const UTF16CHAR* old_src =
          src;  //  In case we have to back up because of dst overflow.
      ch = *src++;

      // If we have a surrogate pair, convert to UTF32CHAR first.
      if (ch >= SURROGATE_HIGH_START && ch <= SURROGATE_HIGH_END) {
        // If the 16 bits following the high surrogate are in the src buffer...
        if (src < src_end) {
          ch2 = *src;

          // If it's a low surrogate, convert to UTF32CHAR.
          if (ch2 >= SURROGATE_LOW_START && ch2 <= SURROGATE_LOW_END) {
            ch = ((ch - SURROGATE_HIGH_START) << HALF_SHIFT) +
                 (ch2 - SURROGATE_LOW_START) + HALF_BASE;
            ++src;
          } else if (strict) {  // it's an unpaired high surrogate
            --src;              // return to the illegal value itself
            result = SourceIllegal;
            break;
          }
        } else {  // We don't have the 16 bits following the high surrogate.
          --src;  // return to the high surrogate
          result = SourceExhausted;
          break;
        }
      } else if (strict) {
        // UTF-16 surrogate values are illegal in UTF-32
        if (ch >= SURROGATE_LOW_START && ch <= SURROGATE_LOW_END) {
          --src;  // return to the illegal value itself
          result = SourceIllegal;
          break;
        }
      }

      if (dst >= dst_end) {
        src = old_src;  // Back up src pointer!
        result = TargetExhausted;
        break;
      }
      *dst++ = ch;
    }

    *src_begin = src;
    *dst_begin = dst;
    return result;
  }

  /**
   * static ConversionResult Convert_32_TO_16(
   *   const UTF32CHAR** src_begin, const UTF32CHAR* src_end,
   *   UTF16CHAR** dst_begin, UTF16CHAR* dst_end,
   *   const UTF16CHAR bogus_char = '?',
   *   bool strict = false);
   */
  template <typename OutputIt>
  static ConversionResult Convert_32_TO_16(const UTF32CHAR** src_begin,
                                           const UTF32CHAR* src_end,
                                           OutputIt* dst_begin,
                                           OutputIt dst_end,
                                           const UTF16CHAR bogus_char = '?',
                                           bool strict = false) {
    ConversionResult result = ConversionOK;
    const UTF32CHAR* src = *src_begin;
    OutputIt dst = *dst_begin;
    while (src < src_end) {
      UTF32CHAR ch;
      if (dst >= dst_end) {
        result = TargetExhausted;
        break;
      }

      ch = *src++;

      if (ch <= MAX_BMP) {  // dst is a ch <= 0xFFFF
        // UTF-16 surrogate values are illegal in UTF-32; 0xFFFF
        // or 0xFFFE are both reserved values
        if (ch >= SURROGATE_HIGH_START && ch <= SURROGATE_LOW_END) {
          if (strict) {
            --src;  // return to the illegal value itself
            result = SourceIllegal;
            break;
          } else {
            *dst++ = bogus_char;
          }
        } else {
          *dst++ = UTF16CHAR(ch);  // normal case
        }
      } else if (ch > MAX_LEGAL_UTF32) {
        if (strict) {
          result = SourceIllegal;
        } else {
          *dst++ = bogus_char;
        }
      } else {
        // dst is a ch in range 0xFFFF - 0x10FFFF.
        if (dst + 1 >= dst_end) {
          --src;  // Back up src pointer!
          result = TargetExhausted;
          break;
        }
        ch -= HALF_BASE;
        *dst++ = UTF16CHAR((ch >> HALF_SHIFT) + SURROGATE_HIGH_START);
        *dst++ = UTF16CHAR((ch & HALF_MASK) + SURROGATE_LOW_START);
      }
    }

    *src_begin = src;
    *dst_begin = dst;
    return result;
  }

  static bool IsLegalUtf8Sequence(const UTF8CHAR* src_begin,
                                  const UTF8CHAR* src_end) {
    const int32 len = TRAILING_BYTES_FOR_UTF8[*src_begin] + 1;
    if (src_begin + len > src_end) {
      return false;
    }

    return IsLegalUtf8(src_begin, len);
  }

  static bool IsLegalUtf8(const UTF8CHAR* src, int32 len) {
    UTF8CHAR byte;
    const UTF8CHAR* last_ptr = src + len;
    switch (len) {
      default:
        return false;
        // Everything else falls through when "true"...
      case 4:
        if ((byte = (*--last_ptr)) < 0x80 || byte > 0xBF) {
          return false;
        }  // else fallthrough
      case 3:
        if ((byte = (*--last_ptr)) < 0x80 || byte > 0xBF) {
          return false;
        }  // else fallthrough
      case 2:
        if ((byte = (*--last_ptr)) > 0xBF) {
          return false;
        }

        switch (uint8(*src)) {
          // no fall-through in this inner switch
          case 0xE0:
            if (byte < 0xA0) return false;
            break;
          case 0xED:
            if (byte > 0x9F) return false;
            break;
          case 0xF0:
            if (byte < 0x90) return false;
            break;
          case 0xF4:
            if (byte > 0x8F) return false;
            break;
          default:
            if (byte < 0x80) return false;
            break;
        }

      case 1:
        if (*src >= 0x80 && *src < 0xC2) {
          return false;
        }
        break;
    }

    if (*src > 0xF4) {
      return false;
    }

    return true;
  }

  //
  // Helper functions
  //

  static FUN_ALWAYS_INLINE bool IsNonCharacter(uint32 ucs4) {
    return ucs4 >= 0xFDD0 && (ucs4 <= 0xFDEF || (ucs4 & 0xFFFE) == 0xFFFE);
  }

  static FUN_ALWAYS_INLINE bool IsHighSurrogate(uint32 ucs4) {
    return ((ucs4 & 0xFFFFFC00) == 0xD800);
  }

  static FUN_ALWAYS_INLINE bool IsLowSurrogate(uint32 ucs4) {
    return ((ucs4 & 0xFFFFFC00) == 0xDC00);
  }

  static FUN_ALWAYS_INLINE bool IsSurrogate(uint32 ucs4) {
    return (ucs4 - 0xD800U < 2048U);
  }
};

// TODO string_conversion.h 파일쪽으로 옮겨줄까??

//
// UtfConversionBuffer
//

template <typename _FromType, typename _ToType,
          int32 DefaultConversionLength = 500>
class UtfConversionBuffer
    : private InlineAllocator<DefaultConversionLength>::template ForElementType<
          _ToType> {
 public:
  using FromType = _FromType;
  using ToType = _ToType;

  using AllocatorType = typename InlineAllocator<
      DefaultConversionLength>::template ForElementType<ToType>;

  explicit UtfConversionBuffer(const FromType* src) {
    if (src) {
      Init(src, Utf::Strlen(src));
    } else {
      converted_ = nullptr;
      converted_length_ = 0;
    }
  }

  UtfConversionBuffer(const FromType* src, int32 src_len) {
    if (src) {
      Init(src, src_len);
    } else {
      converted_ = nullptr;
      converted_length_ = 0;
    }
  }

  UtfConversionBuffer(UtfConversionBuffer&& rhs) {
    AllocatorType::MoveToEmpty(rhs);
  }

  operator const ToType*() const { return converted_; }
  const ToType* ConstData() const { return converted_; }
  int32 Len() const { return converted_length_; }
  bool IsEmpty() const { return converted_length_ == 0; }

  Array<ToType> ToArrayView() const {
    return ArrayView<ToType>(converted_, converted_length_);
  }

  // Disable copy
  UtfConversionBuffer(const UtfConversionBuffer&) = delete;
  UtfConversionBuffer& operator=(const UtfConversionBuffer&) = delete;

 private:
  ToType* converted_;
  int32 converted_length_;

  template <typename ToType2>
  int32 LengthAsToType(
      const FromType* src, int32 src_len,
      typename EnableIf<IsUtf8Char<ToType2>::Value, bool>::Type = true) {
    return Utf::LengthAsUtf8(src, src_len);
  }

  template <typename ToType2>
  int32 LengthAsToType(
      const FromType* src, int32 src_len,
      typename EnableIf<IsUtf16Char<ToType2>::Value, bool>::Type = true) {
    return Utf::LengthAsUtf16(src, src_len);
  }

  template <typename ToType2>
  int32 LengthAsToType(
      const FromType* src, int32 src_len,
      typename EnableIf<IsUtf32Char<ToType2>::Value, bool>::Type = true) {
    return Utf::LengthAsUtf32(src, src_len);
  }

  void Init(const FromType* src, int32 src_len) {
    converted_length_ = LengthAsToType<ToType>(src, src_len);
    AllocatorType::ResizeAllocation(0, converted_length_ + 1, sizeof(ToType));
    converted_ = (ToType*)AllocatorType::GetAllocation();
    Utf::Convert(src, src_len, converted_, converted_length_);
    converted_[converted_length_] = '\0';  // 안전을 위해서 NUL-TERM
  }
};

}  // namespace fun
