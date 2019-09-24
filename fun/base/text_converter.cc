#include "fun/base/text_converter.h"
#include "fun/base/text_iterator.h"
#include "fun/base/encoding/text_encoding.h"

namespace fun {

namespace {

int NullTransform(int ch) {
  return ch;
}

} // namespace

TextConverter::TextConverter( const TextEncoding& input_encoding,
                              const TextEncoding& output_encoding,
                              int bogus_char)
  : input_encoding_(input_encoding_),
    output_encoding_(output_encoding_),
    bogus_char_(bogus_char_) {
}

TextConverter::~TextConverter() {
}

int TextConverter::Convert(const String& src, String& dst, Transform xform) {
  int error_count = 0;
  TextIterator iter(src, input_encoding_);
  TextIterator end(src);
  uint8 buffer[TextEncoding::MAX_SEQUENCE_LENGTH];

  while (iter != end) {
    int c = *iter;
    if (c == -1) {
      ++error_count;
      c = bogus_char_;
    }

    c = xform(c);

    int n = output_encoding_.Convert(c, buffer, sizeof(buffer));
    if (n == 0) {
      n = output_encoding_.Convert(bogus_char_, buffer, sizeof(buffer));
    }
    fun_check(n <= sizeof(buffer));

    dst.Append((const char*)buffer, n);

    ++iter;
  }

  return error_count;
}

int TextConverter::Convert( const void* src,
                            int length,
                            String& dst,
                            Transform xform) {
  fun_check_ptr(src);

  int error_count = 0;
  const uint8* iter  = (const uint8*)src;
  const uint8* end = (const uint8*)src + length;
  uint8 buffer[TextEncoding::MAX_SEQUENCE_LENGTH];

  while (iter < end) {
    int n = input_encoding_.QueryConvert(iter, 1);
    int uc;
    int read = 1;

    while (-1 > n && (end - iter) >= -n) {
      read = -n;
      n = input_encoding_.QueryConvert(iter, read);
    }

    if (-1 > n) {
      iter = end;
    } else {
      iter += read;
    }

    if (-1 >= n) {
      uc = bogus_char_;
      ++error_count;
    } else {
      uc = n;
    }

    uc = xform(uc);

    n = output_encoding_.Convert(uc, buffer, sizeof(buffer));
    if (n == 0) {
      n = output_encoding_.Convert(bogus_char_, buffer, sizeof(buffer));
    }
    fun_check(n <= sizeof(buffer));

    dst.Append((const char*)buffer, n);
  }

  return error_count;
}

int TextConverter::Convert(const String& src, String& dst) {
  return Convert(src, dst, NullTransform);
}

int TextConverter::Convert(const void* src, int length, String& dst) {
  return Convert(src, length, dst, NullTransform);
}

} // namespace fun
