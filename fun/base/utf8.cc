#include "fun/base/utf8.h"
#include "fun/base/encoding/utf8_encoding.h"
#include "fun/base/string/string.h"
#include "fun/base/text_converter.h"
#include "fun/base/text_iterator.h"

#include <algorithm>

namespace fun {

namespace {
static Utf8Encoding utf8_encoding;
}  // namespace

int32 Utf8::icompare(const String& str, int32 pos, int32 len,
                     String::ConstIterator it2, String::ConstIterator end2) {
  int32 sz = str.Len();
  if (pos > sz) {
    pos = sz;
  }
  if (pos + len > sz) {
    len = sz - pos;
  }

  TextIterator uit1(str.begin() + pos, str.begin() + pos + len, utf8_encoding);
  TextIterator uend1(str.begin() + pos + len);
  TextIterator uit2(it2, end2, utf8_encoding);
  TextIterator uend2(end2);
  while (uit1 != uend1 && uit2 != uend2) {
    const int32 c1 = CharTraitsU::ToLower(*uit1);
    const int32 c2 = CharTraitsU::ToLower(*uit2);

    if (c1 < c2) {
      return -1;
    } else if (c1 > c2) {
      return 1;
    }

    ++uit1;
    ++uit2;
  }

  if (uit1 == uend1) {
    return uit2 == uend2 ? 0 : -1;
  } else {
    return 1;
  }
}

int32 Utf8::icompare(const String& str1, const String& str2) {
  return icompare(str1, 0, str1.Len(), str2.begin(), str2.end());
}

int32 Utf8::icompare(const String& str1, int32 len1, const String& str2,
                     int32 len2) {
  if (len2 > str2.Len()) {
    len2 = str2.Len();
  }
  return icompare(str1, 0, len1, str2.begin(), str2.begin() + len2);
}

int32 Utf8::icompare(const String& str1, int32 len, const String& str2) {
  if (len > str2.Len()) len = str2.Len();
  return icompare(str1, 0, len, str2.begin(), str2.begin() + len);
}

int32 Utf8::icompare(const String& str1, int32 pos, int32 len,
                     const String& str2) {
  return icompare(str1, pos, len, str2.begin(), str2.end());
}

int32 Utf8::icompare(const String& str1, int32 pos1, int32 len1,
                     const String& str2, int32 pos2, int32 len2) {
  int32 sz2 = str2.Len();
  if (pos2 > sz2) {
    pos2 = sz2;
  }
  if (pos2 + len2 > sz2) {
    len2 = sz2 - pos2;
  }
  return icompare(str1, pos1, len1, str2.begin() + pos2,
                  str2.begin() + pos2 + len2);
}

int32 Utf8::icompare(const String& str1, int32 pos1, int32 len,
                     const String& str2, int32 pos2) {
  int32 sz2 = str2.Len();
  if (pos2 > sz2) {
    pos2 = sz2;
  }
  if (pos2 + len > sz2) {
    len = sz2 - pos2;
  }
  return icompare(str1, pos1, len, str2.begin() + pos2,
                  str2.begin() + pos2 + len);
}

int32 Utf8::icompare(const String& str, int32 pos, int32 len,
                     const String::CharType* ptr) {
  fun_check_ptr(ptr);
  String str2(ptr);  // TODO: optimize
  return icompare(str, pos, len, str2.begin(), str2.end());
}

int32 Utf8::icompare(const String& str, int32 pos,
                     const String::CharType* ptr) {
  return icompare(str, pos, str.Len() - pos, ptr);
}

int32 Utf8::icompare(const String& str, const String::CharType* ptr) {
  return icompare(str, 0, str.Len(), ptr);
}

String Utf8::ToUpper(const String& str) {
  String result;
  TextConverter converter(utf8_encoding, utf8_encoding);
  converter.Convert(str, result, ::toupper);
  return result;
}

String& Utf8::ToUpperInPlace(String& str) {
  String result;
  TextConverter converter(utf8_encoding, utf8_encoding);
  converter.Convert(str, result, ::toupper);
  fun::Swap(str, result);
  return str;
}

String Utf8::ToLower(const String& str) {
  String result;
  TextConverter converter(utf8_encoding, utf8_encoding);
  converter.Convert(str, result, ::tolower);
  return result;
}

String& Utf8::ToLowerInPlace(String& str) {
  String result;
  TextConverter converter(utf8_encoding, utf8_encoding);
  converter.Convert(str, result, ::tolower);
  fun::Swap(str, result);
  return str;
}

void Utf8::RemoveBOM(String& str) {
  if (str.Len() >= 3 && static_cast<uint8>(str[0]) == 0xEF &&
      static_cast<uint8>(str[1]) == 0xBB &&
      static_cast<uint8>(str[2]) == 0xBF) {
    str.Remove(0, 3);
  }
}

String Utf8::Escape(const String& str, bool strict_json) {
  return Escape(str.begin(), str.end(), strict_json);
}

String Utf8::Escape(const String::ConstIterator& begin,
                    const String::ConstIterator& end, bool strict_json) {
  static const uint32 OFFSETS_FROM_UTF8[6] = {0x00000000UL, 0x00003080UL,
                                              0x000E2080UL, 0x03C82080UL,
                                              0xFA082080UL, 0x82082080UL};

  String result;

  String::ConstIterator it = begin;

  while (it != end) {
    uint32 ch = 0;
    uint32 sz = 0;

    do {
      ch <<= 6;
      ch += (uint8)*it++;
      sz++;
    } while (it != end && (*it & 0xC0) == 0x80 && sz < 6);

    ch -= OFFSETS_FROM_UTF8[sz - 1];

    if (ch == '\n') {
      result += "\\n";
    } else if (ch == '\t') {
      result += "\\t";
    } else if (ch == '\r') {
      result += "\\r";
    } else if (ch == '\b') {
      result += "\\b";
    } else if (ch == '\f') {
      result += "\\f";
    } else if (ch == '\v') {
      result += (strict_json ? "\\u000B" : "\\v");
    } else if (ch == '\a') {
      result += (strict_json ? "\\u0007" : "\\a");
    } else if (ch == '\\') {
      result += "\\\\";
    } else if (ch == '\"') {
      result += "\\\"";
    } else if (ch == '/') {
      result += "\\/";
    } else if (ch == '\0') {
      result += "\\u0000";
    } else if (ch < 32 || ch == 0x7f) {
      result += "\\u";
      // TODO
      // NumberFormatter::AppendHex(result, (unsigned short)ch, 4);
    } else if (ch > 0xFFFF) {
      ch -= 0x10000;
      result += "\\u";
      // TODO
      // NumberFormatter::AppendHex(result, (unsigned short)((ch >> 10) &
      // 0x03ff) + 0xd800, 4);
      result += "\\u";
      // TODO
      // NumberFormatter::AppendHex(result, (unsigned short)(ch & 0x03ff ) +
      // 0xdc00, 4);
    } else if (ch >= 0x80 && ch <= 0xFFFF) {
      result += "\\u";
      // TODO
      // NumberFormatter::AppendHex(result, (unsigned short)ch, 4);
    } else {
      result += (char)ch;
    }
  }
  return result;
}

String Utf8::Unescape(const String& str) {
  return Unescape(str.begin(), str.end());
}

String Utf8::Unescape(const String::ConstIterator& begin,
                      const String::ConstIterator& end) {
  String result;

  String::ConstIterator it = begin;

  while (it != end) {
    uint32 ch = (uint32)*it++;

    if (ch == '\\') {
      if (it == end) {
        // Invalid sequence!
      }

      if (*it == 'n') {
        ch = '\n';
        it++;
      } else if (*it == 't') {
        ch = '\t';
        it++;
      } else if (*it == 'r') {
        ch = '\r';
        it++;
      } else if (*it == 'b') {
        ch = '\b';
        it++;
      } else if (*it == 'f') {
        ch = '\f';
        it++;
      } else if (*it == 'v') {
        ch = '\v';
        it++;
      } else if (*it == 'a') {
        ch = '\a';
        it++;
      } else if (*it == 'u') {
        char digs[5];
        UnsafeMemory::Memset(digs, 0, 5);
        uint32 dno = 0;

        it++;

        while (it != end && CharTraitsA::IsHexDigit(*it) && dno < 4) {
          digs[dno++] = *it++;
        }
        if (dno > 0) {
          ch = strtol(digs, NULL, 16);
        }

        if (ch >= 0xD800 && ch <= 0xDBFF) {
          if (it == end || *it != '\\') {
            // Invalid sequence!
          } else {
            it++;
            if (it == end || *it != 'u') {
              // Invalid sequence!
            } else {
              it++;
            }
          }

          // UTF-16 surrogate pair. Go fetch other half
          UnsafeMemory::Memset(digs, 0, 5);
          dno = 0;
          while (it != end && CharTraitsA::IsHexDigit(*it) && dno < 4) {
            digs[dno++] = *it++;
          }
          if (dno > 0) {
            uint32 tmp = strtol(digs, NULL, 16);
            if (tmp >= 0xDC00 && tmp <= 0xDFFF) {
              ch = (((ch - 0xD800) << 10) | (tmp - 0xDC00)) + 0x10000;
            }
          }
        }
      } else if (*it == 'U') {
        char digs[9];
        UnsafeMemory::Memset(digs, 0, 9);
        uint32 dno = 0;

        it++;
        while (it != end && CharTraitsA::IsHexDigit(*it) && dno < 8) {
          digs[dno++] = *it++;
        }
        if (dno > 0) {
          ch = strtol(digs, NULL, 16);
        }
      }
    }

    uint8 utf8_code[4];
    int32 code_len = utf8_encoding.Convert(ch, utf8_code, 4);
    result.Append((char*)utf8_code, code_len);
  }

  return result;
}

}  // namespace fun
