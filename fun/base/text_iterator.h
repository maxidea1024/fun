#pragma once

#include "fun/base/base.h"
#include "fun/base/string/string.h"

namespace fun {

class TextEncoding;

/**
 * An unidirectional iterator for iterating over characters in a string.
 * The TextIterator uses a TextEncoding object to
 * work with multi-byte character encodings like UTF-8.
 * Characters are reported in unicode.
 *
 * Example: Count the number of UTF-8 characters in a string.
 *
 *   Utf8Encoding utf8_encoding;
 *   String utf8_string("....");
 *   TextIterator it(utf8_string, utf8_encoding);
 *   TextIterator end(utf8_string);
 *   int n = 0;
 *   while (it != end) { ++n; ++it; }
 *
 * NOTE: When an UTF-16 encoding is used, surrogate pairs will be
 * reported as two separate characters, due to restrictions of
 * the TextEncoding class.
 *
 * For iterating over char buffers, see the TextBufferIterator class.
 */
class FUN_BASE_API TextIterator {
 public:
  /**
   * Creates an uninitialized TextIterator.
   */
  TextIterator();

  /**
   * Creates a TextIterator for the given string.
   * The encoding object must not be deleted as long as the iterator
   * is in use.
   */
  TextIterator(const String& str, const TextEncoding& encoding);

  /**
   * Creates a TextIterator for the given range.
   * The encoding object must not be deleted as long as the iterator
   * is in use.
   */
  TextIterator(const String::ConstIterator& begin,
               const String::ConstIterator& end, const TextEncoding& encoding);

  /**
   * Creates an end TextIterator for the given string.
   */
  TextIterator(const String& str);

  /**
   * Creates an end TextIterator.
   */
  TextIterator(const String::ConstIterator& end);

  /**
   * Destroys the TextIterator.
   */
  ~TextIterator();

  /**
   * Copy constructor.
   */
  TextIterator(const TextIterator& it);

  /**
   * Assignment operator.
   */
  TextIterator& operator=(const TextIterator& it);

  /**
   * Swaps the iterator with another one.
   */
  void Swap(TextIterator& it);

  /**
   * Returns the Unicode value of the current character.
   * If there is no valid character at the current position,
   * -1 is returned.
   */
  int operator*() const;

  /**
   * Prefix increment operator.
   */
  TextIterator& operator++();

  /**
   * Postfix increment operator.
   */
  TextIterator operator++(int);

  /**
   * Compares two iterators for equality.
   */
  bool operator==(const TextIterator& it) const;

  /**
   * Compares two iterators for inequality.
   */
  bool operator!=(const TextIterator& it) const;

  /**
   * Returns the end iterator for the range handled
   * by the iterator.
   */
  TextIterator end() const;

 private:
  const TextEncoding* encoding_;
  String::ConstIterator current_;
  String::ConstIterator end_;
};

//
// inlines
//

FUN_ALWAYS_INLINE bool TextIterator::operator==(const TextIterator& it) const {
  return current_ == it.current_;
}

FUN_ALWAYS_INLINE bool TextIterator::operator!=(const TextIterator& it) const {
  return current_ != it.current_;
}

FUN_ALWAYS_INLINE void Swap(TextIterator& it1, TextIterator& it2) {
  it1.Swap(it2);
}

FUN_ALWAYS_INLINE TextIterator TextIterator::end() const {
  return TextIterator(end_);
}

}  // namespace fun
