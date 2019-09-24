#pragma once

#include "fun/base/base.h"
#include "fun/base/flags.h"  // enum flags

#include <ctype.h>
#include <wctype.h>

namespace fun {

/** Determines case sensitivity options for UString comparisons. */
enum class CaseSensitivity : uint8 {
  /** Case sensitive. Upper/lower casing must match for strings to be considered
     equal. */
  CaseSensitive,

  /** Ignore case. Upper/lower casing does not matter when making a comparison.
   */
  IgnoreCase,
};

/** Option flags related to string splitting */
enum class StringSplitOption {
  None = 0x00,
  Trimming = 0x01,
  CullEmpty = 0x02,
  TrimmingAndCullEmpty = Trimming | CullEmpty,
};
FUN_DECLARE_FLAGS(StringSplitOptions, StringSplitOption);

/** TODO */
enum class NulTermination {
  No = 0,
  Yes = 1,
};

template <typename T>
struct SelectLiteral {
  static const char Select(const char ansi, const UNICHAR uni) { return ansi; }
  static const char* Select(const char* ansi, const UNICHAR* uni) {
    return ansi;
  }
};

template <>
struct SelectLiteral<UNICHAR> {
  static const UNICHAR Select(const char ansi, const UNICHAR uni) {
    return uni;
  }
  static const UNICHAR* Select(const char* ansi, const UNICHAR* uni) {
    return uni;
  }
};

#define LITERAL(CharType, StringLiteral) \
  SelectLiteral<CharType>::Select(StringLiteral, UTEXT(StringLiteral))

template <typename T, size_t size>
struct CharTraitsBase {
  typedef T CharType;

  static const CharType LineFeed = UTEXT('\x000A');
  static const CharType VerticalTab = UTEXT('\x000B');
  static const CharType FormFeed = UTEXT('\x000C');
  static const CharType CarriageReturn = UTEXT('\x000D');
  static const CharType NextLine = UTEXT('\x0085');
  static const CharType LineSeparator = UTEXT('\x2028');
  static const CharType ParagraphSeparator = UTEXT('\x2029');
};

template <typename T>
struct CharTraitsBase<T, 1> {
  typedef T CharType;

  static const CharType LineFeed = '\x000A';
  static const CharType VerticalTab = '\x000B';
  static const CharType FormFeed = '\x000C';
  static const CharType CarriageReturn = '\x000D';
  static const CharType NextLine = '\x0085';
};

template <typename T, size_t size>
struct LineBreakChecker {
  typedef T CharType;

  static FUN_ALWAYS_INLINE bool IsLineBreak(CharType ch) {
    return ch == CharTraitsBase<CharType, size>::LineFeed ||
           ch == CharTraitsBase<CharType, size>::VerticalTab ||
           ch == CharTraitsBase<CharType, size>::FormFeed ||
           ch == CharTraitsBase<CharType, size>::CarriageReturn ||
           ch == CharTraitsBase<CharType, size>::NextLine ||
           ch == CharTraitsBase<CharType, size>::LineSeparator ||
           ch == CharTraitsBase<CharType, size>::ParagraphSeparator;
  }
};

template <typename T>
struct LineBreakChecker<T, 1> {
  typedef T CharType;

  static FUN_ALWAYS_INLINE bool IsLineBreak(CharType ch) {
    return ch == CharTraitsBase<CharType, 1>::LineFeed ||
           ch == CharTraitsBase<CharType, 1>::VerticalTab ||
           ch == CharTraitsBase<CharType, 1>::FormFeed ||
           ch == CharTraitsBase<CharType, 1>::CarriageReturn ||
           ch == CharTraitsBase<CharType, 1>::NextLine;
  }
};

template <typename T>
struct CharTraits : public CharTraitsBase<T, sizeof(T)> {
  typedef T CharType;

  static CharType ToUpper(CharType ch);
  static CharType ToLower(CharType ch);
  static bool IsUpper(CharType ch);
  static bool IsLower(CharType ch);
  static bool IsAlpha(CharType ch);
  static bool IsPunct(CharType ch);
  static bool IsAlnum(CharType ch);
  static bool IsDigit(CharType ch);
  static bool IsHexDigit(CharType ch);
  static bool IsWhitespace(CharType ch);

