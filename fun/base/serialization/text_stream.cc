#include "fun/base/serialization/text_stream_impl.h"

#if defined(FUN_TEXTSTREAM_DEBUG)

#include <ctype.h>
#include "private/qtools_p.h"

namespace fun {

// Returns a human readable representation of the first \a len
// characters in \a data.
static QByteArray qt_prettyDebug(const char* data, int len, int maxSize) {
  if (!data) return "(null)";

  QByteArray out;
  for (int i = 0; i < len; ++i) {
    char c = data[i];
    if (isprint(int(uchar(c)))) {
      out += c;
    } else
      switch (c) {
        case '\n':
          out += "\\n";
          break;
        case '\r':
          out += "\\r";
          break;
        case '\t':
          out += "\\t";
          break;
        default: {
          const char buf[] = {'\\', 'x', QtMiscUtils::toHexLower(uchar(c) / 16),
                              QtMiscUtils::toHexLower(uchar(c) % 16), 0};
          out += buf;
        }
      }
  }

  if (len < maxSize) {
    out += "...";
  }

  return out;
}

}  // namespace fun

#endif  // defined(FUN_TEXTSTREAM_DEBUG)

namespace fun {

// A precondition macro
#define FUN_VOID

#define CHECK_VALID_STREAM(rv)                    \
  do {                                            \
    if (!impl_->string_ && !impl_->device_) {     \
      fun_log(Warning, "QTextStream: No device"); \
      return rv;                                  \
    }                                             \
  } while (0);

// Base implementations of operator>> for ints and reals
#define IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(type)                 \
  do {                                                            \
    CHECK_VALID_STREAM(*this);                                    \
    uint64 tmp;                                                   \
    switch (impl_->GetNumber(&tmp)) {                             \
      case TextStreamImpl::NumberParsingStatus::Ok:               \
        i = (type)tmp;                                            \
        break;                                                    \
      case TextStreamImpl::NumberParsingStatus::MissingDigit:     \
      case TextStreamImpl::NumberParsingStatus::InvalidPrefix:    \
        i = (type)0;                                              \
        SetStatus(AtEnd() ? TextStream::Status::ReadPastEnd       \
                          : TextStream::Status::ReadCorruptData); \
        break;                                                    \
    }                                                             \
    return *this;                                                 \
  } while (0);

#define IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(type)              \
  do {                                                          \
    CHECK_VALID_STREAM(*this);                                  \
    double tmp;                                                 \
    if (impl_->GetReal(&tmp)) {                                 \
      f = (type)tmp;                                            \
    } else {                                                    \
      f = (type)0;                                              \
      SetStatus(AtEnd() ? TextStream::Status::ReadPastEnd       \
                        : TextStream::Status::ReadCorruptData); \
    }                                                           \
    return *this;                                               \
  } while (0);

static const int32 FUN_TEXTSTREAM_BUFFERSIZE = 16 * 1024;  // 16kb

TextStreamImpl::TextStreamImpl(TextStream* outer)
    : read_converter_saved_state_(nullptr),
      read_converter_saved_state_offset_(0),
      locale_(Locale::C()),
      outer_(outer) {
  Reset();
}

TextStreamImpl::~TextStreamImpl() {
  if (delete_device_) {
    delete device_;
  }

  delete read_converter_saved_state_;
}

static void ResetCodecConverterStateHelper(
    TextEncoding::ConverterState* state) {
  state->~ConverterState();
  new (state) TextEncoding::ConverterState;
}

static void CopyConverterStateHelper(TextEncoding::ConverterState* dst,
                                     const TextEncoding::ConverterState* src) {
  // ### TextEncoding::ConverterState's copy constructors and assignments are
  // private. This function copies the structure manually.
  fun_check(!src->d);
  dst->flags = src->flags;
  dst->invalid_chars = src->invalid_chars;
  dst->state_data[0] = src->state_data[0];
  dst->state_data[1] = src->state_data[1];
  dst->state_data[2] = src->state_data[2];
}

void TextStreamImpl::Params::Reset() {
  real_number_precision_ = 6;
  integer_base_ = 0;  // 0=auto
  field_width_ = 0;
  pad_char_ = ' ';
  field_alignment_ = TextStream::AlignRight;
  real_number_notation_ = TextStream::SmartNotation;
  number_flags_ = 0;
}

void TextStreamImpl::Reset() {
  params_.Reset();

  device_ = nullptr;
  delete_device_ = false;

  string_ = nullptr;
  string_offset_ = 0;
  string_open_mode_ = IoDevice::NotOpen;

  read_buffer_offset_ = 0;
  read_buffer_start_device_pos_ = 0;
  last_token_len_ = 0;

  encoding_ = TextEncoding::EncodingForLocale();
  ResetEncodingConverterStateHelper(&read_converter_state_);
  ResetEncodingConverterStateHelper(&write_converter_state_);
  delete read_converter_saved_state;
  read_converter_saved_state_ = nullptr;
  write_converter_state_.flags |= TextEncoding::IgnoreHeader;
  auto_detect_unicode_ = true;
}

bool TextStream::FillReadBuffer(int64 max_len) {
  // device가 source인 경우에만 호출되어야함.
  fun_check(string_ == nullptr);
  fun_check(device_ != nullptr);

  const bool text_mode_enabled = device_->IsTextModeEnabled();
  if (text_mode_enabled) {
    device_->SetTextModeEnabled(false);
  }

  char buf[FUN_TEXTSTREAM_BUFFERSIZE];
  int64 bytes_read = 0;

#if defined(FUN_PLATFORM_WINDOWS_FAMILY)
  // IFile
  if (device_->IsSequential()) {
    if (max_len != -1) {
      bytes_read =
          device_->ReadLine(buf, MathBase::Min<int64>(sizeof(buf), max_len));
    } else {
      bytes_read = device_->ReadLine(buf, sizeof(buf));
    }
  } else
#endif  // defined(FUN_PLATFORM_WINDOWS_FAMILY)
  {
    if (max_len != -1) {
      bytes_read =
          device_->Read(buf, MathBase::Min<int64>(sizeof(buf), max_len));
    } else {
      bytes_read = device_->Read(buf, sizeof(buf));
    }
  }

  if (text_mode_enabled) {
    device_->SetTextModeEnabled(true);
  }

  if (bytes_read <= 0) {
    return false;
  }

  if (!encoding_ || auto_detect_unicode_) {
    auto_detect_unicode_ = false;

    encoding_ = TextEncoding::EncodingForUtfText(
        ByteArray::FromRawData(buf, bytes_read), encoding_);
    if (!encoding_) {
      encoding_ = TextEncoding::EncodingForLocale();
      write_converter_state_.flags |= TextEncoding::IgnoreHeader;
    }
  }

  int32 old_read_buffer_size = read_buffer_.Len();
  // convert to unicode
  read_buffer_ +=
      encoding_ ? encoding_->ToUnicode(buf, bytes_read, &read_converter_state_)
                : UString::FromLatin1(buf, bytes_read);

  // remove all '\r\n' in the string.
  if (read_buffer_.Len() > old_read_buffer_size && text_mode_enabled) {
    UNICHAR cr = '\r';
    UNICHAR* write_ptr = read_buffer_.MutableData() + old_read_buffer_size;
    const UNICHAR* read_ptr = read_buffer_.ConstData() + old_read_buffer_size;
    const UNICHAR* end_ptr = read_buffer_.ConstData() + read_buffer_.Count();

    int32 n = old_read_buffer_size;
    if (read_ptr < end_ptr) {
      // Cut-off to avoid unnecessary self-copying.
      while (*read_ptr++ != cr) {
        ++n;
        if (++write_ptr == end_ptr) {
          break;
        }
      }

      while (read_ptr < end_ptr) {
        UNICHAR ch = *read_ptr++;
        if (ch != cr) {
          *write_ptr++ = ch;
        } else {
          if (n < read_buffer_offset_) {
            --read_buffer_offset_;
          }
          --bytes_read;
        }
        ++n;
      }

      read_buffer_.Resize(write_ptr - read_buffer_.ConstData());
    }
  }

  return true;
}

void TextStreamImpl::ResetReadBuffer() {
  read_buffer_.Clear();
  read_buffer_offset_ = 0;
  read_buffer_start_device_pos_ = (device_ ? device_->Tell() : 0);
}

void TextStreamImpl::FlushWriteBuffer() {
  // device source일 경우에만 호출되어야함.
  if (string_ || !device_) {
    return;
  }

  if (status_ != Status::Ok) {
    return;
  }

  if (write_buffer_.IsEmpty()) {
    return;
  }

#if defined(FUN_PLATFORM_WINDOWS_FAMILY)
  // '\r\n' handling
  bool text_mode_enabled = device_->IsTextModeEnabled();
  if (text_mode_enabled) {
    device_->SetTextModeEnabled(false);
    write_buffer_.Replace(AsciiString("\n"), AsciiString("\r\n"));
  }
#endif  // defined(FUN_PLATFORM_WINDOWS_FAMILY)

  if (!encoding_) {
    encoding_ = TextEncoding::EncodingForLocale();
  }

#if defined(FUN_TEXTSTREAM_DEBUG)
  qDebug(
      "TextStreamImpl::FlushWriteBuffer(), using %s encoding_ (%s generating "
      "BOM)",
      encoding_ ? encoding_->GetName().ConstData() : "no",
      !encoding_ || (write_converter_state_.flags & TextEncoding::IgnoreHeader)
          ? "not"
          : "");
#endif

  // convert from unicode to raw data
  // encoding_ might be null if we're already inside global destructors
  // (QTestCodec::EncodingForLocale returned null)
  ByteArray data = encoding_ ? encoding_->FromUnicode(write_buffer_.ConstData(),
                                                      write_buffer_.Len(),
                                                      &write_converter_state_)
                             : write_buffer_.ToLatin1();

  write_buffer_.Clear();

  // Write raw data to the device
  int64 bytes_written = device_->Write(data);

#if defined(FUN_TEXTSTREAM_DEBUG)
  qDebug("TextStreamImpl::FlushWriteBuffer(), device_->Write(\"%s\") == %d",
         qt_prettyDebug(data.ConstData(), MathBase::Min(data.Len(), 32),
                        data.Len())
             .ConstData(),
         int32(bytes_written));
#endif

#if defined(FUN_PLATFORM_WINDOWS_FAMILY)
  // reset the text flag
  if (text_mode_enabled) {
    device_->SetTextModeEnabled(true);
  }
#endif  // defined (FUN_PLATFORM_WINDOWS_FAMILY)

  if (bytes_written <= 0) {
    status = TextStream::WriteFailed;
    return;
  }

  // flush the file
  // TODO
#ifndef QT_NO_QOBJECT
  FileDevice* file = qobject_cast<FileDevice*>(device_);
  bool flushed = !file || file->Flush();
#else
  bool flushed = true;
#endif

#if defined(FUN_TEXTSTREAM_DEBUG)
  qDebug("TextStreamImpl::FlushWriteBuffer() wrote %d bytes",
         int32(bytes_written));
#endif

  if (!flushed || bytes_written != int64(data.Len())) {
    status = TextStream::WriteFailed;
  }
}

UString TextStreamImpl::Read(int32 max_len) {
  UString ret;

  if (string_) {
    last_token_len_ = MathBase::Min(max_len, string_->Len() - string_offset_);
    // TODO source string이 유지된다면, MidRef를 사용해도 될듯 싶은데...
    ret = string_->Mid(string_offset_, last_token_len_);
  } else {
    while (read_buffer_.Len() - read_buffer_offset_ < max_len &&
           FillReadBuffer())
      ;

    last_token_len_ =
        MathBase::Min(max_len, read_buffer_.Len() - read_buffer_offset_);
    ret = read_buffer_.Mid(read_buffer_offset_, last_token_len_);
  }

  ConsumeLastToken();
}

bool TextStreamImpl::Scan(const UNICHAR** ptr, int32* token_length,
                          int32 max_len, TokenDelimiter delimiter) {
  int32 total_size = 0;
  int32 delim_size = 0;
  bool consume_delimiter = false;
  bool found_token = false;
  int32 start_offset = device_ ? read_buffer_offset_ : string_offset_;
  UNICHAR last_char;

  bool can_still_read_from_device = true;
  do {
    int32 end_offset;
    const UNICHAR* ch_ptr;
    if (device_) {
      ch_ptr = read_buffer_.ConstData();
      end_offset = read_buffer_.Len();
    } else {
      ch_ptr = string_->ConstData();
      end_offset = string_->Len();
    }
    ch_ptr += start_offset;

    for (; !found_token && start_offset < end_offset &&
           (!max_len || total_size < max_len);
         ++start_offset) {
      const UNICHAR ch = *ch_ptr++;
      ++total_size;

      switch (delimiter) {
        case Space:
          if (ch.IsSpace()) {
            found_token = true;
            delim_size = 1;
          }
          break;
        case NotSpace:
          if (!ch.IsSpace()) {
            found_token = true;
            delim_size = 1;
          }
          break;
        case EndOfLine:
          if (ch == '\n') {
            found_token = true;
            delim_size = (last_char == '\r') ? 2 : 1;
            consume_delimiter = true;
          }
          last_char = ch;
          break;
      }
    }
  } while (!found_token && (!max_len || total_size < max_len) &&
           (device_ && (can_still_read_from_device = FillReadBuffer())));

  if (total_size == 0) {
#if defined(FUN_TEXTSTREAM_DEBUG)
    qDebug("TextStreamImpl::Scan() reached the end of input.");
#endif
    return false;
  }

  // if we find a '\r' at the end of the data when reading lines,
  // don't make it part of the line.
  if (delimiter == EndOfLine && total_size > 0 && !found_token) {
    if (((string_ && string_offset_ + total_size == string_->Len()) ||
         (device_ && device_->AtEnd())) &&
        last_char == '\r') {
      consume_delimiter = true;
      ++delim_size;
    }
  }

  // set the read offset and length of the token
  if (token_length) {
    *token_length = total_size - delim_size;
  }

  if (ptr) {
    *ptr = ReadPtr();
  }

  // update last token size. the callee will call ConsumeLastToken() when done.
  last_token_len_ = total_size;
  if (!consume_delimiter) {
    last_token_len_ -= delim_size;
  }

#if defined(FUN_TEXTSTREAM_DEBUG)
  qDebug(
      "TextStreamImpl::Scan(%p, %p, %d, %x) token length = %d, delimiter = %d",
      ptr, token_length, max_len, (int32)delimiter, total_size - delim_size,
      delim_size);
#endif

  return true;
}

const UNICHAR* TextStreamImpl::ReadPtr() const {
  fun_check(read_buffer_offset_ <= read_buffer_.Len());

  if (string_) {
    return string_->ConstData() + string_offset_;
  } else {
    return read_buffer_.ConstData() + read_buffer_offset_;
  }
}

void TextStreamImpl::ConsumeLastToken() {
  if (last_token_len_ > 0) {
    Consume(last_token_sie_);
  }

  last_token_len_ = 0;
}

void TextStreamImpl::Consume(int32 len) {
#if defined(FUN_TEXTSTREAM_DEBUG)
  qDebug("TextStreamImpl::Consume({})", len);
#endif

  if (string_) {
    string_offset_ += len;

    if (string_offset_ > string_->Len()) {
      string_offset_ = string_->Len();
    }
  } else {
    read_buffer_offset_ += len;

    if (read_buffer_offset_ >= read_buffer_.Len()) {
      read_buffer_offset_ = 0;
      read_buffer_.Clear();
      SaveConverterState(device_->Tell());
    } else if (read_buffer_offset_ > FUN_TEXTSTREAM_BUFFERSIZE) {
      read_buffer_ = read_buffer_.RemoveAt(0, read_buffer_offset_);
      read_converter_saved_state_offset_ += read_buffer_offset_;
      read_buffer_offset_ = 0;
    }
  }
}

void TextStreamImpl::SaveConverterState(int64 new_pos) {
  if (read_converter_state_.d) {
    // converter cannot be copied, so don't save anything
    // don't update read_buffer_start_device_pos_ either
    return;
  }

  if (!read_converter_saved_state_) {
    read_converter_saved_state_ = new TextEncoding::ConverterState;
  }
  CopyConverterStateHelper(read_converter_saved_state_, &read_converter_state_);

  read_buffer_start_device_pos_ = new_pos;
  read_converter_saved_state_offset_ = 0;
}

void TextStreamImpl::RestoreToSavedConverterState() {
  if (read_converter_saved_state_) {
    // we have a saved state
    // that means the converter can be copied
    CopyConverterStateHelper(&read_converter_state_,
                             read_converter_saved_state_);
  } else {
    // the only state we could save was the initial
    // so reset to that
    ResetCodecConverterStateHelper(&read_converter_state_);
  }
}

void TextStreamImpl::Write(UNICHAR ch) {
  if (string_) {
    // todo Seek를 했을 경우에 대한 처리가 필요해보임...
    string_->Append(ch);
  } else {
    writer_buffer_ += ch;

    if (write_buffer_.Len() > FUN_TEXTSTREAM_BUFFERSIZE) {
      FlushWriteBuffer();
    }
  }
}

void TextStreamImpl::Write(const UNICHAR* str, int32 len) {
  if (string_) {
    // todo Seek를 했을 경우에 대한 처리가 필요해보임...
    string_->Append(str, len);
  } else {
    writer_buffer_.Append(str, len);

    if (write_buffer_.Len() > FUN_TEXTSTREAM_BUFFERSIZE) {
      FlushWriteBuffer();
    }
  }
}

void TextStreamImpl::Write(Latin1String str) {
  if (string_) {
    // todo Seek를 했을 경우에 대한 처리가 필요해보임...
    string_->Append(str);
  } else {
    writer_buffer_ += str;

    if (write_buffer_.Len() > FUN_TEXTSTREAM_BUFFERSIZE) {
      FlushWriteBuffer();
    }
  }
}

void TextStreamImpl::WritePadding(int32 len) {
  if (string_) {
    // todo Seek를 했을 경우에 대한 처리가 필요해보임...
    string_->Resize(string_->Len() + len, params_.pad_char);
  } else {
    write_buffer_.Resize(string_->Len() + len, params_.pad_char);

    if (write_buffer_.Len() > FUN_TEXTSTREAM_BUFFERSIZE) {
      FlushWriteBuffer();
    }
  }
}

bool TextStreamImpl::GetChar(UNICHAR* ch) {
  if ((string_ && string_offset_ == string_->Len()) ||
      (device_ && read_buffer_.IsEmpty() && !FillReadBuffer())) {
    if (ch) *ch = 0;
    return false;
  }

  if (ch) {
    *ch = *ReadPtr();
  }

  Consume(1);

  return true;
}

void TextStreamImpl::UngetChar(UNICHAR ch) {
  if (string_) {
    if (string_offset_ == 0) {
      string_->Prepend(ch);
    } else {
      (*string_)[--string_offset_] = ch;
    }
  } else {
    if (read_buffer_offset_ == 0) {
      read_buffer_.Prepend(ch);
    } else {
      read_buffer_[--read_buffer_offset_] = ch;
    }
  }
}

void TextStreamImpl::PutString(const UNITHCAR* str, int32 len, bool is_number) {
  if (params_.field_width > len) {
    const PaddingResult pad = Padding(len);

    if (params_.field_alignment == TextStream::AlignAccountingStyle &&
        is_number) {
      const UNICHAR sign = len > 0 ? str[0] : UNICHAR(0);
      if (sign == locale_.GetNegativeSign() || locale_.GetPositiveSign()) {
        // write the sign before the padding, then skip it later
        Write(&sign, 1);
        ++str;
        --len;
      }
    }

    WritePadding(pad.left);
    Write(str, len);
    WritePadding(pad.right);
  } else {
    Write(str, len);
  }
}

void TextStreamImpl::PutString(Latin1String str, bool is_number) {
  if (params_.field_width > str.Len()) {
    const PaddingResult pad = Padding(str.Len());

    if (params_.field_alignment == TextStream::AlignAccountingStyle &&
        is_number) {
      const UNICHAR sign = str.Len() > 0 ? str[0] : UNICHAR(0);
      if (sign == locale_.GetNegativeSign() || locale_.GetPositiveSign()) {
        // write the sign before the padding, then skip it later
        Write(&sign, 1);
        str = Latin1String(str.ConstData() + 1, str.Len() - 1);
      }
    }

    WritePadding(pad.left);
    Write(str);
    WritePadding(pad.right);
  } else {
    Write(str);
  }
}

void TextStreamImpl::PutChar(UNICHAR ch) {
  if (params_.field_width > 0) {
    PutString(&ch, 1);
  } else {
    Write(ch);
  }
}

void TextStreamImpl::PutNumber(uint64 number, bool is_negative) {
  UString result;

  unsigned flags = 0;
  const TextStream::NumberFlags number_flags = params_.number_flags;
  if (number_flags & TextStream::ShowBase) {
    flags |= LocaleData::ShowBase;
  }
  if (number_flags & TextStream::ForceSign) {
    flags |= LocaleData::AlwaysShowSign;
  }
  if (number_flags & TextStream::UppercaseBase) {
    flags |= LocaleData::UppercaseBase;
  }
  if (number_flags & TextStream::UppercaseDigits) {
    flags |= LocaleData::CapitalEorX;
  }

  // add thousands group separators. For backward compatibility we
  // don't add a group separator for C locale.
  if (locale_ != Locale::C() &&
      !locale_.GetNumberOptions().TestFlag(Locale::OmitGroupSeparator)) {
    flags |= LocaleData::ThousandsGroup;
  }

  const LocaleData* dd = locale_.impl_->m_data;
  int32 base = params_.integer_base ? params_.integer_base : 10;
  if (is_negative && base == 10) {
    result =
        dd->Int64ToString(-static_cast<int64>(number), -1, base, -1, flags);
  } else if (is_negative) {
    // Workaround for backward compatibility for writing is_negative
    // numbers in octal and hex:
    // TextStream(result) << showbase << hex << -1 << oct << -1
    // should output: -0x1 -0b1
    result = dd->UInt64ToString(number, -1, base, -1, flags);
    result.Prepend(locale_.GetNegativeSign());
  } else {
    result = dd->UInt64ToString(number, -1, base, -1, flags);
    // workaround for backward compatibility - in octal form with
    // ShowBase flag set zero should be written as '00'
    if (number == 0 && base == 8 &&
        params_.number_flags & TextStream::ShowBase &&
        result == Latin1String("0")) {
      result.Prepend('0');
    }
  }
  PutString(result, true);
}

TextStreamImpl::PaddingResult TextStreamImpl::Padding(int32 len) const {
  fun_check(params_.field_width > len);

  int32 left = 0;
  int32 right = 0;

  const int32 pad_len = params_.field_width - len;

  switch (params_.field_alignment) {
    case TextStream::AlignLeft:
      right = pad_len;
      break;
    case TextStream::AlignRight:
    case TextStream::AlignAccountingStyle:
      left = pad_len;
      break;
    case TextStream::AlignCenter:
      left = pad_len / 2;
      right = pad_len - left;
      break;
    default:
      fun_unexpected();
  }
  return {left, right};
}

TextStreamImpl::NumberParsingStatus TextStreamImpl::GetNumber(uint64* ret) {
  Scan(nullptr, nullptr, 0, NotSpace);
  ConsumeLastToken();

  // detect int32 encoding
  int32 base = params_.integer_base;
  if (base == 0) {
    UNICHAR ch;
    if (!GetChar(&ch)) {
      return NumberParsingStatus::InvalidPrefix;
    }

    if (ch == '0') {
      UNICHAR ch2;
      if (!GetChar(&ch2)) {
        // Result is the number 0
        *ret = 0;
        return NumberParsingStatus::Ok;
      }
      ch2 = ch2.ToLower();

      if (ch2 == 'x') {
        base = 16;
      } else if (ch2 == 'b') {
        base = 2;
      } else if (ch2.IsDigit() && ch2.digitValue() >= 0 &&
                 ch2.digitValue() <= 7) {
        base = 8;
      } else {
        base = 10;
      }
      UngetChar(ch2);
    } else if (ch == locale_.GetNegativeSign() ||
               ch == locale_.GetPositiveSign() || ch.IsDigit()) {
      base = 10;
    } else {
      UngetChar(ch);
      return NumberParsingStatus::InvalidPrefix;
    }
    UngetChar(ch);
    // State of the stream is now the same as on entry
    // (cursor is at prefix),
    // and local variable 'base' has been set appropriately.
  }

  uint64 val = 0;
  switch (base) {
    case 2: {
      UNICHAR pf1, pf2, dig;
      // Parse prefix '0b'
      if (!GetChar(&pf1) || pf1 != '0') {
        return NumberParsingStatus::InvalidPrefix;
      }

      if (!GetChar(&pf2) || pf2.ToLower() != 'b') {
        return NumberParsingStatus::InvalidPrefix;
      }

      // Parse digits
      int32 ndigits = 0;
      while (GetChar(&dig)) {
        int32 n = dig.ToLower().unicode();
        if (n == '0' || n == '1') {
          val <<= 1;
          val += n - '0';
        } else {
          UngetChar(dig);
          break;
        }
        ndigits++;
      }
      if (ndigits == 0) {
        // Unwind the prefix and abort
        UngetChar(pf2);
        UngetChar(pf1);
        return NumberParsingStatus::MissingDigit;
      }
      break;
    }
    case 8: {
      UNICHAR pf, dig;
      // Parse prefix '0'
      if (!GetChar(&pf) || pf != '0') {
        return NumberParsingStatus::InvalidPrefix;
      }

      // Parse digits
      int32 ndigits = 0;
      while (GetChar(&dig)) {
        int32 n = dig.ToLower().unicode();
        if (n >= '0' && n <= '7') {
          val *= 8;
          val += n - '0';
        } else {
          UngetChar(dig);
          break;
        }
        ndigits++;
      }
      if (ndigits == 0) {
        // Unwind the prefix and abort
        UngetChar(pf);
        return NumberParsingStatus::MissingDigit;
      }
      break;
    }
    case 10: {
      // Parse sign (or first digit)
      UNICHAR sign;
      int32 ndigits = 0;
      if (!GetChar(&sign)) {
        return NumberParsingStatus::MissingDigit;
      }

      if (sign != locale_.GetNegativeSign() &&
          sign != locale_.GetPositiveSign()) {
        if (!sign.IsDigit()) {
          UngetChar(sign);
          return NumberParsingStatus::MissingDigit;
        }
        val += sign.digitValue();
        ndigits++;
      }

      // Parse digits
      UNICHAR ch;
      while (GetChar(&ch)) {
        if (ch.IsDigit()) {
          val *= 10;
          val += ch.digitValue();
        } else if (locale_ != Locale::C() &&
                   ch == locale_.GetGroupSeparator()) {
          continue;
        } else {
          UngetChar(ch);
          break;
        }
        ndigits++;
      }

      if (ndigits == 0) {
        return NumberParsingStatus::MissingDigit;
      }

      if (sign == locale_.GetNegativeSign()) {
        int64 ival = int64(val);
        if (ival > 0) {
          ival = -ival;
        }
        val = uint64(ival);
      }
      break;
    }
    case 16: {
      UNICHAR pf1, pf2, dig;
      // Parse prefix ' 0x'
      if (!GetChar(&pf1) || pf1 != '0') {
        return NumberParsingStatus::InvalidPrefix;
      }

      if (!GetChar(&pf2) || pf2.ToLower() != 'x') {
        return NumberParsingStatus::InvalidPrefix;
      }

      // Parse digits
      int32 ndigits = 0;
      while (GetChar(&dig)) {
        int32 n = dig.ToLower().unicode();
        if (n >= '0' && n <= '9') {
          val <<= 4;
          val += n - '0';
        } else if (n >= 'a' && n <= 'f') {
          val <<= 4;
          val += 10 + (n - 'a');
        } else {
          UngetChar(dig);
          break;
        }
        ndigits++;
      }

      if (ndigits == 0) {
        return NumberParsingStatus::MissingDigit;
      }
      break;
    }
    default:
      // Unsupported integer_base
      return NumberParsingStatus::InvalidPrefix;
  }

  if (ret) {
    *ret = val;
  }

  return NumberParsingStatus::Ok;
}

bool TextStreamImpl::GetReal(double* real);
{
  // We use a table-driven FSM to parse floating point numbers
  // strtod() cannot be used directly since we may be reading from a
  // QIODevice.
  enum ParserState {
    Init = 0,
    Sign = 1,
    Mantissa = 2,
    Dot = 3,
    Abscissa = 4,
    ExpMark = 5,
    ExpSign = 6,
    Exponent = 7,
    Nan1 = 8,
    Nan2 = 9,
    Inf1 = 10,
    Inf2 = 11,
    NanInf = 12,
    Done = 13
  };

  enum InputToken {
    None = 0,
    InputSign = 1,
    InputDigit = 2,
    InputDot = 3,
    InputExp = 4,
    InputI = 5,
    InputN = 6,
    InputF = 7,
    InputA = 8,
    InputT = 9
  };

  static const uint8 table[13][10] = {
      // None InputSign InputDigit InputDot InputExp  InputI  InputN  InputF
      // InputA  InputT
      {0, Sign, Mantissa, Dot, 0, Inf1, Nan1, 0, 0, 0},      // 0  Init
      {0, 0, Mantissa, Dot, 0, Inf1, Nan1, 0, 0, 0},         // 1  Sign
      {Done, Done, Mantissa, Dot, ExpMark, 0, 0, 0, 0, 0},   // 2  Mantissa
      {0, 0, Abscissa, 0, 0, 0, 0, 0, 0, 0},                 // 3  Dot
      {Done, Done, Abscissa, Done, ExpMark, 0, 0, 0, 0, 0},  // 4  Abscissa
      {0, ExpSign, Exponent, 0, 0, 0, 0, 0, 0, 0},           // 5  ExpMark
      {0, 0, Exponent, 0, 0, 0, 0, 0, 0, 0},                 // 6  ExpSign
      {Done, Done, Exponent, Done, Done, 0, 0, 0, 0, 0},     // 7  Exponent
      {0, 0, 0, 0, 0, 0, 0, 0, Nan2, 0},                     // 8  Nan1
      {0, 0, 0, 0, 0, 0, NanInf, 0, 0, 0},                   // 9  Nan2
      {0, 0, 0, 0, 0, 0, Inf2, 0, 0, 0},                     // 10 Inf1
      {0, 0, 0, 0, 0, 0, 0, NanInf, 0, 0},                   // 11 Inf2
      {Done, 0, 0, 0, 0, 0, 0, 0, 0, 0},                     // 11 NanInf
  };

  ParserState state = Init;
  InputToken input = None;

  Scan(nullptr, nullptr, 0, NotSpace);
  ConsumeLastToken();

  const int32 BufferSize = 128;
  char buf[BufferSize];
  int32 i = 0;

  UNICHAR c;
  while (GetChar(&c)) {
    switch (c.unicode()) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        input = InputDigit;
        break;
      case 'i':
      case 'I':
        input = InputI;
        break;
      case 'n':
      case 'N':
        input = InputN;
        break;
      case 'f':
      case 'F':
        input = InputF;
        break;
      case 'a':
      case 'A':
        input = InputA;
        break;
      case 't':
      case 'T':
        input = InputT;
        break;
      default: {
        UNICHAR lc = c.ToLower();
        if (lc == locale_.GetDecimalPoint().ToLower()) {
          input = InputDot;
        } else if (lc == locale_.GetExponential().ToLower()) {
          input = InputExp;
        } else if (lc == locale_.GetNegativeSign().ToLower() ||
                   lc == locale_.GetPositiveSign().ToLower()) {
          input = InputSign;
        } else if (locale_ != Locale::C()  // backward-compatibility
                   && lc == locale_.GetGroupSeparator().ToLower()) {
          input = InputDigit;  // well, it isn't a digit, but no one cares.
        } else {
          input = None;
        }
      } break;
    }

    state = ParserState(table[state][input]);

    if (state == Init || state == Done || i > (BufferSize - 5)) {
      UngetChar(c);
      if (i > (BufferSize - 5)) {  // ignore rest of digits
        while (GetChar(&c)) {
          if (!c.IsDigit()) {
            UngetChar(c);
            break;
          }
        }
      }
      break;
    }

    buf[i++] = c.ToLatin1();
  }

