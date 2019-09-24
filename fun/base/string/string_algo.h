// TODO join, internal 몽땅 이쪽으로 몰자...

#pragma once

#include "fun/base/base.h"
#include "fun/base/string/cstring_traits.h"

namespace fun {

/**
 * ByteString / UString 모두 사용가능함.
 */
template <typename StringType>
struct StringAlgo {
  typedef typename StringType::CharType CharType;
  typedef typename RemoveCV<StringType>::Type NakedStringType;
  // const bool IsConst = IsConst<StringType>::Value;
  static const bool IsConst = std::is_const<CharType>::value;

  // Trimming helpers

  static inline int32 LeftSpaces(const CharType* begin, const CharType* end) {
    const CharType* original_begin = begin;
    while (begin != end && CharTraits<CharType>::IsWhitespace(*begin)) ++begin;
    return begin - original_begin;
  }

  static inline int32 RightSpaces(const CharType* begin, const CharType* end) {
    const CharType* original_end = end;
    while (begin != end && CharTraits<CharType>::IsWhitespace(end[-1])) --end;
    return original_end - end;
  }

  static inline int32 SideSpaces(const CharType* begin, const CharType* end) {
    const CharType* original_begin = begin;
    const CharType* original_end = end;
    while (begin != end && CharTraits<CharType>::IsWhitespace(*begin)) ++begin;
    while (begin != end && CharTraits<CharType>::IsWhitespace(end[-1])) --end;
    return (original_end - original_begin) - (end - begin);
  }

  static inline int32 TrimmedPositions(const CharType*& begin,
                                       const CharType*& end) {
    while (begin != end && CharTraits<CharType>::IsWhitespace(end[-1])) --end;
    while (begin != end && CharTraits<CharType>::IsWhitespace(*begin)) ++begin;
    return end - begin;
  }

  static inline int32 LeftTrimmedPositions(const CharType*& begin,
                                           const CharType*& end) {
    while (begin != end && CharTraits<CharType>::IsWhitespace(*begin)) ++begin;
    return end - begin;
  }

  static inline int32 RightTrimmedPositions(const CharType*& begin,
                                            const CharType*& end) {
    while (begin != end && CharTraits<CharType>::IsWhitespace(end[-1])) --end;
    return end - begin;
  }

  static inline StringType TrimmedInplace(NakedStringType& str,
                                          const CharType* begin,
                                          const CharType* end) {
    CharType* data = const_cast<CharType*>(str.cbegin());
    if (begin != data) {
      UnsafeMemory::Memmove(data, begin, (end - begin) * sizeof(CharType));
    }
    str.Resize(end - begin);
    return MoveTemp(str);
  }

  static inline StringType TrimmedInplace(const NakedStringType& str,
                                          const CharType* begin,
                                          const CharType* end) {
    // unrechable
    return StringType();
  }

  static inline StringType Trimmed(StringType& str) {
    const CharType* begin = str.cbegin();
    const CharType* end = str.cend();
    TrimmedPositions(begin, end);

    if (begin == str.cbegin() &&
        end == str.cend()) {  // If there is no conversion, the input value is
                              // returned.
      return str;
    }

    if (!IsConst &&
        str.IsDetached()) {  // If it is not const and is not being shared, it
                             // will be modified and returned.
      return TrimmedInplace(str, begin, end);
    }

    // Trimmed substring
    return StringType(begin, end - begin);
  }

  // Left Trimmed
  static inline StringType LeftTrimmed(StringType& str) {
    const CharType* begin = str.cbegin();
    const CharType* end = str.cend();
    LeftTrimmedPositions(begin, end);

    if (begin == str.cbegin() &&
        end == str.cend()) {  // If there is no conversion, the input value is
                              // returned.
      return str;
    }

    if (!IsConst &&
        str.IsDetached()) {  // If it is not const and is not being shared, it
                             // will be modified and returned.
      return TrimmedInplace(str, begin, end);
    }

    // Trimmed substring
    return StringType(begin, end - begin);
  }

  // Right trimmed
  static inline StringType RightTrimmed(StringType& str) {
    const CharType* begin = str.cbegin();
    const CharType* end = str.cend();
    RightTrimmedPositions(begin, end);

    if (begin == str.cbegin() &&
        end == str.cend()) {  // If there is no conversion, the input value is
                              // returned.
      return str;
    }

    if (!IsConst &&
        str.IsDetached()) {  // If it is not const and is not being shared, it
                             // will be modified and returned.
      return TrimmedInplace(str, begin, end);
    }

    // Trimmed substring
    return StringType(begin, end - begin);
  }

  // Simplifying helpers

  static inline StringType Simplified(StringType& str) {
    if (str.IsEmpty()) {
      return str;
    }

    const CharType* src = str.cbegin();
    const CharType* end = str.cend();
    NakedStringType result =
        IsConst || !str.IsDetached() ? StringType(str.Len(), NoInit) : str;

    CharType* dst = const_cast<CharType*>(result.cbegin());
    CharType* ptr = dst;
    bool unmodified = true;
    for (;;) {
      while (src != end && CharTraits<CharType>::IsWhitespace(*src)) ++src;
      while (src != end && !CharTraits<CharType>::IsWhitespace(*src))
        *ptr++ = *src++;

      if (src == end) {
        break;
      }

      if (*src != LITERAL(CharType, ' ')) {
        unmodified = false;
      }

      *ptr++ = LITERAL(CharType, ' ');
    }

    if (ptr != dst && ptr[-1] == LITERAL(CharType, ' ')) {
      --ptr;
    }

    const int32 new_len = ptr - dst;
    if (IsConst && new_len == str.Len() && unmodified) {
      // nothing happened, return the original
      return str;
    }

    result.ResizeUninitialized(new_len);
    return result;
  }
};

}  // namespace fun