  static FUN_ALWAYS_INLINE bool IsIdentifier(CharType ch) {
    return IsAlnum(ch) || IsUnderscore(ch);
  }

  static FUN_ALWAYS_INLINE bool IsUnderscore(CharType ch) {
    return ch == LITERAL(CharType, '_');
  }

  static FUN_ALWAYS_INLINE bool IsLineBreak(CharType ch) {
    return LineBreakChecker<CharType, sizeof(CharType)>::IsLineBreak(ch);
  }

  static FUN_ALWAYS_INLINE CharType FoldCase(CharType ch) {
    // if (ch >= 'a' && ch <= 'z') {
    //  return 'A' + (ch - 'a');
    //}
    // return ch;
    return IsLower(ch) ? ToUpper(ch) : ch;
  }

  /**
   * Convert digit character to integer value.
   */
  static FUN_ALWAYS_INLINE int32 DigitCharToInt(CharType ch, int32 def = -1) {
    return (ch >= '0' && ch <= '9') ? (ch - '0') : def;
  }

  /**
   * Convert integer value to digit character.
   */
  static FUN_ALWAYS_INLINE CharType IntToDigitChar(int32 n,
                                                   CharType def = '?') {
    return (n >= 0 && n <= 9) ? (CharType)('0' + n) : def;
  }

  /**
   * Convert hex character to nibble value.
   */
  static FUN_ALWAYS_INLINE int32 HexCharToNibble(CharType ch, int32 def = -1) {
    if (ch >= '0' && ch <= '9') {
      return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
      return (ch - 'a') + 10;
    } else if (ch >= 'A' && ch <= 'F') {
      return (ch - 'A') + 10;
    } else {
      return def;
    }
  }

  /**
   * Convert integer nibble value to hex character.
   */
  static FUN_ALWAYS_INLINE CharType NibbleToHexChar(uint8 n,
                                                    CharType def = '?') {
    return (n < 16) ? ('a' + n) : def;
  }
};

typedef CharTraits<char> CharTraitsA;
typedef CharTraits<UNICHAR> CharTraitsU;

//
// inlines
//

//
// UNICHAR specialized functions
//

template <>
FUN_ALWAYS_INLINE CharTraits<UNICHAR>::CharType CharTraits<UNICHAR>::ToUpper(
    CharType ch) {
  return ::towupper(ch);
}
template <>
FUN_ALWAYS_INLINE CharTraits<UNICHAR>::CharType CharTraits<UNICHAR>::ToLower(
    CharType ch) {
  return ::towlower(ch);
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsUpper(CharType ch) {
  return ::iswupper(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsLower(CharType ch) {
  return ::iswlower(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsAlpha(CharType ch) {
  return ::iswalpha(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsPunct(CharType ch) {
  return ::iswpunct(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsAlnum(CharType ch) {
  return ::iswalnum(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsDigit(CharType ch) {
  return ::iswdigit(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsHexDigit(CharType ch) {
  return ::iswxdigit(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<UNICHAR>::IsWhitespace(CharType ch) {
  return ::iswspace(ch) != 0;
}

//
// char specialized functions
//

template <>
FUN_ALWAYS_INLINE CharTraits<char>::CharType CharTraits<char>::ToUpper(
    CharType ch) {
  return ::toupper(ch);
}
template <>
FUN_ALWAYS_INLINE CharTraits<char>::CharType CharTraits<char>::ToLower(
    CharType ch) {
  return ::tolower(ch);
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsUpper(CharType ch) {
  return ::isupper(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsLower(CharType ch) {
  return ::islower(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsAlpha(CharType ch) {
  return ::isalpha(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsPunct(CharType ch) {
  return ::ispunct(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsAlnum(CharType ch) {
  return ::isalnum(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsDigit(CharType ch) {
  return ::isdigit(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsHexDigit(CharType ch) {
  return ::isxdigit(ch) != 0;
}
template <>
FUN_ALWAYS_INLINE bool CharTraits<char>::IsWhitespace(CharType ch) {
  return ::isspace(ch) != 0;
}

}  // namespace fun
