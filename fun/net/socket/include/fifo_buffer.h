#pragma once

#include "TBuffer.h"

namespace fun {

/**
A simple buffer class with support for re-entrant,
FIFO-style read/write operations, as well as (optional)
empty/non-empty/full (i.e. writable/readable) transition
notifications. Buffer can be flagged with end-of-file and
error flags, which renders it un-readable/writable.

Critical portions of code are protected by a recursive mutex.
However, to achieve thread-safety in cases where multiple
member function calls are involved and have to be atomic,
the mutex must be locked externally.

Buffer size, as well as amount of unread data and
available space introspections are supported as well.

This class is useful anywhere where a FIFO functionality
is needed.
*/
template <typename T>
class TFIFOBuffer : public Noncopyable {
 public:
  typedef T Type;

  /**
  Creates the FIFOBuffer.
  */
  TFIFOBuffer(int32 buffer_length)
    : internal_buffer_(buffer_length),
      position_(0),
      used_length_(0),
      eof_(false),
      error_(false) {}

  /**
  Creates the FIFOBuffer.
  */
  TFIFOBuffer(T* buffer, int32 buffer_length)
    : internal_buffer_(buffer, buffer_length),
      position_(0),
      used_length_(0),
      eof_(false),
      error_(false) {}

  /**
  Creates the FIFOBuffer.
  */
  TFIFOBuffer(const T* InBuffer, int32 buffer_length)
    : internal_buffer_(InBuffer, buffer_length),
      position_(0),
      used_length_(0),
      eof_(false),
      error_(false) {}

  ~TFIFOBuffer() {}

  /**
   * Resizes the buffer. If preserve_content is true,
   * the content of the old buffer is preserved.
   * New size can be larger or smaller than
   * the current size, but it must not be 0.
   * Additionally, if the new length is smaller
   * than currently used length and preserve_content
   * is true, InvalidAccessException is thrown.
   */
  void Resize(int32 new_length, bool preserve_content = true) {
    ScopedLock guard(mutex_);

    if (preserve_content && new_length < used_length_) {
      throw InvalidAccessException(CStringLiteral("can not resize FIFO without data loss."));
    }

    internal_buffer_.Resize(new_length, preserve_content);

    if (!preserve_content) {
      used_length_ = 0;
    }
  }

  /**
  Peeks into the data currently in the FIFO
  without actually extracting it.
  If length is zero, the return is immediate.
  If length is greater than used length,
  it is substituted with the the current FIFO
  used length.

  Returns the number of elements copied in the
  supplied buffer.
  */
  int32 Peek(T* out_buffer, int32 length) const {
    if (length <= 0) {
      return 0;
    }

    ScopedLock guard(mutex_);

    if (!IsReadable()) {
      return 0;
    }

    if (length > used_length_) {
      length = used_length_;
    }

    UnsafeMemory::Memcpy(out_buffer, internal_buffer_.Begin() + position_, length * sizeof(T));

    return length;
  }

  /**
   * Peeks into the data currently in the FIFO
   * without actually extracting it.
   * Resizes the supplied buffer to the size of
   * data written to it. If length is not
   * supplied by the caller or is greater than length
   * of currently used data, the current FIFO used
   * data length is substituted for it.
   * 
   * Returns the number of elements copied in the
   * supplied buffer.
   */
  int32 Peek(TBuffer<T>& out_buffer, int32 length = 0) const {
    ScopedLock guard(mutex_);

    if (!IsReadable()) {
      return 0;
    }

    if (0 == length || length > used_length_) {
      length = used_length_;
    }

    out_buffer.Resize(length);

    return Peek(out_buffer.Begin(), length);
  }

  /**
   * Copies the data currently in the FIFO
   * into the supplied buffer, which must be
   * preallocated to at least the length size
   * before calling this function.
   * 
   * Returns the size of the copied data.
   */
  int32 Read(T* out_buffer, int32 length) {
    if (0 == length) {
      return 0;
    }

    ScopedLock guard(mutex_);

    if (!IsReadable()) {
      return 0;
    }

    const int32 read_len = Peek(out_buffer, length);
    fun_check(used_length_ >= read_len);
    used_length_ -= read_len;
    if (0 == used_length_) {
      position_ = 0;
    } else {
      position_ += length;
    }

    return read_len;
  }