  if (i == 0) {
    return false;
  }

  if (!f) {
    return true;
  }

  buf[i] = '\0';

  // backward-compatibility. Old implementation supported +nan/-nan
  // for some reason. Locale only checks for lower-case
  // nan/+inf/-inf, so here we also check for uppercase and mixed
  // case versions.
  if (!qstricmp(buf, "nan") || !qstricmp(buf, "+nan") ||
      !qstricmp(buf, "-nan")) {
    *f = qSNaN();
    return true;
  } else if (!qstricmp(buf, "+inf") || !qstricmp(buf, "inf")) {
    *f = qInf();
    return true;
  } else if (!qstricmp(buf, "-inf")) {
    *f = -qInf();
    return true;
  }

  bool ok;
  *f = locale_.ToDouble(UString::FromLatin1(buf), &ok);
  return ok;
}

//
// TextStream
//

TextStream::TextStream() : impl_(new TextStreamImpl(this)) {
  impl_->status_ = Status::Ok;
}

TextStream::TextStream(IoDevice* device) : impl_(new TextStreamImpl(this)) {
  impl_->device_ = device;
  impl_->status_ = Status::Ok;
}

TextStream::TextStream(UString* string, IoDevice::OpenMode open_mode)
    : impl_(new TextStreamImpl(this)) {
  impl_->string_ = string;
  impl_->string_open_mode_ = open_mode;
  impl_->status_ = Status::Ok;
}

