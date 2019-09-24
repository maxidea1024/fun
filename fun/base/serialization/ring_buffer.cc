//PRIVATE

#include "fun/base/ring_buffer.h"

namespace fun {

//
// RingBuffer::Chunk
//

void RingBuffer::Chunk::Allocate(int32 alloc) {
  fun_check(alloc > 0 && Size() == 0);

  if (chunk_.Size() < alloc || IsShared()) {
    chunk_ = ByteArray(alloc, Uninitialized);
  }
}

void RingBuffer::Chunk::Detach() {
  fun_check(IsShared());

  const int32 chunk_size = Size();
  ByteArray tmp(chunk_size, Uninitialized);
  UnsafeMemory::Memcpy(tmp.MutableData(), chunk_.ConstData() + head_, chunk_size);
  chunk_ = MoveTemp(tmp);
  head_ = 0;
  tail_ = chunk_size;
}

ByteArray RingBuffer::Chunk::ToByteArray() {
  if (head_ != 0 || tail_ != chunk_.Size()) {
    if (IsShared()) {
      return chunk_.Mid(head_, Size());
    }

    if (head_ != 0) {
      char* ptr = chunk_.MutableData();
      UnsafeMemory::Memmove(ptr, ptr + head_, Size());
      tail_ -= head_;
      head_ = 0;
    }

    chunk_.Reserve(0); // avoid that resizing needlessly reallocates
    chunk_.Resize(tail_);
  }

  return chunk_;
}


//
// RingBuffer
//

const char*
RingBuffer::ReadPointerAtPosition(int64 pos, int64& len) const {
  fun_check(pos >= 0);

  for (const Chunk& chunk : buffers_) {
    len = chunk.Size();
    if (len > pos) {
      len -= pos;
      return chunk.ConstData() + pos;
    }
    pos -= len;
  }

  len = 0;
  return nullptr;
}

void RingBuffer::Free(int64 len) {
  fun_check(len <= buffer_size_);

  while (len > 0) {
    const int64 chunk_size = buffers_.First().Size();

    if (buffers_.Size() == 1 || chunk_size > len) {
      Chunk& chunk = buffers_.First();

      // keep a single block around if it does not exceed
      // the basic block Size, to avoid repeated allocations
      // between uses of the buffer
      if (buffer_size_ == len) {
        if (chunk.Capacity() <= basic_block_size_ && !chunk.IsShared()) {
          chunk.Reset();
          buffer_size_ = 0;
        } else {
          Clear(); // try to minify/squeeze us
        }
      } else {
        fun_check(len < MaxByteArraySize);
        chunk.Advance(len);
        buffer_size_ -= len;
      }
      return;
    }

    buffer_size_ -= chunk_size;
    len -= chunk_size;
    buffers_.RemoveFirst();
  }
}

char* RingBuffer::Reserve(int64 len) {
  fun_check(len > 0 && len < MaxByteArraySize);

  const int32 chunk_size = MathBase::Max(basic_block_size_, int32(len));
  int32 tail = 0;
  if (buffer_size_ == 0) {
    if (buffers_.IsEmpty()) {
      buffers_.Append(Chunk(chunk_size));
    } else {
      buffers_.First().Allocate(chunk_size);
    }
  } else {
    const Chunk& chunk = buffers_.Last();
    // if need a new buffer
    if (basic_block_size_ == 0 || chunk.IsShared() || len > chunk.Available()) {
      buffers_.Append(Chunk(chunk_size));
    } else {
      tail = chunk.Size();
    }
  }

  buffers_.Last().Grow(len);
  buffer_size_ += len;
  return buffers_.Last().MutableData() + tail;
}

char* RingBuffer::ReserveFront(int64 len) {
  fun_check(len > 0 && len < MaxByteArraySize);

  const int32 chunk_size = MathBase::Max(basic_block_size_, int32(len));
  if (buffer_size_ == 0) {
    if (buffers_.IsEmpty()) {
      buffers_.Prepend(Chunk(chunk_size));
    } else {
      buffers_.First().Allocate(chunk_size);
    }
    buffers_.First().Grow(chunk_size);
    buffers_.First().Advance(chunk_size - len);
  } else {
    const Chunk& chunk = buffers_.First();
    // if need a new buffer
    if (basic_block_size_ == 0 || chunk.IsShared() || len > chunk.Head()) {
      buffers_.Prepend(Chunk(chunk_size));
      buffers_.First().Grow(chunk_size);
      buffers_.First().Advance(chunk_size - len);
    } else {
      buffers_.First().Advance(-len);
    }
  }

  buffer_size_ += len;
  return buffers_.First().MutableData();
}

void RingBuffer::Chop(int64 len) {
  fun_check(len <= buffer_size_);

  while (len > 0) {
    const int64 chunk_size = buffers_.Last().Size();

    if (buffers_.Size() == 1 || chunk_size > len) {
      Chunk& chunk = buffers_.Last();

      // keep a single block around if it does not exceed
      // the basic block Size, to avoid repeated allocations
      // between uses of the buffer
      if (buffer_size_ == len) {
        if (chunk.Capacity() <= basic_block_size_ && !chunk.IsShared()) {
          chunk.Reset();
          buffer_size_ = 0;
        } else {
          Clear(); // try to minify/squeeze us
        }
      } else {
        fun_check(len < MaxByteArraySize);
        chunk.Grow(-len);
        buffer_size_ -= len;
      }
      return;
    }

    buffer_size_ -= chunk_size;
    len -= chunk_size;
    buffers_.RemoveLast();
  }
}

void RingBuffer::Clear() {
  if (buffers_.IsEmpty()) {
    return;
  }

  buffers_.erase(buffers_.begin() + 1, buffers_.end());
  buffers_.First().Clear();
  buffer_size_ = 0;
}

int64 RingBuffer::IndexOf(char c, int64 max_len, int64 pos) const {
  fun_check(max_len >= 0 && pos >= 0);

  if (max_len == 0) {
    return -1;
  }

  int64 index = -pos;
  for (const Chunk& chunk : buffers_) {
    const int64 next_block_index = MathBase::Min(index + chunk.Size(), max_len);

    if (next_block_index > 0) {
      const char* ptr = chunk.ConstData();
      if (index < 0) {
        ptr -= index;
        index = 0;
      }

      const char* found_ptr = reinterpret_cast<const char*>(memchr(ptr, c, next_block_index - index));
      if (found_ptr) {
        return int64(found_ptr - ptr) + index + pos;
      }

      if (next_block_index == max_len) {
        return -1;
      }
    }
    index = next_block_index;
  }
  return -1;
}

int64 RingBuffer::Read(char* buf, int64 max_len) {
  const int64 bytes_to_read = MathBase::Min(Size(), max_len);
  int64 read_so_far = 0;
  while (read_so_far < bytes_to_read) {
    const int64 bytes_to_read_from_this_block =
              MathBase::Min(bytes_to_read - read_so_far, GetNextDataBlockSize());
    if (buf) {
      UnsafeMemory::Memcpy(buf + read_so_far, ReadPointer(), bytes_to_read_from_this_block);
    }
    read_so_far += bytes_to_read_from_this_block;
    Free(bytes_to_read_from_this_block);
  }

  return read_so_far;
}

ByteArray RingBuffer::Read() {
  if (buffer_size_ == 0) {
    return ByteArray();
  }

  buffer_size_ -= buffers_.First().Size();
  return buffers_.TakeFirst().ToByteArray();
}

int64 RingBuffer::Peek(char* buf, int64 max_len, int64 pos) const {
  fun_check(max_len >= 0 && pos >= 0);

  int64 read_so_far = 0;
  for (int32 i = 0; read_so_far < max_len && i < buffers_.Count(); ++i) {
    int64 block_len = buffers_[i].Len();

    if (pos < block_len) {
      block_len = MathBase::Min(block_len - pos, max_len - read_so_far);
      UnsafeMemory::Memcpy(buf + read_so_far, buffers_[i].ConstData() + pos, block_len);
      read_so_far += block_len;
      pos = 0;
    } else {
      pos -= block_len;
    }
  }

  return read_so_far;
}

void RingBuffer::Append(const char* data, int64 len) {
  fun_check(len >= 0);

  if (len == 0) {
    return;
  }

  char* write_ptr = Reserve(len);
  if (len == 1) {
    *write_ptr = *data;
  } else {
    UnsafeMemory::Memcpy(write_ptr, data, len);
  }
}

void RingBuffer::Append(const ByteArray& data) {
  if (buffer_size_ != 0 || buffers_.IsEmpty()) {
    buffers_.Append(Chunk(data));
  } else {
    buffers_.Last().Assign(data);
  }

  buffer_size_ += data.Count();
}

int64 RingBuffer::ReadLine(char* buf, int64 max_len) {
  fun_check(buf && max_len > 1);

  --max_len;
  int64 i = IndexOf('\n', max_len);
  i = Read(buf, i >= 0 ? (i + 1) : max_len);

  // Terminate it.
  buf[i] = '\0';
  return i;
}

} // namespace fun