  /**
   * Copies the data currently in the FIFO
   * into the supplied buffer.
   * Resizes the supplied buffer to the size of
   * data written to it.
   * 
   * Returns the size of the copied data.
   */
  int32 Read(TBuffer<T>& out_buffer, int32 length = 0) const {
    ScopedLock guard(mutex_);

    if (!IsReadable()) {
      return 0;
    }

    const int32 read_len = Peek(out_buffer, length);
    fun_check(used_length_ >= read_len);
    used_length_ -= read_len;
    if (0 == used_length_) {
      position_ = 0;
    } else {
      position_ += length;
    }

    return read_len;
  }

  /**
   * Writes data from supplied buffer to the FIFO buffer.
   * If there is no sufficient space for the whole
   * buffer to be written, data up to available
   * length is written.
   * The length of data to be written is determined from the
   * length argument. Function does nothing and returns zero
   * if length argument is equal to zero.
   * 
   * Returns the length of data written.
   */
  int32 Write(const T* data, int32 length) {
    if (0 == length) {
      return 0;
    }

    ScopedLock guard(mutex_);

    if (!IsWritable()) {
      return 0;
    }

    if (internal_buffer_.Count() - (position_ + used_length_) < length) {
      UnsafeMemory::Memmove(internal_buffer_.Begin(), Begin(), used_length_ * sizeof(T));
      position_ = 0;
    }

    const int32 available_before =  internal_buffer_.Count() - used_length_ - position_;
    const int32 len = length > available_before ? available_before : length;
    UnsafeMemory::Memcpy(Begin() + used_length_, data, len * sizeof(T));
    used_length_ += len;
    fun_check(used_length_ <= internal_buffer_.Count());

    return len;
  }

  /**
   * Writes data from supplied buffer to the FIFO buffer.
   * If there is no sufficient space for the whole
   * buffer to be written, data up to available
   * length is written.
   * The length of data to be written is determined from the
   * length argument or buffer size (when length argument is
   * default zero or greater than buffer size).
   * 
   * Returns the length of data written.
   */
  int32 Write(const TBuffer<T>& data, int32 length = 0) {
    if (length == 0 || length > data.Count()) {
      length = data.Count();
    }

    return Write(data.Begin(), length);
  }

  /**
  Returns the size of the buffer.
  */
  int32 Count() const {
    return internal_buffer_.Count();
  }

  /**
  Returns the size of the used portion of the buffer.
  */
  int32 Used() const {
    return used_length_;
  }

  /**
  Returns the size of the available portion of the buffer.
  */
  int32 Available() const {
    return Count() - used_length_;
  }

  /**
  Drains length number of elements from the buffer.
  If length is zero or greater than buffer current
  content length, buffer is emptied.
  */
  void Drain(int32 length = 0) {
    ScopedLock guard(mutex_);

    if (0 == length || length >= used_length_) {
      position_ = 0;
      used_length_ = 0;
    } else {
      position_ += length;
      used_length_ -= length;
    }
  }

  /**
   * Copies the supplied data to the buffer and adjusts
   * the used buffer size.
   */
  void Copy(const T* data, int32 length) {
    if (length <= 0) {
      return;
    }

    ScopedLock guard(mutex_);

    if (length > Available()) {
      throw InvalidAccessException(CStringLiteral("cannot extend buffer."));
    }

    if (!IsWritable()) {
      throw InvalidAccessException(CStringLiteral("buffer not writable."));
    }

    UnsafeMemory::Memcpy(Begin() + used_length_, data, length * sizeof(T));
    used_length_ += length;
  }