TextStream::TextStream(ByteArray* array, IoDevice::OpenMode open_mode)
    : impl_(new TextStreamImpl(this)) {
  impl_->device_ = new Buffer(array);
  impl_->device_->Open(open_mode);
  impl_->delete_device_ = true;
  impl_->status_ = Status::Ok;
}

TextStream::TextStream(const ByteArray& array, IoDevice::OpenMode open_mode)
    : impl_(new TextStreamImpl(this)) {
  Buffer* buffer = new Buffer();
  buffer->SetData(array);
  buffer->Open(open_mode);

  impl_->device_ = buffer;
  impl_->delete_device_ = true;
  impl_->status_ = Status::Ok;
}

TextStream::TextStream(FILE* fp, IoDevice::OpenMode open_mode)
    : impl_(new TextStreamImpl(this)) {
  File* file = new File;
  file->Open(fp, open_mode);

  impl_->device_ = buffer;
  impl_->delete_device_ = true;
  impl_->status_ = Status::Ok;
}

TextStream::~TextStream() {
  if (!impl_->write_buffer_.IsEmpty()) {
    impl_->FlushWriteBuffer();
  }
}

void TextStream::Reset() { impl_->params_.Reset(); }

void TextStream::Flush() { impl_->FlushWriteBuffer(); }

