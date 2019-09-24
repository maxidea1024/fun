// TODO 코드정리
#include "Net/Engine/Compressor.h"
#include "fun/net/net.h"

#if SUPPORT_ZIP_COMPRESSOR
#include "ThirdParty/zlib/zlib.h"
#endif

#if SUPPORT_LZF_COMPRESSOR
extern "C" {
#include "ThirdParty/lzf/lzf.h"
}
#endif

#if SUPPORT_SNAPPY_COMPRESSOR
#include "ThirdParty/snappy-1.1.0/snappy-c.h"  // C API
#endif

namespace fun {
namespace net {

//
// Raw
//

int32 Compressor::Raw_GetMaxCompressedLength(int32 length) { return length; }

bool Compressor::Raw_Compress(uint8* out, int32* out_length, const uint8* in,
                              const int32 in_length, String* out_error) {
  fun_check(*out_length == in_length);
  UnsafeMemory::Memcpy(out, in, in_length);
  *out_length = in_length;
  return true;
}

bool Compressor::Raw_Decompress(uint8* out, int32* out_length, const uint8* in,
                                const int32 in_length, String* out_error) {
  fun_check(*out_length == in_length);
  UnsafeMemory::Memcpy(out, in, in_length);
  *out_length = in_length;
  return true;
}

//
// Zip(zlib)
//

#if SUPPORT_ZIP_COMPRESSOR
static voidpf ZlibCalloc(voidpf opaque, unsigned count, unsigned size) {
  return UnsafeMemory::Malloc(count * size);
}

static void ZlibFree(voidpf opaque, voidpf ptr) { UnsafeMemory::Free(ptr); }

static int ZlibCompress(Bytef* out, uLongf* out_length, const Bytef* in,
                        uLong in_length) {
  int level = Z_DEFAULT_COMPRESSION;
  z_stream stream;
  int err;

  stream.next_in = (Bytef*)in;
  stream.avail_in = (uInt)in_length;

#ifdef MAXSEG_64K
  // Check for source > 64K on 16-bit machine:
  if ((uLong)stream.avail_in != in_length) {
    return Z_BUF_ERROR;
  }
#endif

  stream.next_out = out;
  stream.avail_out = (uInt)*out_length;
  if ((uLong)stream.avail_out != *out_length) {
    return Z_BUF_ERROR;
  }

  stream.zalloc = ZlibCalloc;
  stream.zfree = ZlibFree;

  stream.opaque = (voidpf)0;

  err = deflateInit(&stream, level);
  if (err != Z_OK) {
    return err;
  }

  err = deflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    deflateEnd(&stream);
    return err == Z_OK ? Z_BUF_ERROR : err;
  }
  *out_length = stream.total_out;

  err = deflateEnd(&stream);
  return err;
}

static int ZlibDecompress(Bytef* out, uLongf* out_length, const Bytef* in,
                          uLong in_length) {
  z_stream stream;
  int err;

  stream.next_in = (Bytef*)in;
  stream.avail_in = (uInt)in_length;
  /* Check for source > 64K on 16-bit machine: */
  if ((uLong)stream.avail_in != in_length) {
    return Z_BUF_ERROR;
  }

  stream.next_out = out;
  stream.avail_out = (uInt)*out_length;
  if ((uLong)stream.avail_out != *out_length) {
    return Z_BUF_ERROR;
  }

  stream.zalloc = ZlibCalloc;
  stream.zfree = ZlibFree;

  err = inflateInit(&stream);
  if (err != Z_OK) {
    return err;
  }

  err = inflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    inflateEnd(&stream);
    if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0)) {
      return Z_DATA_ERROR;
    }
    return err;
  }
  *out_length = stream.total_out;

  err = inflateEnd(&stream);
  return err;
}

int32 Compressor::Zip_GetMaxCompressedLength(int32 length) {
  return compressBound(length);
}

bool Compressor::Zip_Compress(uint8* out, int32* out_length, const uint8* in,
                              const int32 in_length, String* out_error) {
  uLongf compressed_length = (uLongf)*out_length;
  int result = ZlibCompress((Bytef*)out, &compressed_length, (const Bytef*)in,
                            (uLong)in_length);
  if (result == Z_OK) {
    *out_error = TEXT("");
    *out_length = (int32)compressed_length;
    return true;
  } else {
    //@todo 왜 문법 에러가 나는거냐
    //*out_error = String::Format(TEXT("zlib: %s"), A2T(zError(result)));
    *out_error = String::Format("zlib: error=%d", result);
    return false;
  }
}

bool Compressor::Zip_Decompress(uint8* out, int32* out_length, const uint8* in,
                                const int32 in_length, String* out_error) {
  // static int ZlibDecompress(Bytef* dst, uLongf* destLen, const Bytef* source,
  // uLong sourceLen)
  uLongf decompressed_length = (uLongf)*out_length;
  int result = ZlibDecompress((Bytef*)out, &decompressed_length,
                              (const Bytef*)in, (uLong)in_length);
  if (result == Z_OK) {
    *out_error = TEXT("");
    *out_length = (int32)decompressed_length;
    return true;
  } else {
    //@todo 왜 문법 에러가 나는거냐
    //*out_error = String::Format(TEXT("zlib: %s"), A2T(zError(result)));
    *out_error = String::Format("zlib: error=%d", result);
    return false;
  }
}
#endif

//
// Lzf
//

#if SUPPORT_LZF_COMPRESSOR
int32 Compressor::Lzf_GetMaxCompressedLength(int32 length) {
  return length +
         (length / 2);  //원래는 104% 정도면 된다고 하나 혹시 몰라서 크게 잡음..
}

