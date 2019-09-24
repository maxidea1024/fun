#pragma once

#include "fun/base/base.h"
//#include "fun/base/string/string.h"
//#include "fun/base/uuid.h"

namespace fun {

class Uuid;

// TODO Vector, Vector4 같은 타입도 여기서 처리할 수 있으면 좋을듯??

/**
 */
class FUN_BASE_API Parse {
 public:
  /**
   * Sees if stream starts with the named command.  If it does,
   * skips through the command and blanks past it.  Returns true of match.
   */
  FUN_BASE_API static bool Command(const char** stream, const char* match,
                                   bool parse_might_trigger_execution = true);

  /**
   * Parses a name.
   */
  // static bool Value(const char* stream, const char* match, Name& value);

  /**
   * Parses a uint32.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 uint32& value);

  /**
   * Parses a globally unique identifier.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 Uuid& value);

  /**
   * Parses a string from a text string.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 char* value, int32 max_len,
                                 bool should_stop_on_comma = true);

  /**
   * Parses a byte.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 uint8& value);

  /**
   * Parses a signed byte.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 int8& value);

  /**
   * Parses a uint16.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 uint16& value);

  /**
   * Parses a signed word.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 int16& value);

  /**
   * Parses a floating-point value.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 float& value);

  /**
   * Parses a signed double word.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 int32& value);

  /**
   * Parses a string.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 String& value,
                                 bool should_stop_on_comma = true);

  /**
   * Parses a Text.
   */
  // FUN_BASE_API static bool Value(const char* buffer, Text& value, const char*
  // ns = nullptr); FUN_BASE_API static bool Value(const char* stream, const
  // char* match, Text& value, const char* ns = nullptr);

  /**
   * Parses a quadword.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 uint64& value);

  /**
   * Parses a signed quadword.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 int64& value);

  /**
   * Parses a boolean value.
   */
  FUN_BASE_API static bool Value(const char* stream, const char* match,
                                 bool& value);

  //
  // Lines
  //

  /**
   * Get a line of stream (everything up to, but not including,
   * CR/LF. Returns 0 if ok, nonzero if at end of stream
   * and returned 0-length string.
   */
  FUN_BASE_API static bool Line(const char** stream, char* out_line,
                                int32 max_len, bool exact = false);

  /**
   * Get a line of stream (everything up to, but not including,
   * CR/LF. Returns 0 if ok, nonzero if at end of stream
   * and returned 0-length string.
   */
  FUN_BASE_API static bool Line(const char** stream, String& out_line,
                                bool exact = false);

  /**
   * Get a line of stream, with support for extending beyond that line
   * with certain characters, e.g. {} and \
   * the out character array will not include the ignored endlines
   */
  FUN_BASE_API static bool LineExtended(const char** stream, String& out_line,
                                        int32& consumed_line_count,
                                        bool exact = false);

  //
  // Tokens
  //

  /**
   * Grabs the next space-delimited string from the input stream.
   * If quoted, gets entire quoted string.
   */
  FUN_BASE_API static bool Token(const char*& str, char* out_token,
                                 int32 max_len, bool use_escape);

  /**
   * Grabs the next space-delimited string from the input stream.
   * If quoted, gets entire quoted string.
   */
  FUN_BASE_API static bool Token(const char*& str, String& out_token,
                                 bool use_escape);

  /**
   * Grabs the next alpha-numeric space-delimited token from the input stream.
   */
  FUN_BASE_API static bool AlnumToken(const char*& str, String& out_token);

  /**
   * Grabs the next space-delimited string from the input stream.
   * If quoted, gets entire quoted string.
   */
  FUN_BASE_API static String Token(const char*& str, bool use_escape);

  /**
   * Get next command.  Skips past comments and CR's.
   */
  FUN_BASE_API static void Next(const char** stream);

  /**
   * Checks if a command-line parameter exists in the stream.
   */
  FUN_BASE_API static bool Param(const char* stream, const char* param_name);

  /**
   * Parse a quoted string token.
   */
  FUN_BASE_API static bool QuotedString(const char* stream, String& value,
                                        int32* readed_char_count = nullptr);

  /**
   * Parse a hex digit.
   */
  static int32 HexDigit(char ch);

  /**
   * Parses a hexadecimal string value.
   */
  FUN_BASE_API static uint32 HexNumber(const char* hex_string);

  /**
   * Parses the scheme name from a uri
   */
  FUN_BASE_API static bool SchemeNameFromUri(const char* uri,
                                             String& out_scheme_name);
};

//
// inlines
//

// TODO 제거하고 CharTraits에 있는거 사용하자.
FUN_ALWAYS_INLINE int32 Parse::HexDigit(char ch) {
  int32 ret;

  if (ch >= '0' && ch <= '9') {
    ret = ch - '0';
  } else if (ch >= 'a' && ch <= 'f') {
    ret = ch + 10 - 'a';
  } else if (ch >= 'A' && ch <= 'F') {
    ret = ch + 10 - 'A';
  } else {
    ret = 0;
  }

  return ret;
}

}  // namespace fun