bool TextStream::Seek(int64 position) {
  impl_->last_token_len_ = 0;

  if (impl_->device_) {
    // Empty the write buffer
    impl_->FlushWriteBuffer();

    if (!impl_->device_->Seek(position)) {
      return false;
    }

    impl_->ResetReadBuffer();

#ifndef FUN_NO_TEXTCODEC
    // Reset the codec converter states.
    ResetEncodingConverterStateHelper(&impl_->read_converter_state_);
    ResetEncodingConverterStateHelper(&impl_->write_converter_state_);
    delete impl_->read_converted_saved_state_;
    impl_->read_converted_saved_state_ = nullptr;
    impl_->write_converter_state_.flags |= TextEncoding::IgnoreHeader;
#endif
    return true;
  }

  // string
  if (impl_->string_ && position <= impl_->string_->Size()) {
    impl_->string_offset_ = int32(position);
    return true;
  }

  return false;
}

int64 TextStream::Tell() const {
  if (impl_->device_) {
    // Cutoff
    if (impl_->read_buffer_.IsEmpty()) {
      return impl_->device_->Tell();
    }

    if (impl_->device_->IsSequential()) {
      return 0;
    }

    // Seek the device
    if (!impl_->device_->Seek(impl_->read_buffer_start_device_pos_)) {
      return int64(-1);
    }

    // Reset the read buffer
    TextStreamImpl* mutable_this = const_cast<TextStreamImpl*>(impl_);
    mutable_this->read_buffer_.Clear();

#ifndef FUN_NO_TEXTCODEC
    mutable_this->RestoreToSavedConverterState();
    if (impl_->read_buffer_start_device_pos_ == 0) {
      thatimpl_->auto_detect_unicode_ = true;
    }
#endif

    // Rewind the device to get to the current position Ensure that
    // read_buffer_offset_ is unaffected by FillReadBuffer()
    int old_read_buffer_offset =
        impl_->read_buffer_offset_ + impl_->read_converter_saved_state_offset_;
    while (impl_->read_buffer_.Size() < old_read_buffer_offset) {
      if (!mutable_this->FillReadBuffer(1)) {
        return int64(-1);
      }
    }
    mutable_this->read_buffer_offset_ = old_read_buffer_offset;
    mutable_this->read_converter_saved_state_offset_ = 0;

    // Return the device position.
    return impl_->device_->Tell();
  }

  if (impl_->string_) {
    return impl_->string_offset_;
  }

  fun_log(Warning, "QTextStream::Tell: no device");
  return int64(-1);
}

