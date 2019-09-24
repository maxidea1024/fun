#pragma once

#include "fun/base/base.h"
#include "fun/base/flags.h"
#include "fun/base/string/string.h"
#include "fun/base/container/array.h"

namespace fun {

/**
 * Text encoding.
 */
enum class TextFileEncoding {
  AutoDetect,
  ANSI,
  Local8Bit,
  Utf8,
  UTF16LE,
  UTF16BE,
  UTF32LE,
  UTF32BE,
  UTF8WithoutBOM,

  Default = Utf8,
};

/**
 * File helper functions.
 */
class FUN_BASE_API FileHelper {
 public:
  enum class HashOption {
    /** Enable the async task for verifying the hash for the file being loaded */
    EnableVerify = 0x1,

    /** A missing hash entry should trigger an error */
    ErrorMissingHash = 0x2,
  };
  FUN_DECLARE_FLAGS_IN_CLASS(HashOptions, HashOption);


  static bool ReadAllBytes( ByteArray& out_bytes,
                            const char* filename,
                            FileReadFlags flags = 0);
  static bool ReadAllBytes( Array<uint8>& out_bytes,
                            const char* filename,
                            FileReadFlags flags = 0);


  static bool WriteAllBytes(const ByteArray& bytes,
                            const char* filename,
                            IFileManager* fm = &IFileManager::Get(),
                            FileWriteFlags flags = 0);
  static bool WriteAllBytes(const Array<uint8>& bytes,
                            const char* filename,
                            IFileManager* fm = &IFileManager::Get(),
                            FileWriteFlags flags = 0);
  static bool WriteAllBytes(const void* bytes,
                            size_t length,
                            const char* filename,
                            IFileManager* fm = &IFileManager::Get(),
                            FileWriteFlags flags = 0);


  static bool BytesToString(String& out_string,
                            const ByteArray& bytes,
                            TextFileEncoding* out_detected_encoding = nullptr);
  static bool BytesToString(String& out_string,
                            const Array<uint8>& bytes,
                            TextFileEncoding* out_detected_encoding = nullptr);
  static bool BytesToString(String& out_string,
                            const void* bytes,
                            size_t length,
                            TextFileEncoding* out_detected_encoding = nullptr);


  static void StringToBytes(ByteArray& out_bytes,
                            const String& string,
                            TextFileEncoding encoding = TextFileEncoding::Default);
  static void StringToBytes(Array<uint8>& out_bytes,
                            const String& string,
                            TextFileEncoding encoding = TextFileEncoding::Default);


  static bool ReadAllText(String& out_text,
                          const char* filename,
                          HashOptions verify_flags = 0,
                          TextFileEncoding* out_encoding = nullptr);
  static bool ReadAllLines( Array<String>& out_lines,
                            const char* filename,
                            HashOptions verify_flags = 0,
                            StringSplitOptions split_options = StringSplitOption::None,
                            TextFileEncoding* out_encoding = nullptr);


  static bool WriteAllText( const String& Text,
                            const char* filename,
                            TextFileEncoding encoding = TextFileEncoding::Default,
                            IFileManager* fm = &IFileManager::Get(),
                            FileWriteFlags flags = 0);
  static bool WriteAllLines(const Array<String>& lines,
                            const char* filename,
                            TextFileEncoding encoding = TextFileEncoding::Default,
                            IFileManager* fm = &IFileManager::Get(),
                            FileWriteFlags flags = 0);

  static bool CreateBitmap( const char* pattern,
                            int32 data_width,
                            int32 data_height,
                            const class Color* data,
                            const class IntRect* sub_rect = nullptr,
                            IFileManager* fm = &IFileManager::Get(),
                            String* out_filename = nullptr,
                            bool write_alpha = false);
  static bool GenerateNextBitmapFilename( const String& pattern,
                                          const String& extension,
                                          String& out_filename,
                                          IFileManager* fm = &IFileManager::Get());
  static bool LoadANSITextFileToStrings(const char* filename,
                                        IFileManager* fm,
                                        Array<String>& out_strings);
};

} // namespace fun
