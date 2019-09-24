#pragma once

#include "fun/base.h"

namespace fun {

enum JsonOptions {
  /**
   * Applies to JSON::Object. If specified, the Object will
   * preserve the items insertion order. Otherwise, items
   * will be sorted by keys.
   *
   * Has no effect on ToJson() function.
   */
  JSON_PRESERVE_KEY_ORDER = 1,

  /**
   * If specified, when the object is stringified, all
   * unicode characters will be escaped in the resulting
   * string.
   */
  JSON_ESCAPE_UNICODE = 2,

  /**
   * If specified, the object will preserve the items
   * insertion order. Otherwise, items will be sorted
   * by keys.
   */
  JSON_WRAP_STRINGS = 4
};

//@ deprecated
/**
 * Formats string value into the supplied output stream by
 * escaping control and ALL Unicode characters.
 * If wrap is true, the resulting string is enclosed in double quotes.
 *
 * This function is deprecated, please use
 *
 * void fun::ToJson(const String&, std::ostream&, int)
 */
void FUN_BASE_API ToJson(const String& value, std::ostream& out, bool wrap = true);

//@ deprecated
/**
 * Formats string value by escaping control and ALL Unicode characters.
 * If wrap is true, the resulting string is enclosed in double quotes
 *
 * Returns formatted string.
 *
 * This function is deprecated, please use
 *
 * String fun::ToJson(const String&, int)
 */
String FUN_BASE_API ToJson(const String& value, bool wrap = true);

/**
 * Formats string value into the supplied output stream by
 * escaping control characters.
 * If JSON_WRAP_STRINGS is in options, the resulting strings is enclosed in double quotes
 * If JSON_ESCAPE_UNICODE is in options, all unicode characters will be escaped, otherwise
 * only the compulsory ones.
 */
void FUN_BASE_API ToJson(const String& value, std::ostream& out, int options);

/**
 * Formats string value by escaping control characters.
 * If JSON_WRAP_STRINGS is in options, the resulting string is enclosed in double quotes
 * If JSON_ESCAPE_UNICODE is in options, all unicode characters will be escaped, otherwise
 * only the compulsory ones.
 *
 * Returns formatted string.
 * If escapeAllUnicode is true, all unicode characters will be escaped, otherwise only the compulsory ones.
 */
String FUN_BASE_API ToJson(const String& value, int options);

} // namespace fun
