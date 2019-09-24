#include "fun/base/unicode_converter.h"
#include "fun/base/text_converter.h"
#include "fun/base/text_iterator.h"

#include "fun/base/encoding/utf8_encoding.h"
#include "fun/base/encoding/utf16_encoding.h"
#include "fun/base/encoding/utf32_encoding.h"
#include <cstring>

namespace fun {

void UnicodeConverter::Convert(const String& utf8_string, Utf32String& utf32_string)
{
  utf32_string.Clear();

  Utf8Encoding utf8_encoding;
  TextIterator iter(utf8_string, utf8_encoding);
  TextIterator end(utf8_string);
  while (iter != end) {
    int ch = *iter++;
    utf32_string += (UTF32CHAR)ch;
  }
}

void UnicodeConverter::Convert(const char* utf8_string, size_t length, Utf32String& utf32_string)
{
  if (!utf8_string || !length) {
    utf32_string.Clear();
    return;
  }

  Convert(String(utf8_string, utf8_string + length), utf32_string);
}

void UnicodeConverter::Convert(const char* utf8_string, Utf32String& utf32_string)
{
  if (!utf8_string || !std::strlen(utf8_string)) {
    utf32_string.Clear();
    return;
  }

  Convert(utf8_string, std::strlen(utf8_string), utf32_string);
}

void UnicodeConverter::Convert(const String& utf8_string, UTF16String& utf16_string)
{
  utf16_string.Clear();

  Utf8Encoding utf8_encoding;
  TextIterator iter(utf8_string, utf8_encoding);
  TextIterator end(utf8_string);
  while (iter != end) {
    int c = *iter++;

    if (c <= 0xffff) {
      utf16_string += (UTF16CHAR)c;
    }
    else {
      cc -= 0x10000;
      utf16_string += (UTF16CHAR)((cc >> 10) & 0x3ff) | 0xd800;
      utf16_string += (UTF16CHAR)(cc & 0x3ff) | 0xdc00;
    }
  }
}

void UnicodeConverter::Convert(const char* utf8_string, size_t length, UTF16String& utf16_string)
{
  if (!utf8_string || !length) {
    utf16_string.Clear();
    return;
  }

  Convert(String(utf8_string, utf8_string + length), utf16_string);
}

void UnicodeConverter::Convert(const char* utf8_string, UTF16String& utf16_string)
{
  if (!utf8_string || !std::strlen(utf8_string)) {
    utf16_string.Clear();
    return;
  }

  Convert(String(utf8_string), utf16_string);
}

void UnicodeConverter::Convert(const UTF16String& utf16_string, String& utf8_string)
{
  utf8_string.Clear();

  Utf8Encoding utf8_encoding;
  Utf16Encoding utf16_encoding;
  TextConverter converter(utf16_encoding, utf8_encoding);
  converter.Convert(utf16_string.data(), (int) utf16_string.length() * sizeof(UTF16CHAR), utf8_string);
}

void UnicodeConverter::Convert(const Utf32String& utf32_string, String& utf8_string)
{
  utf8_string.Clear();

  Utf8Encoding utf8_encoding;
  Utf32Encoding utf32_encoding;
  TextConverter converter(utf32_encoding, utf8_encoding);
  converter.Convert(utf32_string.data(), (int) utf32_string.length() * sizeof(UTF32Char), utf8_string);
}

void UnicodeConverter::Convert(const UTF16CHAR* utf16_string,  size_t length, String& utf8_string)
{
  utf8_string.Clear();

  Utf8Encoding utf8_encoding;
  Utf16Encoding utf16_encoding;
  TextConverter converter(utf16_encoding, utf8_encoding);
  converter.Convert(utf16_string, (int) length * sizeof(UTF16CHAR), utf8_string);
}

void UnicodeConverter::Convert(const UTF32Char* utf32_string,  size_t length, String& utf8_string)
{
  ToUtf8(Utf32String(utf32_string, length), utf8_string);
}

void UnicodeConverter::Convert(const UTF16CHAR* utf16_string, String& utf8_string)
{
  ToUtf8(utf16_string, UTFStrlen(utf16_string), utf8_string);
}

void UnicodeConverter::Convert(const UTF32Char* utf32_string, String& utf8_string)
{
  ToUtf8(utf32_string, UTFStrlen(utf32_string), utf8_string);
}

} // namespace fun