void TextStream::SkipWhitespaces() {
  CHECK_VALID_STREAM(FUN_VOID);
  impl_->Scan(nullptr, nullptr, 0, TextStreamImpl::NotSpace);
  impl_->ConsumeLastToken();
}

// TODO own_device 플래그를 넘겨줄 수 있도록 하는게??
void TextStream::SetDevice(IoDevice* device) {
  Flush();

  if (impl_->delete_device_) {
    delete impl_->device_;
    impl_->delete_device_ = false;
  }

  impl_->Reset();
  impl_->status_ = Status::Ok;
  impl_->device_ = device;
  impl_->ResetReadBuffer();
}

IoDevice* TextStream::GetDevice() const { return impl_->device_; }

void TextStream::SetString(UString* string, IoDevice::OpenMode open_mode) {
  Flush();

  if (impl_->delete_device_) {
    delete impl_->device_;
    impl_->delete_device_ = false;
  }

  impl_->Reset();
  fun_check(impl_->device_ == nullptr);

  impl_->status_ = Status::Ok;
  impl_->string_ = string;
  impl_->ResetReadBuffer();
}

UString* TextStream::GetString() const { return impl_->string_; }

TextStream::FieldAlignment TextStream::GetFieldAlignment() const {
  return impl_->params_.field_alignment;
}