bool Compressor::Lzf_Compress(uint8* out, int32* out_length, const uint8* in,
                              const int32 in_length, String* out_error) {
  fun_check(*out_length >= Lzf_GetMaxCompressedLength(in_length));
  if (*out_length < Lzf_GetMaxCompressedLength(in_length)) {
    *out_error = "lzf: buffer too short";
    return false;
  }

  *out_error = "";

  unsigned int compressed_length =
      lzf_compress((const void*)in, (unsigned int)in_length, (void*)out,
                   (unsigned int)*out_length);
  *out_length = (int32)compressed_length;
  return true;
}

bool Compressor::Lzf_Decompress(uint8* out, int32* out_length, const uint8* in,
                                const int32 in_length, String* out_error) {
  unsigned int decompressed_length =
      lzf_decompress((const void*)in, (unsigned int)in_length, (void*)out,
                     (unsigned int)*out_length);
  if (decompressed_length == 0) {
    *out_error = "lzf: decompression is failed";
    return false;
  }

  *out_error = "";
  *out_length = (int32)decompressed_length;
  return true;
}
#endif

//
// Snappy
//

#if SUPPORT_SNAPPY_COMPRESSOR
int32 Compressor::Snappy_GetMaxCompressedLength(int32 length) {
  return (int32)snappy_max_compressed_length(length);
}

bool Compressor::Snappy_Compress(uint8* out, int32* out_length, const uint8* in,
                                 const int32 in_length, String* out_error) {
  // Snappy 내부에서 에러 핸들링을 안하는듯 함. 고로 메모리 크래쉬가 나지 않는한
  // 무조건 성공한다는거임. 메모리 공간만 충분하면 그만임.
  fun_check(*out_length >= (int32)snappy_max_compressed_length(in_length));

  size_t compressed_length = (size_t)*out_length;
  snappy_status result = snappy_compress((const char*)in, (size_t)in_length,
                                         (char*)out, &compressed_length);
  switch (result) {
    case SNAPPY_OK:
      *out_error = "";
      *out_length = (int32)compressed_length;
      return true;
    case SNAPPY_INVALID_INPUT:
      *out_error = "snappy: invalid input";
      return false;
    case SNAPPY_BUFFER_TOO_SMALL:
      *out_error = "snappy: buffer too small";
      return false;
    default:
      *out_error =
          "snappy: unknown error(may be snappy sdk's version is changed?)";
      return false;
  }
}

bool Compressor::Snappy_Decompress(uint8* out, int32* out_length,
                                   const uint8* in, const int32 in_length,
                                   String* out_error) {
  size_t decompressed_length = (size_t)*out_length;
  snappy_status result = snappy_uncompress((const char*)in, (size_t)in_length,
                                           (char*)out, &decompressed_length);
  switch (result) {
    case SNAPPY_OK:
      *out_error = "";
      *out_length = (int32)decompressed_length;
      return true;
    case SNAPPY_INVALID_INPUT:
      *out_error = "snappy: invalid input";
      return false;
    case SNAPPY_BUFFER_TOO_SMALL:
      *out_error = "snappy: buffer too small";
      return false;
    default:
      *out_error =
          "snappy: unknown error(may be snappy sdk's version is changed?)";
      return false;
  }
}
#endif

int32 Compressor::GetMaxCompressedLength(CompressionMode compression_mode,
                                         const int32 length) {
  switch (compression_mode) {
    case CompressionMode::None:
      return Raw_GetMaxCompressedLength(length);
#if SUPPORT_ZIP_COMPRESSOR
    case CompressionMode::Zip:
      return Zip_GetMaxCompressedLength(length);
#endif
#if SUPPORT_LZF_COMPRESSOR
    case CompressionMode::Lzf:
      return Lzf_GetMaxCompressedLength(length);
#endif
#if SUPPORT_SNAPPY_COMPRESSOR
    case CompressionMode::Snappy:
      return Snappy_GetMaxCompressedLength(length);
#endif
    default:
      fun_check(0);
      return -1;
  }
}

bool Compressor::Compress(CompressionMode compression_mode, uint8* out,
                          int32* out_length, const uint8* in,
                          const int32 in_length, String* out_error) {
  switch (compression_mode) {
    case CompressionMode::None:
      return Raw_Compress(out, out_length, in, in_length, out_error);
#if SUPPORT_ZIP_COMPRESSOR
    case CompressionMode::Zip:
      return Zip_Compress(out, out_length, in, in_length, out_error);
#endif
#if SUPPORT_LZF_COMPRESSOR
    case CompressionMode::Lzf:
      return Lzf_Compress(out, out_length, in, in_length, out_error);
#endif
#if SUPPORT_SNAPPY_COMPRESSOR
    case CompressionMode::Snappy:
      return Snappy_Compress(out, out_length, in, in_length, out_error);
#endif
    default:
      fun_check(0);
      return false;
  }
}

bool Compressor::Decompress(CompressionMode compression_mode, uint8* out,
                            int32* out_length, const uint8* in,
                            const int32 in_length, String* out_error) {
  switch (compression_mode) {
    case CompressionMode::None:
      return Raw_Decompress(out, out_length, in, in_length, out_error);
#if SUPPORT_ZIP_COMPRESSOR
    case CompressionMode::Zip:
      return Zip_Decompress(out, out_length, in, in_length, out_error);
#endif
#if SUPPORT_LZF_COMPRESSOR
    case CompressionMode::Lzf:
      return Lzf_Decompress(out, out_length, in, in_length, out_error);
#endif
#if SUPPORT_SNAPPY_COMPRESSOR
    case CompressionMode::Snappy:
      return Snappy_Decompress(out, out_length, in, in_length, out_error);
#endif
    default:
      fun_check(0);
      return false;
  }
}

}  // namespace net
}  // namespace fun
