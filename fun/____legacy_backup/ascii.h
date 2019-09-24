#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * This class contains enumerations and static
 * utility functions for dealing with ASCII characters
 * and their properties.
 * 
 * The classification functions will also work if
 * non-ASCII character codes are passed to them,
 * but classification will only check for
 * ASCII characters.
 * 
 * This allows the classification methods to be used
 * on the single bytes of a UTF-8 string, without
 * causing assertions or inconsistent results (depending
 * upon the current locale) on bytes outside the ASCII range,
 * as may be produced by Ascii::IsSpace(), etc.
 */
class FUN_BASE_API Ascii
{
 public:
  /** ASCII character properties. */
  enum CharacterProperties {
    ACP_CONTROL  = 0x0001,
    ACP_SPACE    = 0x0002,
    ACP_PUNCT    = 0x0004,
    ACP_DIGIT    = 0x0008,
    ACP_HEXDIGIT = 0x0010,
    ACP_ALPHA    = 0x0020,
    ACP_LOWER    = 0x0040,
    ACP_UPPER    = 0x0080,
    ACP_GRAPH    = 0x0100,
    ACP_PRINT    = 0x0200
  };

  /**
   * Return the ASCII character properties for the
   * character with the given ASCII value.
   * 
   * If the character is outside the ASCII range
   * (0 .. 127), 0 is returned.
   */
  static int GetProperties(int ch);

  /**
   * Returns true if the given character is
   * within the ASCII range and has at least one of
   * the given properties.
   */
  static bool HasAnyProperties(int ch, int props);

  /**
   * Returns true if the given character is
   * within the ASCII range and has all of
   * the given properties.
   */
  static bool HasAllProperties(int ch, int props);

  /**
   * Returns true if the given character code is within
   * the ASCII range (0 .. 127).
   */
  static bool IsAscii(int ch);

  /**
   * Returns true if the given character is a whitespace.
   */
  static bool IsSpace(int ch);

  /**
   * Returns true if the given character is a digit.
   */
  static bool IsDigit(int ch);

  /**
   * Returns true if the given character is a hexadecimal digit.
   */
  static bool IsHexDigit(int ch);

  /**
   * Returns true if the given character is a hexadecimal digit.
   */
  static bool IsPunct(int ch);

  /**
   * Returns true if the given character is an alphabetic character.
   */
  static bool IsAlpha(int ch);

  /**
   * Returns true if the given character is an alphabetic or numeric character.
   */
  static bool IsAlphaNumeric(int ch);

  /**
   * Returns true if the given character is a lowercase alphabetic
   * character.
   */
  static bool IsLower(int ch);

  /**
   * Returns true if the given character is an uppercase alphabetic
   * character.
   */
  static bool IsUpper(int ch);

  /**
   * Returns true if the given character is printable.
   */
  static bool IsPrintable(int ch);

  /**
   * If the given character is an uppercase character,
   * return its lowercase counterpart, otherwise return
   * the character.
   */
  static int ToLower(int ch);

  /**
   * If the given character is a lowercase character,
   * return its uppercase counterpart, otherwise return
   * the character.
   */
  static int ToUpper(int ch);

 private:
  static const int properties_[128];
};



//
// inlines
//

inline int Ascii::GetProperties(int ch)
{
  return IsAscii(ch) ? properties_[ch] : 0;
}

inline bool Ascii::IsAscii(int ch)
{
  return (static_cast<uint32>(ch) & 0xFFFFFF80) == 0;
}

inline bool Ascii::HasAllProperties(int ch, int props)
{
  return (GetProperties(ch) & props) == props;
}

inline bool Ascii::HasAnyProperties(int ch, int props)
{
  return (GetProperties(ch) & props) != 0;
}

inline bool Ascii::IsSpace(int ch)
{
  return HasAllProperties(ch, ACP_SPACE);
}

inline bool Ascii::IsDigit(int ch)
{
  return HasAllProperties(ch, ACP_DIGIT);
}

inline bool Ascii::IsHexDigit(int ch)
{
  return HasAllProperties(ch, ACP_HEXDIGIT);
}

inline bool Ascii::IsPunct(int ch)
{
  return HasAllProperties(ch, ACP_PUNCT);
}

inline bool Ascii::IsAlpha(int ch)
{
  return HasAllProperties(ch, ACP_ALPHA);
}

inline bool Ascii::IsAlphaNumeric(int ch)
{
  return HasAnyProperties(ch, ACP_ALPHA | ACP_DIGIT);
}

inline bool Ascii::IsLower(int ch)
{
  return HasAllProperties(ch, ACP_LOWER);
}

inline bool Ascii::IsUpper(int ch)
{
  return HasAllProperties(ch, ACP_UPPER);
}

inline bool Ascii::IsPrintable(int ch)
{
  return HasAllProperties(ch, ACP_PRINT);
}

inline int Ascii::ToLower(int ch)
{
  return IsUpper(ch) ? ch + 32 : ch;
}

inline int Ascii::ToUpper(int ch)
{
  return IsLower(ch) ? ch - 32 : ch;
}

} // namespace fun
