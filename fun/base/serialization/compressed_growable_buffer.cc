#include "fun/base/serialization/compressed_growable_buffer.h"

namespace fun {

CompressedGrowableBuffer::CompressedGrowableBuffer(int32 max_pending_buffer_size, CompressionFlags compression_flags)
  : max_pending_buffer_size_(max_pending_buffer_size),
    compression_flags_(compression_flags),
    current_offset_(0),
    entry_count_(0),
    decompressed_buffer_book_keeping_info_index_(INVALID_INDEX) {
  pending_compression_buffer_.Clear(max_pending_buffer_size);
}

void CompressedGrowableBuffer::Lock() {
  fun_check(decompressed_buffer_.Count() == 0);
}

void CompressedGrowableBuffer::Unlock() {
  decompressed_buffer_.Clear();
  decompressed_buffer_book_keeping_info_index_ = INVALID_INDEX;
}

int32 CompressedGrowableBuffer::Append(void* data, int32 size) {
  fun_check(decompressed_buffer_.Count() == 0);
  fun_check(size <= max_pending_buffer_size_);
  entry_count_++;

  // data does NOT fit into pending compression buffer. Compress existing data
  // and purge buffer.
  if (max_pending_buffer_size_ - pending_compression_buffer_.Count() < size) {
    // Allocate temporary buffer to hold compressed data. It is bigger than the uncompressed size as
    // compression is not guaranteed to create smaller data and we don't want to handle that case so
    // we simply assert if it doesn't fit. For all practical purposes this works out fine and is what
    // other code in the engine does as well.
    int32 compressed_size = max_pending_buffer_size_ * 4 / 3;
    void* tmp_buffer = UnsafeMemory::Malloc(compressed_size);

    // Compress the memory. compressed_size is [in/out]
    //TODO
    //fun_verify(Compression::Compress(compression_flags_, tmp_buffer, compressed_size, pending_compression_buffer_.ConstData(), pending_compression_buffer_.Count()));
    Compression::Compress(compression_flags_, tmp_buffer, compressed_size, pending_compression_buffer_.ConstData(), pending_compression_buffer_.Count());

    // Append the compressed data to the compressed buffer and delete temporary data.
    int32 start_index = compressed_buffer_.AddUninitialized(compressed_size);
    UnsafeMemory::Memcpy(&compressed_buffer_[start_index], tmp_buffer, compressed_size);
    UnsafeMemory::Free(tmp_buffer);

    // Keep track of book keeping info for later access to data.
    BufferBookKeeping info;
    info.compressed_offset = start_index;
    info.compressed_size = compressed_size;
    info.uncompressed_offset = current_offset_ - pending_compression_buffer_.Count();
    info.uncompressed_size = pending_compression_buffer_.Count();
    book_keeping_info_.Add(info);

    // Resize & empty the pending buffer to the default state.
    pending_compression_buffer_.Clear(max_pending_buffer_size_);
  }

  // Appends the data to the pending buffer. The pending buffer is compressed
  // as needed above.
  const int32 start_index = pending_compression_buffer_.AddUninitialized(size);
  UnsafeMemory::Memcpy(&pending_compression_buffer_[start_index], data, size);

  // Return start offset in uncompressed memory.
  const int32 start_offset = current_offset_;
  current_offset_ += size;
  return start_offset;
}

void* CompressedGrowableBuffer::Access(int32 offset) {
  void* uncompressed_data = nullptr;

  // Check whether the decompressed data is already cached.
  if (decompressed_buffer_book_keeping_info_index_ != INVALID_INDEX) {
    const BufferBookKeeping& info = book_keeping_info_[decompressed_buffer_book_keeping_info_index_];
    // Cache HIT.
    if ((info.uncompressed_offset <= offset) && (info.uncompressed_offset + info.uncompressed_size > offset)) {
      // Figure out index into uncompressed data and set it. DecompressionBuffer (return value) is going
      // to be valid till the next call to Access or Unlock.
      const int32 internal_offset = offset - info.uncompressed_offset;
      uncompressed_data = &decompressed_buffer_[internal_offset];
    }
    // Cache MISS.
    else {
      decompressed_buffer_book_keeping_info_index_ = INVALID_INDEX;
    }
  }

  // Traverse book keeping info till we find the matching block.
  if (uncompressed_data == nullptr) {
    for (int32 info_index = 0; info_index < book_keeping_info_.Count(); ++info_index) {
      const BufferBookKeeping& info = book_keeping_info_[info_index];
      if ((info.uncompressed_offset <= offset) && (info.uncompressed_offset + info.uncompressed_size > offset)) {
        // Found the right buffer, now decompress it.
        decompressed_buffer_.Clear(info.uncompressed_size);
        decompressed_buffer_.AddUninitialized(info.uncompressed_size);
        //TODO
        //fun_verify(Compression::Uncompress(compression_flags_, decompressed_buffer_.MutableData(), info.uncompressed_size, &compressed_buffer_[info.compressed_offset], info.compressed_size));
        Compression::Uncompress(compression_flags_, decompressed_buffer_.MutableData(), info.uncompressed_size, &compressed_buffer_[info.compressed_offset], info.compressed_size);

        // Figure out index into uncompressed data and set it. DecompressionBuffer (return value) is going
        // to be valid till the next call to Access or Unlock.
        const int32 internal_offset = offset - info.uncompressed_offset;
        uncompressed_data = &decompressed_buffer_[internal_offset];

        // Keep track of buffer index for the next call to this function.
        decompressed_buffer_book_keeping_info_index_ = info_index;
        break;
      }
    }
  }

  // If we still haven't found the data it might be in the pending compression buffer.
  if (uncompressed_data == nullptr) {
    const int32 uncompressed_start_offset = current_offset_ - pending_compression_buffer_.Count();
    if ((uncompressed_start_offset <= offset) && (current_offset_ > offset)) {
      // Figure out index into uncompressed data and set it. pending_compression_buffer_ (return value)
      // is going to be valid till the next call to Access, Unlock or Append.
      const int32 internal_offset = offset - uncompressed_start_offset;
      uncompressed_data = &pending_compression_buffer_[internal_offset];
    }
  }

  // Return value is only valid till next call to Access, Unlock or Append!
  fun_check(uncompressed_data);
  return uncompressed_data;
}

} // namespace fun