void TextStream::SetFieldAlignment(FieldAlignment alignment) {
  impl_->params_.field_alignment = alignment;
}

UNICHAR TextStream::GetPadChar() const { return impl_->params_.pad_char; }

void TextStream::SetPadChar(UNICHAR ch) { return impl_->params_.pad_char = ch; }

int32 TextStream::GetFieldWidth() const {
  return return impl_->params_.field_width;
}

void TextStream::SetFieldWidth(int32 width) {
  return impl_->params_.field_width = width;
}

TextStream::NumberFlags TextStream::GetNumberFlags() {
  return impl_->params_.number_flags;
}

void TextStream::SetNumberFlags(NumberFlags flags) {
  impl_->params_.number_flags = flags;
}

int32 TextStream::GetIntegerBase() const { return impl_->params_.integer_base; }

void TextStream::SetIntegerBase(int32 base) {
  // fun_check(base == 2 || base == 8 || base == 10 || base == 16);
  impl_->params_.integer_base = base;
}

TextStream::ReadNumberNotation TextStream::GetRealNumberNotation() const {
  return impl_->params_.real_number_notation;
}

void TextStream::GetRealNumberNotation(ReadNumberNotation notation) {
  impl_->params_.real_number_notation = notation;
}

int32 TextStream::GetRealNumberPrecision() const {
  return impl_->params_.real_number_precision;
}

void TextStream::SetRealNumberPrecision(int32 precision) {
  impl_->params_.real_number_precision = precision;
}

Status TextStream::GetStatus() const { return impl_->status_; }

void TextStream::SetStatus(Status status) {
  // 이전에 오류가 발생한 상황에서는 설정하지 않음.
  if (impl_->status_ == Status::Ok) {
    impl_->status_ = status;
  }
}

void TextStream::ResetStatus() { impl_->status_ = Status::Ok; }

bool TextStream::AtEnd() const {
  if (impl_->string_) {
    return impl_->string_offset_ >= impl_->string_->Len();
  } else {
    return impl_->read_buffer_.IsEmpty() && impl_->device_->AtEnd();
  }
}

bool TextStream::ReadLineInto(String* buf, int64 max_len) {
  // keep in sync with CHECK_VALID_STREAM
  if (!impl_->string_ && !impl_->device_) {
    fun_log(Warning, "QTextStream: No device");
    if (buf && !buf->IsNull()) {
      buf->Resize(0);
    }

    return false;
  }

  const UNICHAR* read_ptr;
  int32 len;
  if (!impl_->Scan(&read_ptr, &len, int32(max_len),
                   TextStreamImpl::EndOfLine)) {
    if (buf && !buf->IsNull()) {
      buf->Resize(0);
    }

    return false;
  }

  if (FUN_LIKELY(buf)) {
    buf->SetUnicode(read_ptr, len);
  }

  impl_->ConsumeLastToken();

  return true;
}

