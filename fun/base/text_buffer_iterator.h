#pragma once

#include "fun/base/base.h"
#include <cstdlib>

namespace fun {

class TextEncoding;

/**
 * An unidirectional iterator for iterating over characters in a buffer.
 * The TextBufferIterator uses a TextEncoding object to
 * work with multi-byte character encodings like UTF-8.
 * Characters are reported in unicode.
 *
 * Example: Count the number of UTF-8 characters in a buffer.
 *
 *   Utf8Encoding utf8Encoding;
 *   char buffer[] = "...";
 *   TextBufferIterator it(buffer, utf8Encoding);
 *   TextBufferIterator end(it.end());
 *   int n = 0;
 *   while (it != end) { ++n; ++it; }
 *
 * NOTE: When an UTF-16 encoding is used, surrogate pairs will be
 * reported as two separate characters, due to restrictions of
 * the TextEncoding class.
 *
 * For iterating over the characters in a std::string, see the
 * TextIterator class.
 */
class FUN_BASE_API TextBufferIterator {
 public:
  /**
   * Creates an uninitialized TextBufferIterator.
   */
  TextBufferIterator();

  /**
   * Creates a TextBufferIterator for the given buffer, which must be 0-terminated.
   * The encoding object must not be deleted as long as the iterator
   * is in use.
   */
  TextBufferIterator(const char* begin, const TextEncoding& encoding);

  /**
   * Creates a TextBufferIterator for the given buffer with the given size.
   * The encoding object must not be deleted as long as the iterator
   * is in use.
   */
  TextBufferIterator(const char* begin, size_t length, const TextEncoding& encoding);

  /**
   * Creates a TextBufferIterator for the given range.
   * The encoding object must not be deleted as long as the iterator
   * is in use.
   */
  TextBufferIterator(const char* begin, const char* end, const TextEncoding& encoding);

  /**
   * Creates an end TextBufferIterator for the given buffer.
   */
  TextBufferIterator(const char* end);

  /**
   * Destroys the TextBufferIterator.
   */
  ~TextBufferIterator();

  /**
   * Copy constructor.
   */
  TextBufferIterator(const TextBufferIterator& it);

  /**
   * Assignment operator.
   */
  TextBufferIterator& operator = (const TextBufferIterator& it);

  /**
   * Swaps the iterator with another one.
   */
  void Swap(TextBufferIterator& it);

  /**
   * Returns the Unicode value of the current character.
   * If there is no valid character at the current position,
   * -1 is returned.
   */
  int operator * () const;

  /**
   * Prefix increment operator.
   */
  TextBufferIterator& operator ++ ();

  /**
   * Postfix increment operator.
   */
  TextBufferIterator operator ++ (int);

  /**
   * Compares two iterators for equality.
   */
  bool operator == (const TextBufferIterator& it) const;

  /**
   * Compares two iterators for inequality.
   */
  bool operator != (const TextBufferIterator& it) const;

  /**
   * Returns the end iterator for the range handled
   * by the iterator.
   */
  TextBufferIterator end() const;

 private:
  const TextEncoding* encoding_;

  const char* current_;
  const char* end_;
};


//
// inlines
//

FUN_ALWAYS_INLINE bool TextBufferIterator::operator == (const TextBufferIterator& it) const {
  return current_ == it.current_;
}

FUN_ALWAYS_INLINE bool TextBufferIterator::operator != (const TextBufferIterator& it) const {
  return current_ != it.current_;
}

FUN_ALWAYS_INLINE void Swap(TextBufferIterator& it1, TextBufferIterator& it2) {
  it1.Swap(it2);
}

FUN_ALWAYS_INLINE TextBufferIterator TextBufferIterator::end() const {
  return TextBufferIterator(end_);
}

} // namespace fun
