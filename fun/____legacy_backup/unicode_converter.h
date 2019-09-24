#pragma once

#include "fun/base/base.h"

#if !defined(FUN_NO_WSTRING)

#include "fun/base/utf_string.h"

namespace fun {

/**
 * A convenience class that converts strings from
 * UTF-8 encoded std::strings to UTF-16 or UTF-32 encoded std::wstrings
 * and vice-versa.
 *
 * This class is mainly used for working with the unicode Windows APIs
 * and probably won't be of much use anywhere else ???
 */
class FUN_BASE_API UnicodeConverter
{
 public:
  /**
   * Converts the given UTF-8 encoded string into an UTF-32 encoded wide string.
   */
  static void Convert(const String& utf8_string, Utf32String& utf32_string);

  /**
   * Converts the given UTF-8 encoded character sequence into an UTF-32 encoded wide string.
   */
  static void Convert(const char* utf8_string, size_t length, Utf32String& utf32_string);

  /**
   * Converts the given zero-terminated UTF-8 encoded character sequence into an UTF-32 encoded wide string.
   */
  static void Convert(const char* utf8_string, Utf32String& utf32_string);

  /**
   * Converts the given UTF-8 encoded string into an UTF-16 encoded wide string.
   */
  static void Convert(const String& utf8_string, UTF16String& utf16_string);

  /**
   * Converts the given UTF-8 encoded character sequence into an UTF-16 encoded wide string.
   */
  static void Convert(const char* utf8_string, size_t length, UTF16String& utf16_string);

  /**
   * Converts the given zero-terminated UTF-8 encoded character sequence into an UTF-16 encoded wide string.
   */
  static void Convert(const char* utf8_string, UTF16String& utf16_string);

  /**
   * Converts the given UTF-16 encoded wide string into an UTF-8 encoded string.
   */
  static void Convert(const UTF16String& utf16_string, String& utf8_string);

  /**
   * Converts the given UTF-32 encoded wide string into an UTF-8 encoded string.
   */
  static void Convert(const Utf32String& utf32_string, String& utf8_string);

  /**
   * Converts the given zero-terminated UTF-16 encoded wide character sequence into an UTF-8 encoded string.
   */
  static void Convert(const UTF16CHAR* utf16_string,  size_t length, String& utf8_string);

  /**
   * Converts the given zero-terminated UTF-32 encoded wide character sequence into an UTF-8 encoded string.
   */
  static void Convert(const UTF32Char* utf16_string, size_t length, String& utf8_string);

  /**
   * Converts the given UTF-16 encoded zero terminated character sequence into an UTF-8 encoded string.
   */
  static void Convert(const UTF16CHAR* utf16_string, String& utf8_string);

  /**
   * Converts the given UTF-32 encoded zero terminated character sequence into an UTF-8 encoded string.
   */
  static void Convert(const UTF32Char* utf32_string, String& utf8_string);

  template <typename FromTy, typename ToTy>
  static void ToUtf32(const FromTy& from, ToTy& to)
  {
    Convert(from, to);
  }

  template <typename FromTy, typename ToTy>
  static void ToUtf32(const FromTy& from, size_t length, ToTy& to)
  {
    Convert(from, length, to);
  }

  template <typename FromTy, typename ToTy>
  static void ToUtf16(const FromTy& from, ToTy& to)
  {
    Convert(from, to);
  }

  template <typename FromTy, typename ToTy>
  static void ToUtf16(const FromTy& from, size_t length, ToTy& to)
  {
    Convert(from, length, to);
  }

  template <typename FromTy, typename ToTy>
  static void ToUtf8(const FromTy& from, ToTy& to)
  {
    Convert(from, to);
  }

  template <typename FromTy, typename ToTy>
  static void ToUtf8(const FromTy& from, size_t length, ToTy& to)
  {
    Convert(from, length, to);
  }

  template <typename ToTy>
  static ToTy ConvertTo(const char* chars)
  {
    ToTy utf_str;
    UnicodeConverter::Convert(chars, utf_str);
    return utf_str;
  }

  template <typename ToTy>
  static ToTy ConvertTo(const String& str)
  {
    ToTy utf_str;
    UnicodeConverter::Convert(str, utf_str);
    return utf_str;
  }

  /**
   * Returns the length (in characters) of a zero-terminated UTF string.
   */
  template <typename T>
  static size_t UTFStrlen(const T* ptr)
  {
    if (ptr == nullptr) {
      return 0;
    }

    const T* p;
    for (p = ptr; *p; ++p) ;
    return p - ptr;
  }
};

} // namespace fun

#endif // !defined(FUN_NO_WSTRING)