String TextStream::ReadAll() { return Read(int32_MAX); }

String TextStream::ReadLine(int64 max_len) {
  UString line;
  ReadLineInto(line, max_len);
  return line;
}

String TextStream::Read(int64 max_len) {
  CHECK_VALID_STREAM(UString());

  if (max_len <= 0) {
    return UString();
  }

  return impl_->Read(int32(max_len));
}

void TextStream::SetLocale(const Locale& locale) { impl_->locale_ = locale; }

Locale TextStream::GetLocale() const { return impl_->locale_; }

void TextStream::SetEncoding(TextEncoding* encoding) {
  int64 seek_pos = -1;
  if (!impl_->read_buffer_.IsEmpty()) {
    if (!impl_->device_->IsSequential()) {
      seek_pos = Tell();
    }
  }

  impl_->encoding_ = encoding;

  if (seek_pos >= 0 && !impl_->read_buffer_.IsEmpty()) {
    Seek(seek_pos);
  }
}

void TextStream::SetEncoding(const char* encoding_name) {
  TextEncoding* encoding = TextEncoding::EncodingForName(encoding_name);
  if (encoding) {
    SetEncoding(encoding);
  }
}

TextEncoding* TextStream::GetEncoding() const { return impl_->encoding_; }

void TextStream::SetAutoDetectUnicode(bool enabled) {
  impl_->auto_detect_unicode_ = enabled;
}

bool TextStream::GetAutoDetectUnicode() const {
  return impl_->auto_detect_unicode_;
}

void TextStream::SetGenerateByteOrderMark(bool generate) {
  if (impl_->write_buffer_.IsEmpty()) {
    impl_->write_converter_state_.flags.SetFlags(TextEncoding::IgnoreHeader,
                                                 !generate);
  }
}

bool TextStream::GetGenerateByteOrderMark() const {
  return (impl_->write_converter_state_.flags & TextEncoding::IgnoreHeader) ==
         0;
}

//
// Serialization operators.
//

TextStream& TextStream::operator>>(UNICHAR& ch) {
  fun_check_valid_stream(*this);

  impl_->Scan(nullptr, nullptr, 0, TextStreamImpl::NoSpace);
  if (!impl_->GetChar(&ch)) {
    SetStatus(ReadPastEnd);
  }
  return *this;
}

TextStream& TextStream::operator>>(char& ch) {
  UNICHAR uch;
  *this >> uch;
  ch = (char)uch;
  return *this;
}

TextStream& TextStream::operator>>(int16& i) {
  IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(int16);
}

TextStream& TextStream::operator>>(uint16& i) {
  IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(uint16);
}

TextStream& TextStream::operator>>(int32& i) {
  IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(int32);
}

TextStream& TextStream::operator>>(uint32& i) {
  IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(uint32);
}

TextStream& TextStream::operator>>(int64& i) {
  IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(int64);
}

TextStream& TextStream::operator>>(uint64& i) {
  IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(uint64);
}

TextStream& TextStream::operator>>(float& f) {
  IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(float);
}

TextStream& TextStream::operator>>(double& f) {
  IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(double);
}

TextStream& TextStream::operator>>(UString& str) {
  CHECK_VALID_STREAM(*this);

  str.Clear();
  impl_->Scan(nullptr, nullptr, 0, TextStreamImpl::NotSpace);
  impl_->ConsumeLastToken();

  const UNICHAR* ptr;
  int32 len;
  if (!impl_->Scan(&ptr, &len, 0, TextStreamImpl::Space)) {
    SetStatus(ReadPastEnd);
    return *this;
  }

  str = UString(ptr, len);
  impl_->ConsumeLastToken();
  return *this;
}

TextStream& TextStream::operator>>(ByteArray& array) {
  CHECK_VALID_STREAM(*this);

  array.Clear();
  impl_->Scan(nullptr, nullptr, 0, TextStreamImpl::NotSpace);
  impl_->ConsumeLastToken();

  const UNICHAR* ptr;
  int32 len;
  if (!impl_->Scan(&ptr, &len, 0, TextStreamImpl::Space)) {
    SetStatus(ReadPastEnd);
    return *this;
  }

  for (int32 i = 0; i < len; ++i) {
    array += ptr[i].ToLatin1();
  }

  impl_->ConsumeLastToken();
  return *this;
}

// TODO 이건 좀 위험해 보이는데...??
TextStream& TextStream::operator>>(char* buf) {
  CHECK_VALID_STREAM(*this);

  *buf = 0;

  impl_->Scan(nullptr, nullptr, 0, TextStreamImpl::NotSpace);
  impl_->ConsumeLastToken();

  const UNICHAR* ptr;
  int32 len;
  if (!impl_->Scan(&ptr, &len, 0, TextStreamImpl::Space)) {
    SetStatus(ReadPastEnd);
    return *this;
  }

  for (int32 i = 0; i < len; ++i) {
    *buf++ = ptr[i].ToLatin1();
  }
  *buf = '\0';

  impl_->ConsumeLastToken();
  return *this;
}

TextStream& TextStream::operator<<(UNICHAR c) {
  fun_check_valid_stream(*this);

  impl_->PutChar(c);
  return *this;
}

TextStream& TextStream::operator<<(char c) {
  fun_check_valid_stream(*this);

  impl_->PutChar(UNICHAR(c));
  return *this;
}

TextStream& TextStream::operator<<(int16 i) {
  fun_check_valid_stream(*this);

  impl_->PutNumber((uint64)MathBase::Abs((uint64)i), i < 0);
  return *this;
}

TextStream& TextStream::operator<<(uint16 i) {
  fun_check_valid_stream(*this);

  impl_->PutNumber((uint64)i, false);
  return *this;
}

TextStream& TextStream::operator<<(int32 i) {
  fun_check_valid_stream(*this);

  impl_->PutNumber((uint64)MathBase::Abs((uint64)i), i < 0);
  return *this;
}

TextStream& TextStream::operator<<(uint32 i) {
  fun_check_valid_stream(*this);

  impl_->PutNumber((uint64)i, false);
  return *this;
}

TextStream& TextStream::operator<<(int64 i) {
  fun_check_valid_stream(*this);

  impl_->PutNumber((uint64)MathBase::Abs((uint64)i), i < 0);
  return *this;
}

TextStream& TextStream::operator<<(uint64 i) {
  fun_check_valid_stream(*this);

  impl_->PutNumber(i, false);
  return *this;
}

TextStream& TextStream::operator<<(float f) { return *this << double(f); }

