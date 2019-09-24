#pragma once

#include "fun/base/base.h"
#include "fun/base/flags.h"

namespace fun {

enum class CompressionFlag {
  None = 0x00,
  ZLib = 0x01,
  GZip = 0x02,
  BiasMemory = 0x10,
  BiasSpeed = 0x20,

  TypeMask = 0x0F,
  OptionsMask = 0xF0,

  Default = ZLib,
};

FUN_DECLARE_FLAGS(CompressionFlags, CompressionFlag);

namespace CompressionConstants {

  static const int32 LOADING_COMPRESSION_CHUNK_SIZE_PRE_369 = 32768;
  static const int32 LOADING_COMPRESSION_CHUNK_SIZE = 131072;
  static const int32 SAVING_COMPRESSION_CHUNK_SIZE = LOADING_COMPRESSION_CHUNK_SIZE;

} // namespace CompressionConstants

class Compression {
 public:
  static const uint32 MAX_UNCOMPRESSED_SIZE = 256 * 1024;

  //TODO
  //static FUN_ALIGNED_VOLATILE double compressor_time;
  //static FUN_ALIGNED_VOLATILE uint64 compressor_src_bytes;
  //static FUN_ALIGNED_VOLATILE uint64 compressor_dst_bytes;
  static volatile double compressor_time;
  static volatile uint64 compressor_src_bytes;
  static volatile uint64 compressor_dst_bytes;

  FUN_BASE_API static int32 CompressBound(CompressionFlags flags, int32 uncompressed_size);

  FUN_BASE_API static bool Compress(CompressionFlags flags,
                                    void* compressed_buffer,
                                    int32& compressed_size,
                                    const void* uncompressed_buffer,
                                    int32 uncompressed_size);

  FUN_BASE_API static bool Uncompress(CompressionFlags flags,
                                      void* uncompressed_buffer,
                                      int32 uncompressed_size,
                                      const void* compressed_buffer,
                                      int32 compressed_size,
                                      bool is_source_padded = false);
};

} // namespace fun
