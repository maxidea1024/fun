// TODO core쪽으로 빼주자...
#pragma once

#include "fun/net/net.h"

#define SUPPORT_ZIP_COMPRESSOR 1
#define SUPPORT_LZF_COMPRESSOR 0
#define SUPPORT_SNAPPY_COMPRESSOR 0

namespace fun {
namespace net {

class FUN_NETX_API Compressor {
 public:
  static int32 GetMaxCompressedlen(CompressionMode compression_mode,
                                   const int32 len);
  static bool Compress(CompressionMode compression_mode, uint8* out,
                       int32* out_len, const uint8* in, const int32 in_len,
                       String* out_error);
  static bool Decompress(CompressionMode compression_mode, uint8* out,
                         int32* out_len, const uint8* in, const int32 in_len,
                         String* out_error);

  //
  // Raw
  //

  static int32 Raw_GetMaxCompressedlen(int32 len);
  static bool Raw_Compress(uint8* out, int32* out_len, const uint8* in,
                           const int32 in_len, String* out_error);
  static bool Raw_Decompress(uint8* out, int32* out_len, const uint8* in,
                             const int32 in_len, String* out_error);

  //
  // Zip(zlib)
  //

#if SUPPORT_ZIP_COMPRESSOR
  static int32 Zip_GetMaxCompressedlen(int32 len);
  static bool Zip_Compress(uint8* out, int32* out_len, const uint8* in,
                           const int32 in_len, String* out_error);
  static bool Zip_Decompress(uint8* out, int32* out_len, const uint8* in,
                             const int32 in_len, String* out_error);
#endif

  //
  // Lzf
  //

#if SUPPORT_LZF_COMPRESSOR
  static int32 Lzf_GetMaxCompressedlen(int32 len);
  static bool Lzf_Compress(uint8* out, int32* out_len, const uint8* in,
                           const int32 in_len, String* out_error);
  static bool Lzf_Decompress(uint8* out, int32* out_len, const uint8* in,
                             const int32 in_len, String* out_error);
#endif

  //
  // Snappy
  //

  //@note
  // snappy의 경우 현재, unity3d 용 라이브러리에서 64비트를 지원하지 못하고
  // 있음. 사용을 하는게 바람직한건지 어쩐건지.. 아니면, unity3d를 사용하지 않고,
  //네이티브로 구현한다면 상관 없을텐데...
#if SUPPORT_SNAPPY_COMPRESSOR
  static int32 Snappy_GetMaxCompressedlen(int32 len);
  static bool Snappy_Compress(uint8* out, int32* out_len, const uint8* in,
                              const int32 in_len, String* out_error);
  static bool Snappy_Decompress(uint8* out, int32* out_len, const uint8* in,
                                const int32 in_len, String* out_error);
#endif
};

}  // namespace net
}  // namespace fun
