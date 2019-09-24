#include "fun/base/text_iterator.h"
#include "fun/base/encoding/text_encoding.h"
#include <algorithm>

namespace fun {

TextIterator::TextIterator() : encoding_(nullptr) {}

TextIterator::TextIterator(const String& str, const TextEncoding& encoding)
  : encoding_(&encoding), current_(str.begin()), end_(str.end()) {}

TextIterator::TextIterator( const String::ConstIterator& begin,
                            const String::ConstIterator& end,
                            const TextEncoding& encoding)
  : encoding_(&encoding), current_(begin), end_(end) {}

// 이 생성자는 그냥 끝(end())을 나타내는 용도로 사용됨.
TextIterator::TextIterator(const String& str)
  : encoding_(0),
  // current_(str.begin()),
    current_(str.end()), //note: end가 맞음...
    end_(str.end()) {
}

TextIterator::TextIterator(const String::ConstIterator& end)
  : encoding_(nullptr),
    current_(end),
    end_(end) {}

TextIterator::~TextIterator() {}

TextIterator::TextIterator(const TextIterator& it)
  : encoding_(it.encoding_),
    current_(it.current_),
    end_(it.end_) {}

TextIterator& TextIterator::operator = (const TextIterator& it) {
  if (FUN_LIKELY(&it != this)) {
    encoding_ = it.encoding_;
    current_ = it.current_;
    end_ = it.end_;
  }

  return *this;
}

void TextIterator::Swap(TextIterator& it) {
  fun::Swap(encoding_, it.encoding_);
  fun::Swap(current_, it.current_);
  fun::Swap(end_, it.end_);
}

int TextIterator::operator * () const {
  fun_check_ptr(encoding_);
  fun_check(current_ != end_);
  String::ConstIterator it = current_;

  uint8 buffer[TextEncoding::MAX_SEQUENCE_LENGTH];
  uint8* p = buffer;

  if (it != end_) {
    *p++ = *it++;
  } else {
    *p++ = 0;
  }

  int read = 1;
  int n = encoding_->QueryConvert(buffer, 1);

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

TextIterator& TextIterator::operator ++ () {
  fun_check_ptr(encoding_);
  fun_check(current_ != end_);

  uint8 buffer[TextEncoding::MAX_SEQUENCE_LENGTH];
  uint8* p = buffer;

  if (current_ != end_) {
    *p++ = *current_++;
  } else {
    *p++ = 0;
  }

  int read = 1;
  int n = encoding_->SequenceLength(buffer, 1);

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

TextIterator TextIterator::operator ++ (int) {
  TextIterator prev(*this);
  operator ++ ();
  return prev;
}

} // namespace fun
