#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/serialization/compression.h"

namespace fun {

/**
 * Growable compressed buffer. Usage is to append frequently but only request
 * and therefore decompress very infrequently. The prime usage case is the
 * memory profiler keeping track of full call stacks.
 */
class FUN_BASE_API CompressedGrowableBuffer {
 public:
  /**
   * Constructor
   *
   * @param max_pending_buffer_size - Max chunk size to compress in uncompressed
   * bytes
   * @param compression_flags - Compression flags to compress memory with
   */
  CompressedGrowableBuffer(int32 max_pending_buffer_size,
                           CompressionFlags compression_flags);

  /**
   * Locks the buffer for reading. Needs to be called before calls to Access and
   * needs to be matched up with Unlock call.
   */
  void Lock();

  /**
   * Unlocks the buffer and frees temporary resources used for accessing.
   */
  void Unlock();

  /**
   * Appends passed in data to the buffer. The data needs to be less than the
   * max pending buffer size. The code will assert on this assumption.
   *
   * @param data - Data to append
   * @param size - Size of data in bytes.
   *
   * @return Offset of data, used for retrieval later on
   */
  int32 Append(void* data, int32 size);

  /**
   * Accesses the data at passed in offset and returns it. The memory is
   * read-only and memory will be freed in call to unlock. The lifetime of the
   * data is till the next call to Unlock, Append or Access
   *
   * @param offset - Offset to return corresponding data for
   */
  void* Access(int32 offset);

  /**
   * @return  Number of entries appended.
   */
  int32 Count() const { return entry_count_; }

  /**
   * Helper function to return the amount of memory allocated by this buffer
   *
   * @return number of bytes allocated by this buffer
   */
  uint32 GetAllocatedSize() const {
    return compressed_buffer_.GetAllocatedSize() +
           pending_compression_buffer_.GetAllocatedSize() +
           decompressed_buffer_.GetAllocatedSize() +
           book_keeping_info_.GetAllocatedSize();
  }

 private:
  /** Helper structure for book keeping. */
  struct BufferBookKeeping {
    /** Offset into compressed data. */
    int32 compressed_offset;
    /** Size of compressed data in this chunk. */
    int32 compressed_size;
    /** Offset into uncompressed data. */
    int32 uncompressed_offset;
    /** Size of uncompressed data in this chunk. */
    int32 uncompressed_size;
  };

  /** Maximum chunk size to compress in uncompressed bytes. */
  int32 max_pending_buffer_size_;
  /** Compression flags used to compress the data. */
  CompressionFlags compression_flags_;
  /** Current offset in uncompressed data. */
  int32 current_offset_;
  /** Number of entries in buffer. */
  int32 entry_count_;
  /** Compressed data. */
  Array<uint8> compressed_buffer_;
  /** Data pending compression once size limit is reached. */
  Array<uint8> pending_compression_buffer_;
  /** Temporary decompression buffer used between Lock/ Unlock. */
  Array<uint8> decompressed_buffer_;
  /** Index into book keeping info associated with decompressed buffer. */
  int32 decompressed_buffer_book_keeping_info_index_;
  /** Book keeping information for decompression/ access. */
  Array<BufferBookKeeping> book_keeping_info_;
};

}  // namespace fun