  /**
   * Advances buffer by length elements.
   * Should be called AFTER the data
   * was copied into the buffer.
   */
  void Advance(int32 amount) {
    ScopedLock guard(mutex_);

    if (amount > Available()) {
      throw InvalidAccessException(CStringLiteral("cannot extend buffer."));
    }

    if (!IsWritable()) {
      throw InvalidAccessException(CStringLiteral("buffer not writable."));
    }

    if (internal_buffer_.Count() - (position_ + used_length_) < amount) {
      UnsafeMemory::Memmove(internal_buffer_.Begin(), Begin(), used_length_ * sizeof(T));
      position_ = 0;
    }

    used_length_ += amount;
  }

  /**
   * Returns the pointer to the beginning of the buffer.
   */
  T* Begin() {
    ScopedLock guard(mutex_);
    if (position_ != 0) {
      // Move the data to the start of the buffer so Begin() and Next()
      // always return consistent pointers with each other and allow writing
      // to the end of the buffer.
      UnsafeMemory::Memmove(internal_buffer_.Begin(), internal_buffer_.Begin() + position_, used_length_ * sizeof(T));
      position_ = 0;
    }
    return internal_buffer_.Begin();
  }

  /**
   * Returns the pointer to the next available position in the buffer.
   */
  T* Next() {
    ScopedLock guard(mutex_);
    return Begin() + used_length_;
  }

  /**
   * Returns value at index position.
   * Throws InvalidAccessException if index is larger than
   * the last valid (used) buffer position.
   */
  T& operator [] (int32 index) {
    ScopedLock guard(mutex_);
    if (index >= used_length_) {
      //TODO format
      //throw InvalidAccessException(format(CStringLiteral("index out of bounds: %z (max index allowed: %z)"), index, used_length_ - 1));
      throw InvalidAccessException(CStringLiteral("index out of bounds"));
    }

    return internal_buffer_[position_ + index];
  }

  /**
  Returns value at index position.
  Throws InvalidAccessException if index is larger than
  the last valid (used) buffer position.
  */
  const T& operator [] (int32 index) const {
    ScopedLock guard(mutex_);
    if (index >= used_length_) {
      //TODO format
      //throw InvalidAccessException(format(CStringLiteral("index out of bounds: %z (max index allowed: %z)"), index, used_length_ - 1));
      throw InvalidAccessException(CStringLiteral("index out of bounds"));
    }

    return internal_buffer_[position_ + index];
  }

  /**
  Returns const reference to the underlying buffer.
  */
  const TBuffer<T>& Buffer() const {
    return internal_buffer_;
  }

  /**
  Sets the error flag on the buffer and empties it.

  Setting error flag to true prevents reading and writing
  to the buffer; to re-enable FIFOBuffer for reading/writing,
  the error flag must be set to false.
  */
  void SetError(bool error = true) {
    if (error) {
      ScopedLock guard(mutex_);
      error_ = error;
      used_length_ = 0;
    } else {
      ScopedLock guard(mutex_);
      error_ = false;
    }
  }

  /**
  Returns true if error flag is not set on the buffer,
  otherwise returns false.
  */
  bool IsValid() const {
    return !error_;
  }

  void SetEOF(bool eof = true) {
    ScopedLock guard(mutex_);
    eof_ = eof;
  }

  bool HasEOF() const {
    return eof_;
  }

  bool IsEOF() const {
    return IsEmpty() && eof_;
  }

  bool IsEmpty() const {
    return 0 == used_length_;
  }

  bool IsFull() const {
    return Count() == used_length_;
  }

  bool IsReadable() const {
    return !IsEmpty() && IsValid();
  }

  bool IsWritable() const {
    return !IsFull() && IsValid() && !eof_;
  }

  Mutex& GetMutex() {
    return mutex_;
  }

 private:
  TFIFOBuffer();
  TFIFOBuffer(const TFIFOBuffer&);
  TFIFOBuffer& operator = (const TFIFOBuffer&);

 private:
  TBuffer<T> internal_buffer_;
  int32 position_;
  int32 used_length_;
  mutable Mutex mutex_;
  bool eof_;
  bool error_;
};

/**
 * We provide an instantiation for char
 */
typedef TFIFOBuffer<char> FIFOBuffer;

} // namespace fun
