#pragma once

#include "fun/base/base.h"
#include "fun/base/string/string.h"

namespace fun {

/**
 * This class provides static methods that are UTF-8 capable variants
 * of the same functions in red/core/str.h.
 *
 * The various variants of icompare() provide case insensitive comparison
 * for UTF-8 encoded strings.
 *
 * ToUpper(), ToUpperInPlace(), ToLower() and ToLowerInPlace() provide
 * Unicode-based character case transformation for UTF-8 encoded strings.
 *
 * RemoveBOM() removes the UTF-8 Byte Order Mark sequence (0xEF, 0xBB, 0xBF)
 * from the beginning of the given String, if it's there.
 */
class FUN_BASE_API Utf8 {
 public:
  static int32 icompare(const String& str, int32 pos, int32 len,
                        String::ConstIterator it2, String::ConstIterator end2);
  static int32 icompare(const String& str1, const String& str2);
  static int32 icompare(const String& str1, int32 len1, const String& str2,
                        int32 len2);
  static int32 icompare(const String& str1, int32 len, const String& str2);
  static int32 icompare(const String& str1, int32 pos, int32 len,
                        const String& str2);
  static int32 icompare(const String& str1, int32 pos1, int32 len1,
                        const String& str2, int32 pos2, int32 len2);
  static int32 icompare(const String& str1, int32 pos1, int32 len,
                        const String& str2, int32 pos2);
  static int32 icompare(const String& str, int32 pos, int32 len,
                        const String::CharType* ptr);
  static int32 icompare(const String& str, int32 pos,
                        const String::CharType* ptr);
  static int32 icompare(const String& str, const String::CharType* ptr);

  static String ToUpper(const String& str);
  static String& ToUpperInPlace(String& str);
  static String ToLower(const String& str);
  static String& ToLowerInPlace(String& str);

  /**
   * Remove the UTF-8 Byte Order Mark sequence (0xEF, 0xBB, 0xBF)
   * from the beginning of the String, if it's there.
   */
  static void RemoveBOM(String& str);

  /**
   * Escapes a String. Special characters like tab, backslash, ... are
   * escaped. Unicode characters are escaped to \uxxxx.
   * If strict_json is true, \a and \v will be escaped to \\u0007 and \\u000B
   * instead of \\a and \\v for strict JSON conformance.
   */
  static String Escape(const String& str, bool strict_json = false);

  /**
   * Escapes a String. Special characters like tab, backslash, ... are
   * escaped. Unicode characters are escaped to \uxxxx.
   * If strict_json is true, \a and \v will be escaped to \\u0007 and \\u000B
   * instead of \\a and \\v for strict JSON conformance.
   */
  static String Escape(const String::ConstIterator& begin,
                       const String::ConstIterator& end,
                       bool strict_json = false);

  /**
   * Creates an Utf8 String from a String that contains escaped characters.
   */
  static String Unescape(const String& str);

  /**
   * Creates an Utf8 String from a String that contains escaped characters.
   */
  static String Unescape(const String::ConstIterator& begin,
                         const String::ConstIterator& end);
};

}  // namespace fun
