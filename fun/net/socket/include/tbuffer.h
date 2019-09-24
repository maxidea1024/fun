// TODO ChannelBuffer로 대체하는게 좋을듯...
//이름은 그대로 Buffer라고 쓰고, 코드만 변경?
//아니면 이름도 새로 바꿔서 적용하는게 좋으려나...

#pragma once

#include "fun/net/net.h"

namespace fun {

/**
 * A buffer class that allocates a buffer of a given type and size
 * in the constructor and deallocates the buffer in the destructor.
 *
 * This class is useful everywhere where a temporary buffer
 * is needed.
 */
template <typename T>
class TBuffer {
 public:
  /**
   * Creates and allocates the Buffer.
   */
  TBuffer(int32 length)
      : capacity_(length), used_length_(length), ptr_(nullptr), own_mem_(true) {
    if (length > 0) {
      ptr_ = new T[length];
    }
  }

  /**
   * Creates the Buffer. length argument specifies the length
   * of the supplied memory pointed to by pMem in the number
   * of elements of type T. Supplied pointer is considered
   * blank and not owned by Buffer, so in this case Buffer
   * only acts as a wrapper around externally supplied
   * (and lifetime-managed) memory.
   */
  TBuffer(T* external_mem, int32 length)
      : capacity_(length),
        used_length_(length),
        ptr_(external_mem),
        own_mem_(false) {}

  /**
   * Creates and allocates the Buffer; copies the contents of
   * the supplied memory into the buffer. length argument specifies
   * the length of the supplied memory pointed to by external_mem in the
   * number of elements of type T.
   */
  TBuffer(const T* external_mem, int32 length)
      : capacity_(length), used_length_(length), ptr_(nullptr), own_mem_(true) {
    if (capacity_ > 0) {
      ptr_ = new T[capacity_];
      UnsafeMemory::Memcpy(ptr_, external_mem, used_length_ * sizeof(T));
    }
  }

  /**
   * Copy constructor.
   */
  TBuffer(const TBuffer& rhs)
      : capacity_(rhs.used_length_),
        used_length_(rhs.used_length_),
        ptr_(nullptr),
        own_mem_(true) {
    if (used_length_ > 0) {
      ptr_ = new T[used_length_];
      UnsafeMemory::Memcpy(ptr_, rhs.ptr_, used_length_ * sizeof(T));
    }
  }

  /**
   * Assignment operator.
   * */
  TBuffer& operator=(const TBuffer& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      TBuffer tmp(rhs);
      Swap(tmp);
    }

    return *this;
  }

  /**
   * Destroys the Buffer.
   */
  ~TBuffer() {
    if (own_mem_) {
      delete[] ptr_;
    }
  }

  /**
   * Clear buffer.
   */
  void Clear() { Resize(0, false); }

  /**
   * Resizes the buffer capacity and size. If preserve_content is true,
   * the content of the old buffer is copied over to the
   * new buffer. The new capacity can be larger or smaller than
   * the current one; if it is smaller, capacity will remain intact.
   * Size will always be set to the new capacity.
   *
   * Buffers only wrapping externally owned storage can not be
   * resized. If resize is attempted on those, InvalidAccessException
   * is thrown.
   */
  void Resize(int32 new_capacity, bool preserve_content = true) {
    if (!own_mem_) {
      throw InvalidAccessException(
          "cannot resize buffer which does not own its storage.");
    }

    if (new_capacity > capacity_) {
      T* new_mem = new T[new_capacity];
      if (preserve_content) {
        UnsafeMemory::Memcpy(new_mem, ptr_, used_length_ * sizeof(T));
      }
      delete[] ptr_;
      ptr_ = new_mem;
      capacity_ = new_capacity;
    }

    used_length_ = new_capacity;
  }

