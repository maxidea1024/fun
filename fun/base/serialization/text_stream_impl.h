#pragma once

#include "fun/base/serialization/text_stream.h"

namespace fun {

// TODO unicode가 아닌 utf8인코딩을 기본으로 처리하는것은 어떨런지??
class TextStreamImpl {
 public:
  // Streaming parameters
  struct Params {
    int32 real_number_precision;
    int32 integer_base;
    int32 field_width;
    UNICHAR pad_char;
    TextStream::FieldAlignment field_alignment;
    TextStream::RealNumberNotation real_number_notation;
    TextStream::NumberFlags number_flags;
  };

  TextStreamImpl(TextStream* outer);
  ~TextStreamImpl();

  void Reset();

  // abstract device source
  IoDevice* device_;

  // string source
  UString string_;
  int32 string_offset_;
  IoDevice::OpenMode string_open_mode_;

  // encoding
  TextEncoding* encoding_;
  TextEncoding::ConverterState read_converter_state_;
  TextEncoding::ConverterState write_converter_state_;
  TextEncoding::ConverterState* read_converter_saved_state_;

  UString write_buffer_;
  UString read_buffer_;
  int32 read_buffer_offset_;
  int32 read_converter_saved_state_offset_;
  int64 read_buffer_start_device_pos_;

  Params params_;

  // status
  TextStream::Status status_;
  Locale locale_;
  TextStream* outer_;

  int32 last_token_len_;
  bool delete_device_;

  bool auto_detect_unicode_;

  // I/O
  enum TokenDelimiter { Space, NotSpace, EndOfLine };

  UString Read(int32 max_len);

  bool Scan(const UNICHAR** ptr, int32* token_length, int32 max_len,
            TokenDelimiter delimiter);

  const UNICHAR* ReadPtr() const;

  void ConsumeLastToken();

  void Consume(int32 len);

  void SaveConverterState(int64 new_pos);

  void RestoreToSavedConverterState();

  enum class NumberParsingStatus {
    Ok,
    MissingDigit,
    InvalidPrefix,
  };

  bool GetChar(UNICHAR* ch);
  void UngetChar(UNICHAR ch);
  NumberParsingStatus GetNumber(uint64* num);
  bool GetReal(double* real);

  void Write(const UString& str);
  void Write(UNICHAR ch);
  void Write(const UNICHAR* str, int32 len);
  void Write(Latin1String str);
  void WritePadding(int32 len);

  void PutString(const UString& str, bool is_number = false);
  void PutString(const UNITHCAR* str, int32 len, bool is_number = false);
  void PutString(Latin1String str, bool is_number = false);
  void PutChar(UNICHAR ch);
  void PutNumber(uint64 number, bool is_negative);

  struct PaddingResult {
    int32 left;
    int32 right;
  };
  PaddingResult Padding(int32 len) const;

  // buffers
  bool FillReadBuffer(int64 max_len = -1);
  void ResetReadBuffer();
  void FlushWriteBuffer();
};

//
// inlines
//

inline void TextStreamImpl::Write(const UString& str) {
  Write(str.ConstData(), str.Len());
}

inline void TextStreamImpl::PutString(const UString& str, bool is_number) {
  PutString(str.ConstData(), str.Len(), is_number);
}

}  // namespace fun
