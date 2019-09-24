#pragma once

#include "fun/base/container/array.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
 *
 * \code
 * +-------------------+------------------+------------------+
 * | prependable bytes |  readable bytes  |  writable bytes  |
 * |                   |     (CONTENT)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0      <=      readerIndex   <=   writerIndex    <=     size
 * \endcode
 *
 * https://docs.jboss.org/netty/3.2/api/org/jboss/netty/buffer/ChannelBuffer.html
 */
class FUN_NET_API Buffer {
 public:
  // 왜 버퍼 앞에 8바이트의 쓰지 않는 공간을 만들어두는 걸까???
  // 앞쪽에 길이 같은것을 추가하기 용이하게 하기 위해서임!
  static const size_t DISCARDABLE_PREPEND_LENGTH = 8;
  static const size_t INITIAL_BUFFER_SIZE = 1024;

  explicit Buffer(size_t initial_size = INITIAL_BUFFER_SIZE)
      : buffer_(DISCARDABLE_PREPEND_LENGTH + initial_size),
        reader_index_(DISCARDABLE_PREPEND_LENGTH),
        writer_index_(DISCARDABLE_PREPEND_LENGTH) {
    fun_check(ReadableLength() == 0);
    fun_check(WritableLength() == 0);
    fun_check(PrependableLength() == DISCARDABLE_PREPEND_LENGTH);
  }

  void Swap(Buffer& other) {
    buffer_.Swap(other.buffer_);

    fun::Swap(reader_index_, other.reader_index_);
    fun::Swap(writer_index_, other.writer_index_);
  }

  size_t ReadableLength() const { return writer_index_ - reader_index_; }

  size_t WritableLength() const { return buffer_.Count() - writer_index_; }

  size_t PrependableLength() const { return reader_index_; }

  const char* ReadablePtr() const { return begin() + reader_index_; }

  const char* FindCRLF() const { return Find("\r\n", 2); }

  const char* FindCRLF(const char* start) const {
    return Find(start, "\r\n", 2);
  }

  const char* FindEOL() const { return Find('\n'); }

  const char* FindEOL(const char* start) const { return Find(start, '\n'); }

  const char* Find(char c) const {
    const void* found = memchr(ReadablePtr(), c, ReadableLength());
    return static_cast<const char*>(found);
  }

  const char* Find(const char* sub, size_t len) const {
    const char* found =
        std::search(ReadablePtr(), WritablePtr(), sub, sub + len);
    return found = WritablePtr() ? nullptr : found;
  }

  const char* Find(const char* start, char c) const {
    const void* found = memchr(ReadablePtr(), c, ReadableLength());
    return static_cast<const char*>(found);
  }

  const char* Find(const char* start, const char* sub, size_t len) const {
    fun_check(ReadablePtr() <= start);
    fun_check(start <= WritablePtr());
    const char* found = std::search(start, WritablePtr(), sub, sub + len);
    return found = WritablePtr() ? nullptr : found;
  }

  void Drain(size_t len) {
    fun_check(len <= ReadableLength());

    if (len < ReadableLength()) {
      reader_index_ += len;
    } else {
      DrainAll();
    }
  }

  void DrainUntil(const char* end) {
    fun_check(ReadablePtr() <= end);
    fun_check(end <= WritablePtr());

    Drain(end - ReadablePtr());
  }

  void DrainAll() {
    reader_index_ = DISCARDABLE_PREPEND_LENGTH;
    writer_index_ = DISCARDABLE_PREPEND_LENGTH;
  }

  String ReadAllAsString() { return ReadAsString(ReadableLength()); }

  String ReadAsString(size_t len) {
    fun_check(len <= ReadableLength());
    String ret(ReadableLength(), len);
    Drain(len);
    return ret;
  }

  StringView ToStringView() const {
    return StringView(ReadablePtr(), ReadableLength());
  }

  void Append(const StringView& str) { Append(str.ConstData(), str.Len()); }

  void Append(const char* data, size_t len) {
    EnsureWritableSpace(len);
    UnsafeMemory::Memcpy(WritablePtr(), data, len);
    AdvanceWritten(len);
  }

  void Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
  }

  void EnsureWritableSpace(size_t len) {
    if (WritableLength() < len) {
      MakeSpace(len);
    }

    fun_check(WritableLength() >= len);
  }

  char* WritablePtr() { return begin() + writer_index_; }

  const char* WritablePtr() const { return begin() + writer_index_; }

  void AdvanceWritten(size_t len) {
    fun_check(len <= WritableLength());

    writer_index_ += len;
  }

  void Unwrite(size_t len) {
    fun_check(len <= ReadableLength());

    writer_index_ -= len;
  }

  void Prepend(const void* data, size_t len) {
    fun_check(len <= PrependableLength());

    reader_index_ -= len;
    UnsafeMemory::Memcpy(begin() + reader_index_, data, len);  // memmove?
  }

  void Shrink(size_t reserve) {
    Buffer other;
    other.EnsureWritableSpace(ReadableLength() + reserve);
    other.Append(ToStringView());
    Swap(other);
  }

  size_t GetInternalCapacity() const { return buffer_.Capacity(); }

  // NOTE readv를 위한 특수한 상황에서만 사용됨.
  ssize_t ReadFd(int fd, int* saved_errno);

 private:
  char* begin() { return buffer_.MutableData(); }

  const char* begin() const { return buffer_.ConstData(); }

  // 쓸수 있는 공간이 부족한 경우에만 호출됨.
  void MakeSpace(size_t len) {
    if (WritableLength() + PrependabeLength() <
        len + DISCARDABLE_PREPEND_LENGTH) {
      buffer_.ResizeUninitialized(writer_index_ + len);
    } else {
      fun_check(DISCARDABLE_PREPEND_LENGTH < reader_index_);
      size_t readable_len = ReadableLength();
      std::copy(begin() + reader_index_, begin() + writer_index_,
                begin() + DISCARDABLE_PREPEND_LENGTH);
      reader_index_ = DISCARDABLE_PREPEND_LENGTH;
      writer_index_ = reader_index_ + readable_len;
      fun_check(readable_len == ReadableLength());
    }
  }

 private:
  Array<char> buffer_;
  size_t reader_index_;
  size_t writer_index_;
};

}  // namespace net
}  // namespace fun
