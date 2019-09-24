#pragma once

#include "fun/base/base.h"

namespace fun {

class TextEncoding;

/**
 * A TextConverter converts strings from one encoding into another.
 */
class FUN_BASE_API TextConverter {
 public:
  /**
   * Transform function for convert.
   */
  typedef int (*Transform)(int);

  /**
   * Creates the TextConverter. The encoding objects must not be deleted while the
   * TextConverter is in use.
   */
  TextConverter(const TextEncoding& input_encoding,
                const TextEncoding& output_encoding,
                int bogus_char ='?');

  /**
   * Destroys the TextConverter.
   */
  ~TextConverter();

  /**
   * Converts the source string from inEncoding to outEncoding
   * and appends the result to destination. Every character is
   * passed to the transform function.
   * If a character cannot be represented in outEncoding, defaultChar
   * is used instead.
   * Returns the number of encoding errors (invalid byte sequences
   * in source).
   */
  int Convert(const String& src, String& dst, Transform xform);

  /**
   * Converts the source buffer from inEncoding to outEncoding
   * and appends the result to destination. Every character is
   * passed to the transform function.
   * If a character cannot be represented in outEncoding, defaultChar
   * is used instead.
   * Returns the number of encoding errors (invalid byte sequences
   * in source).
   */
  int Convert(const void* src, int length, String& dst, Transform xform);

  /**
   * Converts the source string from inEncoding to outEncoding
   * and appends the result to destination.
   * If a character cannot be represented in outEncoding, defaultChar
   * is used instead.
   * Returns the number of encoding errors (invalid byte sequences
   * in source).
   */
  int Convert(const String& src, String& dst);

  /**
   * Converts the source buffer from inEncoding to outEncoding
   * and appends the result to destination.
   * If a character cannot be represented in outEncoding, defaultChar
   * is used instead.
   * Returns the number of encoding errors (invalid byte sequences
   * in source).
   */
  int Convert(const void* src, int length, String& dst);

  // Disable default constructor and copy.
  TextConverter() = delete;
  TextConverter(const TextConverter&) = delete;
  TextConverter& operator = (const TextConverter&) = delete;

 private:
  const TextEncoding& input_encoding_;
  const TextEncoding& output_encoding_;
  int bogus_char_;
};

} // namespace fun
