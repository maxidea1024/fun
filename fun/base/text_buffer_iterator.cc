#include "fun/base/text_buffer_iterator.h"
#include "fun/base/encoding/text_encoding.h"

namespace fun {

TextBufferIterator::TextBufferIterator()
  : encoding_(nullptr), current_(nullptr), end_(nullptr) {}

TextBufferIterator::TextBufferIterator( const char* begin,
                                        const TextEncoding& encoding)
  : encoding_(&encoding), current_(begin), end_(begin + std::strlen(begin)) {}

TextBufferIterator::TextBufferIterator( const char* begin,
                                        size_t length,
                                        const TextEncoding& encoding)
  : encoding_(&encoding), current_(begin), end_(begin + length) {}

TextBufferIterator::TextBufferIterator( const char* begin,
                                        const char* end,
                                        const TextEncoding& encoding)
  : encoding_(&encoding), current_(begin), end_(end) {}

TextBufferIterator::TextBufferIterator(const char* end)
  : encoding_(nullptr), current_(end), end_(end) {}

TextBufferIterator::~TextBufferIterator() {}

TextBufferIterator::TextBufferIterator(const TextBufferIterator& it)
  : encoding_(it.encoding_), current_(it.current_), end_(it.end_) {}

TextBufferIterator&
TextBufferIterator::operator = (const TextBufferIterator& it) {
  if (FUN_LIKELY(&it != this)) {
    encoding_ = it.encoding_;
    current_ = it.current_;
    end_ = it.end_;
  }

  return *this;
}

void TextBufferIterator::Swap(TextBufferIterator& it) {
  fun::Swap(encoding_, it.encoding_);
  fun::Swap(current_, it.current_);
  fun::Swap(end_, it.end_);
}

int TextBufferIterator::operator * () const {
  fun_check_ptr(encoding_);
  fun_check(current_ != end_);
  const char* it = current_;

  uint8 buffer[TextEncoding::MAX_SEQUENCE_LENGTH];
  uint8* p = buffer;

  if (it != end_) {
    *p++ = *it++;
  } else {
    *p++ = 0;
  }

  int32 read = 1;
  int32 n = encoding_->QueryConvert(buffer, 1);

  while (-1 > n && (end_ - it) >= -n - read) {
    while (read < -n && it != end_) {
      *p++ = *it++;
      read++;
    }

    n = encoding_->QueryConvert(buffer, read);
  }

  if (-1 > n) {
    return -1;
  } else {
    return n;
  }
}

TextBufferIterator& TextBufferIterator::operator ++ () {
  fun_check_ptr(encoding_);
  fun_check(current_ != end_);

  uint8 buffer[TextEncoding::MAX_SEQUENCE_LENGTH];
  uint8* p = buffer;

  if (current_ != end_) {
    *p++ = *current_++;
  } else {
    *p++ = 0;
  }

  int32 read = 1;
  int32 n = encoding_->SequenceLength(buffer, 1);

  while (-1 > n && (end_ - current_) >= -n - read) {
    while (read < -n && current_ != end_) {
      *p++ = *current_++;
      read++;
    }

    n = encoding_->SequenceLength(buffer, read);
  }

  while (read < n && current_ != end_) {
    current_++;
    read++;
  }

  return *this;
}

TextBufferIterator TextBufferIterator::operator ++ (int) {
  TextBufferIterator prev(*this);
  operator ++ ();
  return prev;
}

} // namespace fun