TextStream& TextStream::operator<<(double d) {
  fun_check_valid_stream(*this);

  LocaleData::DoubleForm form = LocaleData::DFDecimal;
  switch (GetRealNumberNotation()) {
    case FixedNotation:
      form = LocaleData::DFDecimal;
      break;
    case ScientificNotation:
      form = LocaleData::DFExponent;
      break;
    case SmartNotation:
      form = LocaleData::DFSignificantDigits;
      break;
  }

  uint32 flags = 0;
  const Locale::NumberOptions number_options = GetLocale().GetNumberOptions();
  if (GetNumberFlags() & ShowBase) {
    flags |= LocaleData::ShowBase;
  }
  if (GetNumberFlags() & ForceSign) {
    flags |= LocaleData::AlwaysShowSign;
  }
  if (GetNumberFlags() & UppercaseBase) {
    flags |= LocaleData::UppercaseBase;
  }
  if (GetNumberFlags() & UppercaseDigits) {
    flags |= LocaleData::CapitalEorX;
  }
  if (GetNumberFlags() & ForcePoint) {
    flags |= LocaleData::ForcePoint;

    // Only for backwards compatibility
    flags |= LocaleData::AddTrailingZeroes | LocaleData::ShowBase;
  }
  if (GetLocale() != Locale::C() &&
      !(number_options & Locale::OmitGroupSeparator)) {
    flags |= LocaleData::ThousandsGroup;
  }
  if (!(number_options & Locale::OmitLeadingZeroInExponent)) {
    flags |= LocaleData::ZeroPadExponent;
  }
  if (number_options & Locale::IncludeTrailingZeroesAfterDot) {
    flags |= LocaleData::AddTrailingZeroes;
  }

  const LocaleData* dd = impl_->locale_.impl_->m_data;
  UString num = dd->DoubleToString(d, impl_->params_.real_number_precision,
                                   form, -1, flags);
  impl_->PutString(num, true);
  return *this;
}

TextStream& TextStream::operator<<(const UString& ustr) {
  fun_check_valid(*this);

  impl_->PutString(ustr);
  return *this;
}

TextStream& TextStream::operator<<(const UStringRef& ustr) {
  fun_check_valid(*this);

  impl_->PutString(ustr.ConstData(), ustr.Len());
  return *this;
}

TextStream& TextStream::operator<<(const String& str) {
  fun_check_valid(*this);

  impl_->PutString(UString::FromUtf8(str.ConstData(), str.Len());
  return *this;
}

TextStream& TextStream::operator<<(Latin1String str) {
  fun_check_valid(*this);

  impl_->PutString(str);
  return *this;
}

TextStream& TextStream::operator<<(const char* str) {
  fun_check_valid(*this);

  impl_->PutString(Latin1String(str));
  return *this;
}

TextStream& TextStream::operator<<(const void* ptr) {
  fun_check_valid(*this);

  const int32 old_base = impl_->params_.integer_base;
  const NumberFlags old_flags = impl_->params_.number_flags;
  impl_->params_.integer_base = 16;
  impl_->params_.number_flags |= ShowBase;
  impl_->PutNumber(reinterpret_cast<uintptr_t>(ptr), false);
  impl_->params_.integer_base = old_base;
  impl_->params_.number_flags = old_flags;
  return *this;
}

//
// manipulations
//

TextStream& bin(TextStream& stream) {
  stream.SetIntegerBase(2);
  return stream;
}

TextStream& oct(TextStream& stream) {
  stream.SetIntegerBase(8);
  return stream;
}

TextStream& dec(TextStream& stream) {
  stream.SetIntegerBase(10);
  return stream;
}

TextStream& hex(TextStream& stream) {
  stream.SetIntegerBase(16);
  return stream;
}

TextStream& showbase(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() | TextStream::ForceSign);
  return stream;
}

TextStream& forcepoint(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() | TextStream::ForcePoint);
  return stream;
}

TextStream& noshowbase(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() &= ~TextStream::ShowBase);
  return stream;
}

TextStream& noforcesign(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() &= ~TextStream::ForceSign);
  return stream;
}

TextStream& noforcepoint(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() &= ~TextStream::ForcePoint);
  return stream;
}

TextStream& uppercasebase(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() | TextStream::UppercaseBase);
  return stream;
}

TextStream& uppercasedigits(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() | TextStream::UppercaseDigits);
  return stream;
}

TextStream& lowercasebase(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() & ~TextStream::UppercaseBase);
  return stream;
}

TextStream& lowercasedigits(TextStream& stream) {
  stream.SetNumberFlags(stream.GetNumberFlags() & ~TextStream::UppercaseDigits);
  return stream;
}

TextStream& fixed(TextStream& stream) {
  stream.SetRealNumberNotation(TextStream::FixedNotation);
  return stream;
}

TextStream& scientific(TextStream& stream) {
  stream.SetRealNumberNotation(TextStream::ScientificNotation);
  return stream;
}

TextStream& left(TextStream& stream) {
  stream.SetFieldAlignment(TextStream::AlignLeft);
  return stream;
}

TextStream& right(TextStream& stream) {
  stream.SetFieldAlignment(TextStream::AlignRight);
  return stream;
}

TextStream& center(TextStream& stream) {
  stream.SetFieldAlignment(TextStream::AlignCenter);
  return stream;
}

TextStream& endl(TextStream& stream) {
  return stream << Latin1Char('\n') << flush;
}

TextStream& flush(TextStream& stream) {
  stream.Flush();
  return stream;
}

TextStream& reset(TextStream& stream) {
  stream.Reset();
  return stream;
}

TextStream& ws(TextStream& stream) {
  stream.SkipWhiteSpace();
  return stream;
}

TextStream& bom(TextStream& stream) {
  stream.SetGenerateByteOrderMark(true);
  return stream;
}

void TextStream::SetEncoding(TextEncoding* encoding) {
  int64 seek_pos = -1;
  if (!impl_->read_buffer_.IsEmpty()) {
    if (!impl_->device_->IsSequential()) {
      seek_pos = Tell();
    }
  }

  impl_->encoding = encoding;

  if (seek_pos >= 0 && !impl_->read_buffer_.IsEmpty()) {
    Seek(seek_pos);
  }
}

void TextStream::SetEncoding(const char* encoding_name) {
  if (TextEncoding* encoding = TextEncoding::EncodingForName(encoding_name)) {
    SetEncoding(encoding);
  }
}

TextEncoding* TextStream::GetEncoding() const { return impl_->encoding_; }

void TextStream::SetAutoDetectUnicode(bool enabled) {
  impl_->auto_detect_unicode_ = enabled;
}

bool TextStream::GetAutoDetectUnicode() const {
  return impl_->auto_detect_unicode_;
}

void TextStream::SetGenerateByteOrderMark(bool generate) {
  if (impl_->write_buffer_.IsEmpty()) {
    impl_->write_converter_state_.flags.SetFlags(TextEncoding::IgnoreHeader,
                                                 !generate);
  }
}

bool TextStream::GetGenerateByteOrderMark() const {
  return (impl_->write_converter_state_.flags & TextEncoding::IgnoreHeader) ==
         0;
}

void TextStream::SetLocale(const Locale& locale) { impl_->locale_ = locale; }

Locale TextStream::GetLocale() const { return impl_->locale_; }

}  // namespace fun
