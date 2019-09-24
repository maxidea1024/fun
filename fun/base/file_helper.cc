#include "fun/base/file_helper.h"

// Unicode BOM
//--------------------------------------------------------
//  bytes        encoding form
//  00 00 FE FF  UTF - 32, big - endian
//  FF FE 00 00  UTF - 32, little - endian
//  FE FF        UTF - 16, big - endian
//  FF FE        UTF - 16, little - endian
//  EF BB BF     UTF - 8

namespace fun {

static const uint8 UTF32BE_BOM[] = {0x00, 0x00, 0xFE, 0xFF};
static const uint8 UTF32LE_BOM[] = {0xFF, 0xFE, 0x00, 0x00};
static const uint8 UTF16BE_BOM[] = {0xFE, 0xFF};
static const uint8 UTF16LE_BOM[] = {0xFF, 0xFE};
static const uint8 UTF8_BOM[] = {0xEF, 0xBB, 0xBF};

bool FileHelper::ReadAllBytes(ByteArray& out_bytes, const char* filename,
                              FileReadFlags flags) {
  UniquePtr<Archive> reader(
      IFileManager::Get().CreateFileReader(filename, flags));
  if (!reader.IsValid()) {
    if (!(flags & FileReadFlag::Silent)) {
      // TODO
      // fun_log(LogStreaming, Warning, "Failed to read file '{0}' error.",
      // filename);
    }
    return false;  // 예외를 던지지 않음.
  }

  const int64 file_size = reader->TotalSize();
  out_bytes.ResizeUninitialized(file_size);
  reader->Serialize(out_bytes.MutableData(), out_bytes.Len());
  return reader->Close();
}

bool FileHelper::ReadAllBytes(Array<uint8>& out_bytes, const char* filename,
                              FileReadFlags flags) {
  UniquePtr<Archive> reader(
      IFileManager::Get().CreateFileReader(filename, flags));
  if (!reader.IsValid()) {
    if (!(flags & FileReadFlag::Silent)) {
      // TODO
      // fun_log(LogStreaming, Warning, "Failed to read file '{0}' error.",
      // filename);
    }
    return false;  // 예외를 던지지 않음.
  }

  const int64 file_size = reader->TotalSize();
  out_bytes.ResizeUninitialized(file_size);
  reader->Serialize(out_bytes.MutableData(), out_bytes.Count());
  return reader->Close();
}

bool FileHelper::WriteAllBytes(const ByteArray& bytes, const char* filename,
                               IFileManager* fm, FileWriteFlags flags) {
  return WriteAllBytes((const uint8*)bytes.ConstData(), bytes.Len(), filename,
                       fm, flags);
}

bool FileHelper::WriteAllBytes(const Array<uint8>& bytes, const char* filename,
                               IFileManager* fm, FileWriteFlags flags) {
  return WriteAllBytes(bytes.ConstData(), bytes.Count(), filename, fm, flags);
}

bool FileHelper::WriteAllBytes(const uint8* bytes, size_t length,
                               const char* filename, IFileManager* fm,
                               FileWriteFlags flags) {
  UniquePtr<Archive> writer(fm->CreateFileWriter(filename, flags));
  if (writer.IsValid()) {
    writer->Serialize(const_cast<uint8*>(bytes), length);  // mutable casting.
    return true;
  }
  return false;
}

bool FileHelper::BytesToString(String& out_string, const ByteArray& bytes,
                               TextFileEncoding* out_detected_encoding) {
  return BytesToString(out_string, (const uint8*)bytes.ConstData(), bytes.Len(),
                       out_detected_encoding);
}

bool FileHelper::BytesToString(UString& out_string, const Array<uint8>& bytes,
                               TextFileEncoding* out_detected_encoding) {
  return BytesToString(out_string, bytes.ConstData(), bytes.Count(),
                       out_detected_encoding);
}

bool FileHelper::BytesToString(UString& out_string, const uint8* bytes,
                               int32 length,
                               TextFileEncoding* out_detected_encoding) {
  if (out_detected_encoding) {
    *out_detected_encoding = TextFileEncoding::UTF8WithoutBOM;
  }

  out_string.Clear();

  if (length >= 4 && !(length & 3) &&
      UnsafeMemory::Memcmp(bytes, UTF32LE_BOM, sizeof(UTF32LE_BOM)) ==
          0) {  // UTF32LE
    if (out_detected_encoding) {
      *out_detected_encoding = TextFileEncoding::UTF32LE;
    }

    // Skip over UTF-32 BOM if there is one
    bytes += 4;
    length -= 4;

    const int32 utf32_len = length / 4;

#if FUN_ARCH_LITTLE_ENDIAN
    const int32 utf16_len =
        Utf::LengthAsUtf16((const UTF32CHAR*)bytes, utf32_len);
    out_string.ResizeUninitialized(utf16_len);
    Utf::Convert((const UTF32CHAR*)bytes, utf32_len, out_string.MutableData(),
                 out_string.Len());
#else
    Array<UTF32CHAR, InlineAllocator<512>> utf32_buf(utf32_len, NoInit);
    UTF32CHAR* dst = utf32_buf.MutableData();
    for (int32 i = 0; i < utf32_len; ++i) {
      *dst = UTF32CHAR(bytes[0]) << 24;
      *dst |= UTF32CHAR(bytes[1]) << 16;
      *dst |= UTF32CHAR(bytes[2]) << 8;
      *dst |= UTF32CHAR(bytes[3]);

      bytes += 4;
      ++dst;
    }

    const int32 utf16_len =
        Utf::LengthAsUtf16(utf32_buf.ConstData(), utf32_buf.Count());
    out_string.Resize(utf16_len);
    Utf::Convert(utf32_buf.ConstData(), utf32_buf.Count(),
                 out_string.MutableData(), out_string.Len());
#endif
  } else if (length >= 4 && !(length & 3) &&
             UnsafeMemory::Memcmp(bytes, UTF32BE_BOM, sizeof(UTF32BE_BOM)) ==
                 0) {  // UTF32BE
    if (out_detected_encoding) {
      *out_detected_encoding = TextFileEncoding::UTF32BE;
    }

    // Skip over UTF-32 BOM if there is one
    bytes += 4;
    length -= 4;

    const int32 utf32_len = length / 4;

#if FUN_ARCH_LITTLE_ENDIAN
    Array<UTF32CHAR, InlineAllocator<512>> utf32_buf(utf32_len, NoInit);
    UTF32CHAR* dst = utf32_buf.MutableData();
    for (int32 i = 0; i < utf32_len; ++i) {
      *dst = UTF32CHAR(bytes[0]) << 24;
      *dst |= UTF32CHAR(bytes[1]) << 16;
      *dst |= UTF32CHAR(bytes[2]) << 8;
      *dst |= UTF32CHAR(bytes[3]);

      bytes += 4;
      ++dst;
    }

    const int32 utf16_len =
        Utf::LengthAsUtf16(utf32_buf.ConstData(), utf32_buf.Count());
    out_string.ResizeUninitialized(utf16_len);
    Utf::Convert(utf32_buf.ConstData(), utf32_buf.Count(),
                 out_string.MutableData(), out_string.Len());
#else
    const int32 utf16_len =
        Utf::LengthAsUtf16((const UTF32CHAR*)bytes, utf32_len);
    out_string.Resize(utf16_len);
    Utf::Convert((const UTF32CHAR*)bytes, utf32_len, out_string.MutableData(),
                 out_string.Len());
#endif
  } else if (length >= 2 && !(length & 1) &&
             UnsafeMemory::Memcmp(bytes, UTF16LE_BOM, sizeof(UTF16LE_BOM)) ==
                 0) {  // UTF16LE
    if (out_detected_encoding) {
      *out_detected_encoding = TextFileEncoding::UTF16LE;
    }

    // Skip over UTF-16 BOM if there is one
    bytes += 2;
    length -= 2;

    const int32 char_count = length / 2;
    out_string.ResizeUninitialized(char_count);

    TCHAR* p = out_string.MutableData();
    for (int32 i = 0; i < char_count * 2; i += 2) {
      *p++ = CharCast<TCHAR>(
          (UCS2CHAR)((uint16)bytes[i] + (uint16)bytes[i + 1] * 256));
    }
  } else if (length >= 2 && !(length & 1) &&
             UnsafeMemory::Memcmp(bytes, UTF16BE_BOM, sizeof(UTF16BE_BOM)) ==
                 0) {  // UTF16BE
    if (out_detected_encoding) {
      *out_detected_encoding = TextFileEncoding::UTF16BE;
    }

    // Skip over UTF-16 BOM if there is one
    bytes += 2;
    length -= 2;

    const int32 char_count = length / 2;
    out_string.ResizeUninitialized(char_count);

    TCHAR* p = out_string.MutableData();
    for (int32 i = 0; i < char_count * 2; i += 2) {
      *p++ = CharCast<TCHAR>(
          (UCS2CHAR)((uint16)bytes[i + 1] + (uint16)bytes[i] * 256));
    }
  } else if (length >= 3 &&
             UnsafeMemory::Memcmp(bytes, UTF8_BOM, sizeof(UTF8_BOM)) ==
                 0) {  // Utf8
    if (out_detected_encoding) {
      *out_detected_encoding = TextFileEncoding::Utf8;
    }

    // Skip over UTF-8 BOM if there is one
    bytes += 3;
    length -= 3;

    // Utf8 -> TCHAR
    // 현재, TCHAR는 UTF16LE라고 암묵적으로 가정하고 있는 차후에 시스템
    // 전반적으로 정리가 필요해보임. UCS2를 사용하는 윈도우즈에서는 아무런
    // 문제가 되지 않으나, wchar_t가 32비트인 unix계열에서는 문제가 발생함.
    const int32 utf16_len = Utf::LengthAsUtf16(bytes, length);
    out_string.ResizeUninitialized(utf16_len);
    Utf::Convert(bytes, length, out_string.MutableData(), out_string.Len());
  } else {  // ANSI (UTF8을 기본으로 하는게 좋으려나??)
    // TODO ANSI 보다는 locale 코덱으로 처리하는게 자연스러울듯 싶기도...

    if (out_detected_encoding) {
      *out_detected_encoding = TextFileEncoding::ANSI;
    }

    // PURE ANSI
    // 단순히 캐스팅을 하므로, ASCII외에는 모조리 깨짐.
    // out_string.Resize(length);
    // TCHAR* p = out_string.MutableData();
    // for (int32 i = 0; i < length; ++i) {
    //  *p++ = TCHAR(*bytes++);
    //}

    const int32 utf16_len =
        MultiByteToWideChar(CP_ACP, 0, (char*)bytes, length, nullptr, 0);
    out_string.ResizeUninitialized(utf16_len);
    MultiByteToWideChar(CP_ACP, 0, (char*)bytes, length,
                        out_string.MutableData(), out_string.Len());
  }

  return false;
}

// TODO 공용함수를 하나 빼주는게 좋을듯함...
void FileHelper::StringToBytes(ByteArray& out_bytes, const UString& String,
                               TextFileEncoding encoding) {
  if (encoding == TextFileEncoding::AutoDetect) {
    const bool is_pure_ansi = string.IndexOfIf([](TCHAR Ch) -> bool {
      return Ch > 0x7F;
    }) == INVALID_INDEX;
    encoding =
        is_pure_ansi ? TextFileEncoding::ANSI : TextFileEncoding::UTF16LE;
  }

  out_bytes.Clear();

  switch (encoding) {
    case TextFileEncoding::ANSI: {
      // out_bytes.Resize(string.Len());
      // const TCHAR* src = string.ConstData();
      // char* dst = out_bytes.MutableData();
      // for (int32 i = 0; i < string.Len(); ++i) {
      //  *dst++ = char(*src++);
      //}

      const int32 mbcs_len =
          WideCharToMultiByte(CP_ACP, 0, string.ConstData(), string.Len(),
                              nullptr, 0, nullptr, nullptr);
      out_bytes.ResizeUninitialized(mbcs_len);
      WideCharToMultiByte(CP_ACP, 0, string.ConstData(), string.Len(),
                          out_bytes.MutableData(), out_bytes.Len(), nullptr,
                          nullptr);
      break;
    }

    case TextFileEncoding::UTF16LE: {
      const int32 BOM_SIZE = sizeof(UTF16LE_BOM);
      out_bytes.ResizeUninitialized(string.Len() * 2 + BOM_SIZE);

      char* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF16LE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const TCHAR* src = string.ConstData();
      const TCHAR* src_end = string.ConstData() + string.Len();
      for (; src != src_end; ++src) {
        *dst++ = char(*src & 0xFF);
        *dst++ = char(*src >> 8);
      }
      break;
    }

    case TextFileEncoding::UTF16BE: {
      const int32 BOM_SIZE = sizeof(UTF16BE_BOM);
      out_bytes.ResizeUninitialized(string.Len() * 2 + BOM_SIZE);

      char* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF16BE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const TCHAR* src = string.ConstData();
      const TCHAR* src_end = string.ConstData() + string.Len();
      for (; src != src_end; ++src) {
        *dst++ = char(*src >> 8);
        *dst++ = char(*src & 0xFF);
      }
      break;
    }

    case TextFileEncoding::UTF32LE: {
      const int32 utf32_len =
          Utf::LengthAsUtf32(string.ConstData(), string.Len());
      Array<UTF32CHAR> utf32_buf(utf32_len, NoInit);
      Utf::Convert(string.ConstData(), string.Len(), utf32_buf.MutableData(),
                   utf32_buf.Count());

      const int32 BOM_SIZE = sizeof(UTF32LE_BOM);
      out_bytes.ResizeUninitialized(utf32_len * 4 + BOM_SIZE);

      char* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF32LE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const UTF32CHAR* src = utf32_buf.ConstData();
      const UTF32CHAR* src_end = utf32_buf.ConstData() + utf32_buf.Count();
      for (; src != src_end; ++src) {
        *dst++ = char(*src & 0xFF);
        *dst++ = char((*src >> 8) & 0xFF);
        *dst++ = char((*src >> 16) & 0xFF);
        *dst++ = char((*src >> 24) & 0xFF);
      }
      break;
    }

    case TextFileEncoding::UTF32BE: {
      const int32 utf32_len =
          Utf::LengthAsUtf32(string.ConstData(), string.Len());
      Array<UTF32CHAR> utf32_buf(utf32_len, NoInit);
      Utf::Convert(string.ConstData(), string.Len(), utf32_buf.MutableData(),
                   utf32_buf.Count());

      const int32 BOM_SIZE = sizeof(UTF32BE_BOM);
      out_bytes.ResizeUninitialized(utf32_len * 4 + BOM_SIZE);

      char* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF32BE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const UTF32CHAR* src = utf32_buf.ConstData();
      const UTF32CHAR* src_end = utf32_buf.ConstData() + utf32_buf.Count();
      for (; src != src_end; ++src) {
        *dst++ = char((*src >> 24) & 0xFF);
        *dst++ = char((*src >> 16) & 0xFF);
        *dst++ = char((*src >> 8) & 0xFF);
        *dst++ = char(*src & 0xFF);
      }
      break;
    }

    case TextFileEncoding::Utf8:
    case TextFileEncoding::UTF8WithoutBOM: {
      const int32 BOM_SIZE =
          encoding == TextFileEncoding::Utf8 ? sizeof(UTF8_BOM) : 0;
      const int32 utf8_len =
          Utf::LengthAsUtf8(string.ConstData(), string.Len());
      out_bytes.ResizeUninitialized(utf8_len + BOM_SIZE);

      char* dst = out_bytes.MutableData();

      // BOM(optional)
      if (BOM_SIZE) {
        UnsafeMemory::Memcpy(dst, UTF8_BOM, BOM_SIZE);
        dst += BOM_SIZE;
      }

      Utf::Convert(string.ConstData(), string.Len(), dst,
                   out_bytes.Len() - BOM_SIZE);
      break;
    }

    default:
      // unreachable to here...
      CHECK(0);
      break;
  }
}

void FileHelper::StringToBytes(Array<uint8>& out_bytes, const UString& string,
                               TextFileEncoding encoding) {
  // TODO
  //코드 중복을 피할 수 없을까??

  if (encoding == TextFileEncoding::AutoDetect) {
    const bool is_pure_ansi = string.IndexOfByIf([](TCHAR Ch) -> bool {
      return Ch > 0x7F;
    }) == INVALID_INDEX;
    encoding =
        is_pure_ansi ? TextFileEncoding::ANSI : TextFileEncoding::UTF16LE;
  }

  out_bytes.Clear();

  switch (encoding) {
    case TextFileEncoding::ANSI: {
      // out_bytes.AddUninitialized(string.Len());
      // const TCHAR* src = string.ConstData();
      // uint8* dst = out_bytes.MutableData();
      // for (int32 i = 0; i < string.Len(); ++i) {
      //  *dst++ = uint8(*src++);
      //}

      const int32 mbcs_len =
          WideCharToMultiByte(CP_ACP, 0, string.ConstData(), string.Len(),
                              nullptr, 0, nullptr, nullptr);
      out_bytes.AddUninitialized(mbcs_len);
      WideCharToMultiByte(CP_ACP, 0, string.ConstData(), string.Len(),
                          (char*)out_bytes.MutableData(), out_bytes.Count(),
                          nullptr, nullptr);
      break;
    }

    case TextFileEncoding::UTF16LE: {
      const int32 BOM_SIZE = sizeof(UTF16LE_BOM);
      out_bytes.AddUninitialized(string.Len() * 2 + BOM_SIZE);

      uint8* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF16LE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const TCHAR* src = string.ConstData();
      const TCHAR* src_end = string.ConstData() + string.Len();
      for (; src != src_end; ++src) {
        *dst++ = uint8(*src & 0xFF);
        *dst++ = uint8(*src >> 8);
      }
      break;
    }

    case TextFileEncoding::UTF16BE: {
      const int32 BOM_SIZE = sizeof(UTF16BE_BOM);
      out_bytes.AddUninitialized(string.Len() * 2 + BOM_SIZE);

      uint8* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF16BE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const TCHAR* src = string.ConstData();
      const TCHAR* src_end = string.ConstData() + string.Len();
      for (; src != src_end; ++src) {
        *dst++ = uint8(*src >> 8);
        *dst++ = uint8(*src & 0xFF);
      }
      break;
    }

    case TextFileEncoding::UTF32LE: {
      const int32 utf32_len =
          Utf::LengthAsUtf32(string.ConstData(), string.Len());
      Array<UTF32CHAR> utf32_buf(utf32_len, NoInit);
      Utf::Convert(string.ConstData(), string.Len(), utf32_buf.MutableData(),
                   utf32_buf.Count());

      const int32 BOM_SIZE = sizeof(UTF32LE_BOM);
      out_bytes.AddUninitialized(utf32_len * 4 + BOM_SIZE);

      uint8* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF32LE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const UTF32CHAR* src = utf32_buf.ConstData();
      const UTF32CHAR* src_end = utf32_buf.ConstData() + utf32_buf.Count();
      for (; src != src_end; ++src) {
        *dst++ = uint8(*src & 0xFF);
        *dst++ = uint8((*src >> 8) & 0xFF);
        *dst++ = uint8((*src >> 16) & 0xFF);
        *dst++ = uint8((*src >> 24) & 0xFF);
      }
      break;
    }

    case TextFileEncoding::UTF32BE: {
      const int32 utf32_len =
          Utf::LengthAsUtf32(string.ConstData(), string.Len());
      Array<UTF32CHAR> utf32_buf(utf32_len, NoInit);
      Utf::Convert(string.ConstData(), string.Len(), utf32_buf.MutableData(),
                   utf32_buf.Count());

      const int32 BOM_SIZE = sizeof(UTF32BE_BOM);
      out_bytes.AddUninitialized(utf32_len * 4 + BOM_SIZE);

      uint8* dst = out_bytes.MutableData();

      // BOM
      UnsafeMemory::Memcpy(dst, UTF32BE_BOM, BOM_SIZE);
      dst += BOM_SIZE;

      const UTF32CHAR* src = utf32_buf.ConstData();
      const UTF32CHAR* src_end = utf32_buf.ConstData() + utf32_buf.Count();
      for (; src != src_end; ++src) {
        *dst++ = uint8((*src >> 24) & 0xFF);
        *dst++ = uint8((*src >> 16) & 0xFF);
        *dst++ = uint8((*src >> 8) & 0xFF);
        *dst++ = uint8(*src & 0xFF);
      }
      break;
    }

    case TextFileEncoding::Utf8:
    case TextFileEncoding::UTF8WithoutBOM: {
      const int32 BOM_SIZE =
          encoding == TextFileEncoding::Utf8 ? sizeof(UTF8_BOM) : 0;
      const int32 utf8_len =
          Utf::LengthAsUtf8(string.ConstData(), string.Len());
      out_bytes.AddUninitialized(utf8_len + BOM_SIZE);

      uint8* dst = out_bytes.MutableData();

      // BOM(optional)
      if (BOM_SIZE) {
        UnsafeMemory::Memcpy(dst, UTF8_BOM, BOM_SIZE);
        dst += BOM_SIZE;
      }

      Utf::Convert(string.ConstData(), string.Len(), dst,
                   out_bytes.Count() - BOM_SIZE);
      break;
    }

    default:
      // unreachable to here...
      CHECK(0);
      break;
  }
}

bool FileHelper::ReadAllText(UString& out_text, const char* filename,
                             HashOptions verify_flags,
                             TextFileEncoding* out_encoding) {
  ByteArray bytes;
  if (!ReadAllBytes(bytes, filename)) {
    return false;
  }

  // TODO secured 텍스트를 읽어들이는 코드를 추가해주어야함.

  return BytesToString(out_text, (const uint8*)bytes.ConstData(), bytes.Len(),
                       out_encoding);
}

bool FileHelper::ReadAllLines(Array<UString>& out_lines, const char* filename,
                              HashOptions verify_flags,
                              StringSplitOptions split_options,
                              TextFileEncoding* out_encoding) {
  UString text;
  if (!ReadAllText(text, filename, verify_flags, out_encoding)) {
    return false;
  }

  text.SplitLines(out_lines, split_options);
  return true;
}

bool FileHelper::WriteAllText(const UString& text, const char* filename,
                              TextFileEncoding encoding, IFileManager* fm,
                              FileWriteFlags flags) {
  ByteArray bytes;
  StringToBytes(bytes, text, encoding);
  return WriteAllBytes(bytes, filename, fm, flags);
}

bool FileHelper::WriteAllLines(const Array<UString>& lines,
                               const char* filename, TextFileEncoding encoding,
                               IFileManager* fm, FileWriteFlags flags) {
  const int32 LINE_TERMINATOR_LENGTH =
      CStringTraitsA::Strlen(FUN_LINE_TERMINATOR);

  int32 required_buffer_len = 0;
  for (const auto& line : lines) {
    required_buffer_len += line.Len() + LINE_TERMINATOR_LENGTH;
  }

  UString text;
  text.Reserve(required_buffer_len);
  for (const auto& line : lines) {
    text += line;
    text += FUN_LINE_TERMINATOR;
  }

  return WriteAllText(text, filename, encoding, fm, flags);
}

//
//
//

bool FileHelper::CreateBitmap(const char* pattern, int32 data_width,
                              int32 data_height, const class Color* data,
                              const class IntRect* sub_rect, IFileManager* fm,
                              UString* out_filename, bool write_alpha) {
#if ALLOW_DEBUG_FILES
  IntRect src(0, 0, data_width, data_height);
  if (sub_rect == nullptr || sub_rect->Area() == 0) {
    sub_rect = &src;
  }

  UString filename;
  // if the pattern already has a .bmp extension, then use that the file to
  // write to
  if (CPaths::GetExtension(pattern) == TEXT("bmp")) {
    filename = pattern;
  } else {
    if (GenerateNextBitmapFilename(pattern, TEXT("bmp"), filename, fm)) {
      if (out_filename) {
        *out_filename = filename;
      }
    } else {
      return false;
    }
  }

  UniquePtr<Archive> ar(fm->CreateDebugFileWriter(*filename));
  if (ar.IsValid()) {
    // Types.
#pragma pack(push, 1)
    struct BITMAPFILEHEADER {
      uint16 bfType GCC_PACK(1);
      uint32 bfSize GCC_PACK(1);
      uint16 bfReserved1 GCC_PACK(1);
      uint16 bfReserved2 GCC_PACK(1);
      uint32 bfOffBits GCC_PACK(1);
    } fh;

    struct BITMAPINFOHEADER {
      uint32 biSize GCC_PACK(1);
      int32 biWidth GCC_PACK(1);
      int32 biHeight GCC_PACK(1);
      uint16 biPlanes GCC_PACK(1);
      uint16 biBitCount GCC_PACK(1);
      uint32 biCompression GCC_PACK(1);
      uint32 biSizeImage GCC_PACK(1);
      int32 biXPelsPerMeter GCC_PACK(1);
      int32 biYPelsPerMeter GCC_PACK(1);
      uint32 biClrUsed GCC_PACK(1);
      uint32 biClrImportant GCC_PACK(1);
    } ih;

    struct BITMAPV4HEADER {
      uint32 bV4RedMask GCC_PACK(1);
      uint32 bV4GreenMask GCC_PACK(1);
      uint32 bV4BlueMask GCC_PACK(1);
      uint32 bV4AlphaMask GCC_PACK(1);
      uint32 bV4CSType GCC_PACK(1);
      uint32 bV4EndpointR[3] GCC_PACK(1);
      uint32 bV4EndpointG[3] GCC_PACK(1);
      uint32 bV4EndpointB[3] GCC_PACK(1);
      uint32 bV4GammaRed GCC_PACK(1);
      uint32 bV4GammaGreen GCC_PACK(1);
      uint32 bV4GammaBlue GCC_PACK(1);
    } ih_v4;
#pragma pack(pop)

    int32 width = sub_rect->Width();
    int32 height = sub_rect->Height();
    uint32 bytes_per_pixel = write_alpha ? 4 : 3;
    uint32 bytes_per_line = Align(width * bytes_per_pixel, 4);

    uint32 info_header_size =
        sizeof(BITMAPINFOHEADER) + (write_alpha ? sizeof(BITMAPV4HEADER) : 0);

    // filename header.
    fh.bfType = ByteOrder::ToLittleEndian((uint16)('B' + 256 * 'M'));
    fh.bfSize = ByteOrder::ToLittleEndian((uint32)(
        sizeof(BITMAPFILEHEADER) + info_header_size + bytes_per_line * height));
    fh.bfReserved1 = ByteOrder::ToLittleEndian((uint16)0);
    fh.bfReserved2 = ByteOrder::ToLittleEndian((uint16)0);
    fh.bfOffBits = ByteOrder::ToLittleEndian(
        (uint32)(sizeof(BITMAPFILEHEADER) + info_header_size));
    ar->Serialize(&fh, sizeof(fh));

    // Info header.
    ih.biSize = ByteOrder::ToLittleEndian((uint32)info_header_size);
    ih.biWidth = ByteOrder::ToLittleEndian((uint32)width);
    ih.biHeight = ByteOrder::ToLittleEndian((uint32)height);
    ih.biPlanes = ByteOrder::ToLittleEndian((uint16)1);
    ih.biBitCount = ByteOrder::ToLittleEndian((uint16)bytes_per_pixel * 8);
    if (write_alpha) {
      ih.biCompression = ByteOrder::ToLittleEndian((uint32)3);  // BI_BITFIELDS
    } else {
      ih.biCompression = ByteOrder::ToLittleEndian((uint32)0);  // BI_RGB
    }
    ih.biSizeImage = ByteOrder::ToLittleEndian((uint32)bytes_per_line * height);
    ih.biXPelsPerMeter = ByteOrder::ToLittleEndian((uint32)0);
    ih.biYPelsPerMeter = ByteOrder::ToLittleEndian((uint32)0);
    ih.biClrUsed = ByteOrder::ToLittleEndian((uint32)0);
    ih.biClrImportant = ByteOrder::ToLittleEndian((uint32)0);
    ar->Serialize(&ih, sizeof(ih));

    // If we're writing alpha, we need to write the extra portion of the V4
    // header
    if (write_alpha) {
      ih_v4.bV4RedMask = ByteOrder::ToLittleEndian((uint32)0x00ff0000);
      ih_v4.bV4GreenMask = ByteOrder::ToLittleEndian((uint32)0x0000ff00);
      ih_v4.bV4BlueMask = ByteOrder::ToLittleEndian((uint32)0x000000ff);
      ih_v4.bV4AlphaMask = ByteOrder::ToLittleEndian((uint32)0xff000000);
      ih_v4.bV4CSType = ByteOrder::ToLittleEndian((uint32)'Win ');
      ih_v4.bV4GammaRed = ByteOrder::ToLittleEndian((uint32)0);
      ih_v4.bV4GammaGreen = ByteOrder::ToLittleEndian((uint32)0);
      ih_v4.bV4GammaBlue = ByteOrder::ToLittleEndian((uint32)0);
      ar->Serialize(&ih_v4, sizeof(ih_v4));
    }

    // Colors.
    for (int32 i = sub_rect->max.y - 1; i >= sub_rect->min.y; --i) {
      for (int32 j = sub_rect->min.x; j < sub_rect->max.x; ++j) {
        ar->Serialize((void*)&data[i * data_width + j].b, 1);
        ar->Serialize((void*)&data[i * data_width + j].g, 1);
        ar->Serialize((void*)&data[i * data_width + j].r, 1);

        if (write_alpha) {
          ar->Serialize((void*)&data[i * data_width + j].a, 1);
        }
      }

      // Pad each row's length to be a multiple of 4 bytes.

      for (uint32 pad_index = width * bytes_per_pixel;
           pad_index < bytes_per_line; ++pad_index) {
        uint8 b = 0;
        ar->Serialize(&b, 1);
      }
    }

    // Success.
    if (!g_is_editor) {
      SendDataToPCViaFunConsole("FUN_PROFILER!BUGIT:", filename);
    }
  } else {
    return false;
  }
#endif  // ALLOW_DEBUG_FILES

  // Success.
  return true;
}

bool FileHelper::GenerateNextBitmapFilename(const String& pattern,
                                            const String& extension,
                                            String& out_filename,
                                            IFileManager* fm) {
  out_filename = "";

  UString filename;
  bool ok = false;

  for (int32 test_bitmap_index = g_last_screenshot_bitmap_index + 1;
       test_bitmap_index < 100000; ++test_bitmap_index) {
    filename = UString::Format(TEXT("%s%05i.%s"), *pattern, test_bitmap_index,
                               *extension);
    if (fm->FileSize(*filename) < 0) {
      g_last_screenshot_bitmap_index = test_bitmap_index;
      out_filename = filename;
      ok = true;
      break;
    }
  }

  return ok;
}

// TODO 필요에 의해서 만들어지긴 했겠지만, 좀 이쁘지 않은...
bool FileHelper::LoadANSITextFileToStrings(const char* filename,
                                           IFileManager* fm,
                                           Array<UString>& out_strings) {
  IFileManager* file_manager = fm ? fm : &IFileManager::Get();

  // Read and parse the file, adding the pawns and their sounds to the list
  UniquePtr<Archive> text_file(file_manager->CreateFileReader(filename, 0));
  if (text_file.IsValid()) {
    // get the size of the file
    int64 size = text_file->TotalSize();
    // read the file
    Array<uint8> buffer(size, NoInit);
    text_file->Serialize(buffer.MutableData(), size);
    // zero terminate it
    buffer.Add(0);

    // Now read it
    // init traveling pointer
    ANSICHAR* ptr = (ANSICHAR*)buffer.MutableData();

    // iterate over the lines until complete
    bool is_done = false;
    while (!is_done) {
      // Store the location of the first character of this line
      ANSICHAR* start = ptr;

      // Advance the char pointer until we hit a newline character
      while (*ptr && *ptr != '\r' && *ptr != '\n') {
        ptr++;
      }

      // If this is the end of the file, we're done
      if (*ptr == 0) {
        is_done = 1;
      }
      // Handle different line endings.
      // If \r\n then NULL and advance 2, otherwise NULL and advance 1
      // This handles \r, \n, or \r\n
      else if (*ptr == '\r' && *(ptr + 1) == '\n') {
        // This was \r\n.
        // Terminate the current line, and advance the pointer forward 2
        // characters in the stream
        *ptr++ = 0;
        *ptr++ = 0;
      } else {
        // Terminate the current line, and advance the pointer to the next
        // character in the stream
        *ptr++ = 0;
      }

      UString cur_line = ANSI_TO_TCHAR(start);
      out_strings.Add(cur_line);
    }

    return true;
  } else {
    // TODO
    // fun_log(LogStreaming, Warning, "Failed to open ANSI TEXT file {0}",
    // filename);
    return false;
  }
}

}  // namespace fun