  /**
   * Sets the buffer capacity. If preserve_content is true,
   * the content of the old buffer is copied over to the
   * new buffer. The new capacity can be larger or smaller than
   * the current one; size will be set to the new capacity only if
   * new capacity is smaller than the current size, otherwise it will
   * remain intact.
   *
   * Buffers only wrapping externally owned storage can not be
   * resized. If resize is attempted on those, InvalidAccessException
   * is thrown.
   */
  void SetCapacity(int32 new_capacity, bool preserve_content = true) {
    if (!own_mem_) {
      throw InvalidAccessException(
          "cannot resize buffer which does not own its storage.");
    }

    if (new_capacity != capacity_) {
      T* new_mem = nullptr;
      if (new_capacity > 0) {
        new_mem = new T[new_capacity];
        if (preserve_content) {
          const int32 new_length =
              used_length_ < new_capacity ? used_length_ : new_capacity;
          UnsafeMemory::Memcpy(new_mem, ptr_, new_length * sizeof(T));
        }
      }
      delete[] ptr_;
      ptr_ = new_mem;
      capacity_ = new_capacity;

      if (new_capacity < used_length_) {
        used_length_ = new_capacity;
      }
    }
  }

  /**
   * Assigns the argument buffer to this buffer.
   * If necessary, resizes the buffer.
   */
  void Assign(const T* data, int32 length) {
    if (length <= 0) {
      return;
    }

    if (length > capacity_) {
      Resize(length, false);
    }

    UnsafeMemory::Memcpy(ptr_, data, length * sizeof(T));

    used_length_ = length;
  }

  /**
   * Resizes this buffer and appends the argument buffer.
   */
  void Append(const T* data, int32 length) {
    if (length <= 0) {
      return;
    }

    Resize(used_length_ + length, true);

    UnsafeMemory::Memcpy(ptr_ + used_length_ - length, data,
                         length * sizeof(T));
  }

  /**
   * Resizes this buffer by one element and appends the argument value.
   */
  void Append(const T& value) {
    Resize(used_length_ + 1, true);
    ptr_[used_length_ - 1] = value;
  }

  /**
   * Resizes this buffer and appends the argument buffer.
   */
  void Append(const TBuffer& buf) { Append(buf.begin(), buf.Count()); }

  /**
   * Returns the allocated memory size in elements.
   */
  int32 Capacity() const { return capacity_; }

  /**
   * Returns the allocated memory size in bytes.
   */
  int32 CapacityBytes() const { return capacity_ * sizeof(T); }

  /**
   * Swaps the buffer with another one.
   */
  void Swap(TBuffer& other) {
    fun::Swap(ptr_, other.ptr_);
    fun::Swap(capacity_, other.capacity_);
    fun::Swap(used_length_, other.used_length_);
  }

  /**
   * Compare operator.
   */
  bool operator==(const TBuffer& rhs) const {
    if (this != &rhs) {
      if (used_length_ == rhs.used_length_) {
        if (UnsafeMemory::Memcmp(ptr_, rhs.ptr_, used_length_ * sizeof(T)) ==
            0) {
          return true;
        }
      }
      return false;
    }

    return true;
  }

  /**
   * Compare operator.
   */
  bool operator!=(const TBuffer& rhs) const { return !(*this == rhs); }

  /**
   * Sets the contents of the buffer to zero.
   */
  void SetZeroed() { UnsafeMemory::Memzero(ptr_, used_length_ * sizeof(T)); }

  /**
   * Returns the used size of the buffer in elements.
   */
  int32 Count() const { return used_length_; }

  /**
   * Returns the used size of the buffer in bytes.
   */
  int32 ByteCount() const { return used_length_ * sizeof(T); }

  /**
   * Returns a pointer to the beginning of the buffer.
   */
  T* begin() { return ptr_; }

  /**
   * Returns a pointer to the beginning of the buffer.
   */
  const T* begin() const { return ptr_; }

  /**
   * Returns a pointer to end of the buffer.
   */
  T* end() { return ptr_ + used_length_; }

  /**
   * Returns a pointer to the end of the buffer.
   */
  const T* end() const { return ptr_ + used_length_; }

  /**
   * Return true if buffer is empty.
   */
  bool IsEmpty() const { return 0 == used_length_; }

  T& operator[](int32 index) {
    fun_check(index < used_length_);
    return ptr_[index];
  }

  const T& operator[](int32 index) const {
    fun_check(index < used_length_);
    return ptr_[index];
  }

 private:
  TBuffer();

  int32 capacity_;
  int32 used_length_;
  T* ptr_;
  bool own_mem_;
};

}  // namespace fun
