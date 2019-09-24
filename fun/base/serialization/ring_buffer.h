// PRIVATE

#include "fun/base/base.h"
#include "fun/base/container/array.h"

#define FUN_RINGBUFFER_CHUNKSIZE 4096

namespace fun {

class RingBuffer {
 public:
  class Chunk {
   public:
    Chunk() : head_offset_(0), tail_offset_(0) {}

    inline Chunk(const Chunk& other) noexcept
        : chunk_(other.chunk_), head_(other.head_), tail_(other.tail_) {}

    explicit inline Chunk(int32 alloc) noexcept
        : chunk_(alloc, Uninitialized), head_(0), tail_(0) {}

    explicit inline Chunk(const ByteArray& data) noexcept
        : chunk_(data), head_(0), tail_(data.Len()) {}

    inline Chunk& operator=(const Chunk& other) noexcept {
      chunk_ = other.chunk_;
      head_ = other.head_;
      tail_ = other.tail_;
      return *this;
    }

    inline Chunk(Chunk&& other) noexcept
        : chunk_(other.chunk_), head_(other.head_), tail_(other.tail_) {
      other.head_ = 0;
      other.tail_ = 0;
    }

    inline Chunk& operator=(Chunk&& other) noexcept {
      Swap(other);
      return *this;
    }

    inline void Swap(Chunk& other) noexcept {
      chunk_.Swap(other.chunk_);

      fun::Swap(head_, other.head_);
      fun::Swap(tail_, other.tail_);
    }

    // allocating and sharing
    void Allocate(int32 alloc);
    inline bool IsShared() const { return !chunk_.isDetached(); }
    FUN_BASE_API void Detach();
    ByteArray ToByteArray();

    // getters
    inline int32 Head() const { return head_; }

    inline int32 Size() const { return tail_ - head_; }

    inline int32 Capacity() const { return chunk_.Size(); }

    inline int32 Available() const { return chunk_.Size() - tail_; }

    inline const char* ConstData() const { return chunk_.ConstData() + head_; }

    inline char* MutableData() {
      if (IsShared()) Detach();
      return chunk_.MutableData() + head_;
    }

    // array management
    inline void Advance(int32 offset) {
      fun_check(head_ + offset >= 0);
      fun_check(Size() - offset > 0);

      head_ += offset;
    }

    inline void Grow(int32 offset) {
      fun_check(Size() + offset > 0);
      fun_check(Head() + Size() + offset <= Capacity());

      tail_ += offset;
    }

    inline void Assign(const ByteArray& data) {
      chunk_ = data;
      head_ = 0;
      tail_ = data.Len();
    }

    inline void Reset() { head_ = tail_ = 0; }

    inline void Clear() { Assign(ByteArray()); }

   private:
    ByteArray chunk_;
    int32 head_;
    int32 tail_;
  };

  explicit inline RingBuffer(int32 growth = FUN_RINGBUFFER_CHUNKSIZE)
      : buffer_size_(0), basic_block_size_(growth) {}

  inline void SetChunkSize(int32 size) { basic_block_size_ = size; }

  inline int32 GetChunkSize() const { return basic_block_size_; }

  inline int64 GetNextDataBlockSize() const {
    return buffer_size_ == 0 ? int64(0) : buffers_.First().Size();
  }

  inline const char* ReadPointer() const {
    return buffer_size_ == 0 ? nullptr : buffers_.First().ConstData();
  }

  FUN_BASE_API const char* ReadPointerAtPosition(int64 pos, int64& len) const;
  FUN_BASE_API void Free(int64 len);
  FUN_BASE_API char* Reserve(int64 len);
  FUN_BASE_API char* ReserveFront(int64 len);

  inline void Truncate(int64 pos) {
    fun_check(pos >= 0 && pos <= Size());

    Chop(Size() - pos);
  }

  FUN_BASE_API void Chop(int64 len);

  inline bool IsEmpty() const { return buffer_size_ == 0; }

  inline int32 GetChar() {
    if (IsEmpty()) {
      return -1;
    }

    char c = *ReadPointer();
    Free(1);
    return int32(uint8(c));
  }

  inline void PutChar(char c) {
    char* ptr = Reserve(1);
    *ptr = c;
  }

  void UngetChar(char c) {
    char* ptr = ReserveFront(1);
    *ptr = c;
  }

  inline int64 Size() const { return buffer_size_; }

  FUN_BASE_API void Clear();
  inline int64 IndexOf(char c) const { return IndexOf(c, Size()); }
  FUN_BASE_API int64 IndexOf(char c, int64 max_len, int64 pos = 0) const;
  FUN_BASE_API int64 Read(char* buf, int64 max_len);
  FUN_BASE_API ByteArray Read();
  FUN_BASE_API int64 Peek(char* buf, int64 max_len, int64 pos = 0) const;
  FUN_BASE_API void Append(const char* data, int64 len);
  FUN_BASE_API void Append(const ByteArray& data);

  inline int64 Skip(int64 len) {
    int64 bytes_to_skip = MathBase::Min(len, buffer_size_);

    Free(bytes_to_skip);
    return bytes_to_skip;
  }

  FUN_BASE_API int64 ReadLine(char* buf, int64 max_len);

  inline bool CanReadLine() const { return IndexOf('\n') >= 0; }

 private:
  Array<Chunk> buffers_;
  int64 buffer_size_;
  int32 basic_block_size_;
};

}  // namespace fun
