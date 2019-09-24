#include "fun/base/serialization/compression.h"

namespace fun {

FUN_ALIGNED_VOLATILE double Compression::compressor_time = 0.0;
FUN_ALIGNED_VOLATILE uint64 Compression::compressor_src_bytes = 0;
FUN_ALIGNED_VOLATILE uint64 Compression::compressor_dst_bytes = 0;

int32 Compression::CompressBound(CompressionFlags flags,
                                 int32 uncompressed_size) {
  int32 compression_bound = uncompressed_size;

  flags = CheckGlobalCompressionFlags(flags);

  if ((flags & CompressionFlags::TypeMask) == CompressionFlags::ZLib) {
    compression_bound = compressBound(uncompressed_size);
  }

  return compression_bound;
}

bool Compression::Compress(CompressionFlags flags, void* compressed_buffer,
                           int32& compressed_size,
                           const void* uncompressed_buffer,
                           int32 uncompressed_size) {
  // TODO
}

bool Compression::Uncompress(CompressionFlags flags, void* uncompressed_buffer,
                             int32 uncompressed_size,
                             const void* compressed_buffer,
                             int32 compressed_size, bool is_source_padded) {
  // TODO
}

}  // namespace fun
