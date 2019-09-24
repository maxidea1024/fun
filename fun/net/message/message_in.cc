#include "fun/net/message/message_in.h"
#include "fun/net/message/message_format_exception.h"

namespace fun {
namespace net {

MessageIn::MessageIn()
  : recursion_limit_(MessageFormatConfig::DefaultRecursionLimit),
  , recursion_depth_(0),
    message_max_length_(MessageFormatConfig::message_max_length),
    original_buffer_(nullptr),
    original_buffer_end_(nullptr),
    buffer_start_(nullptr),
    buffer_end_(nullptr),
    buffer_(nullptr),
    exceptions_enabled_(false) {
}

MessageIn::MessageIn(const MessageOut& source)
  : recursion_limit_(MessageFormatConfig::DefaultRecursionLimit),
    recursion_depth_(0),
    message_max_length_(MessageFormatConfig::message_max_length),
    sharable_buffer_(source.sharable_buffer_), // share
    exceptions_enabled_(false) {
  //내부에서 버퍼의 길이로 범위를 체크하는데, 이게 문제가 될 수 있음.
  //왜냐면 MessageOut의 SharableBuffer의 길이는 실제
  //데이터 길이가 아니라 버퍼의 길이이므로, 별도로 처리해야함.
  SetupMasterView(0, source.Length(), source.Length());
}

MessageIn::MessageIn(const MessageOut& source, int32 offset, int32 length)
  : recursion_limit_(MessageFormatConfig::DefaultRecursionLimit),
    recursion_depth_(0),
    message_max_length_(MessageFormatConfig::message_max_length),
    sharable_buffer_(source.sharable_buffer_), // share
    exceptions_enabled_(false) {
  //내부에서 버퍼의 길이로 범위를 체크하는데, 이게 문제가 될 수 있음.
  //왜냐면 MessageOut의 SharableBuffer의 길이는 실제
  //데이터 길이가 아니라 버퍼의 길이이므로, 별도로 처리해야함.
  SetupMasterView(offset, length, source.Length());
}

MessageIn::MessageIn(const MessageIn& source, bool with_source_position)
  : recursion_limit_(source.recursion_limit_),
   , recursion_depth_(source.recursion_depth_),
    message_max_length_(source.message_max_length_),
    sharable_buffer_(source.sharable_buffer_), // share
    exceptions_enabled_(false) {
  const int32 source_offset = source.GetMasterViewOffset();
  const int32 source_length = source.GetMasterViewLength();
  SetupMasterView(source_offset, source_length);

  if (with_source_position) {
    Seek(source.Tell());
  } else {
    fun_check(Tell() == 0);
  }
}

MessageIn::MessageIn(const MessageIn& source, int32 offset, int32 length)
  : recursion_limit_(source.recursion_limit_),
    recursion_depth_(source.recursion_depth_),
    message_max_length_(source.message_max_length_),
    sharable_buffer_(source.sharable_buffer_), // share
    exceptions_enabled_(false) {
  const int32 absolute_offset = source.GetMasterViewOffset() + offset;
  SetupMasterView(absolute_offset, length);
}

MessageIn::MessageIn(const ByteArray& source)
  : recursion_limit_(MessageFormatConfig::DefaultRecursionLimit),
    recursion_depth_(0),
    message_max_length_(MessageFormatConfig::message_max_length_),
    sharable_buffer_(source), // share
    exceptions_enabled_(false) {
  SetupMasterView();
}

MessageIn::MessageIn(const ByteArray& source, int32 offset, int32 length)
  : recursion_limit_(MessageFormatConfig::DefaultRecursionLimit),
    recursion_depth_(0),
    message_max_length_(MessageFormatConfig::MessageMaxLength),
    sharable_buffer_(source), // share
    exceptions_enabled_(false) {
  SetupMasterView(offset, length);
}

MessageIn& MessageIn::operator = (const MessageIn& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    recursion_limit_ = rhs.recursion_limit_;
    recursion_depth_ = rhs.recursion_depth_;
    message_max_length_ = rhs.message_max_length_;
    sharable_buffer_ = rhs.sharable_buffer_;
    exceptions_enabled_ = rhs.exceptions_enabled_;

    const int32 master_view_offset = rhs.GetMasterViewOffset();
    const int32 master_view_length = rhs.GetMasterViewLength();
    SetupMasterView(master_view_offset, master_view_length);

    Seek(rhs.Tell());
  }

  return *this;
}

void MessageIn::ResetBuffer() {
  sharable_buffer_ = ByteArray();
  SetupMasterView();
}

void MessageIn::SetupMasterView() {
  if (sharable_buffer_.IsEmpty()) {
    original_buffer_ = nullptr;
    original_buffer_end_ = nullptr;
  } else {
    original_buffer_ = (const uint8*)sharable_buffer_.ConstData();
    original_buffer_end_ = original_buffer_ + sharable_buffer_.Len();

    fun_check((intptr_t)original_buffer_ >= (intptr_t)sharable_buffer_.cbegin());
    fun_check((intptr_t)original_buffer_ <= (intptr_t)sharable_buffer_.cend());
    fun_check((intptr_t)original_buffer_end_ >= (intptr_t)sharable_buffer_.cbegin());
    fun_check((intptr_t)original_buffer_end_ <= (intptr_t)sharable_buffer_.cend());
  }

  buffer_start_ = original_buffer_;
  buffer_end_ = original_buffer_end_;
  buffer_ = buffer_start_;
  recursion_depth_ = 0;
}

void MessageIn::SetupMasterView(int32 master_view_offset,
                                int32 master_view_length,
                                int32 source_length) {
  if (master_view_offset < 0) {
    throw InvalidArgumentException();
  }

  if (master_view_length < 0) {
    throw InvalidArgumentException();
  }

  if (source_length < 0) {
    source_length = sharable_buffer_.Len();
  }

  fun_check((master_view_offset + master_view_length) <= source_length);
  if ((master_view_offset + master_view_length) > source_length) {
    throw InvalidArgumentException();
  }

  if (sharable_buffer_.IsEmpty()) {
    original_buffer_ = nullptr;
    original_buffer_end_ = nullptr;
  } else {
    original_buffer_ = (const uint8*)sharable_buffer_.ConstData() + master_view_offset;
    original_buffer_end_ = original_buffer_ + master_view_length;

    fun_check((intptr_t)original_buffer_ >= (intptr_t)sharable_buffer_.cbegin());
    fun_check((intptr_t)original_buffer_ <= (intptr_t)sharable_buffer_.cend());
    fun_check((intptr_t)original_buffer_end_ >= (intptr_t)sharable_buffer_.cbegin());
    fun_check((intptr_t)original_buffer_end_ <= (intptr_t)sharable_buffer_.cend());
  }

  buffer_start_ = original_buffer_;
  buffer_end_ = original_buffer_end_;
  buffer_ = buffer_start_;
  recursion_depth_ = 0;
}

int32 MessageIn::GetMasterViewOffset() const {
  if (sharable_buffer_.IsEmpty()) {
    return 0;
  }

  return int32((const char*)original_buffer_ - sharable_buffer_.ConstData());
}

int32 MessageIn::GetMasterViewLength() const {
  return int32(original_buffer_end_ - original_buffer_);
}

void MessageIn::Seek(int32 new_position) {
  const uint8* new_ptr = original_buffer_ + new_position;

  if (new_ptr < buffer_start_ || new_ptr > buffer_end_) {
    if (exceptions_enabled_) {
      throw IndexOutOfBoundsException(StringLiteral("position must be located between view.offset and view.end"));
    } else {
      return;
    }
  }

  buffer_ = new_ptr;
}

bool MessageIn::ReadAsShared(MessageIn& to, int32 length_to_read) {
  if (ReadableLength() < length_to_read) {
    if (exceptions_enabled_) {
      throw MessageFormatException::TruncatedMessage();
    } else {
      return false;
    }
  }

  // share
  to = MessageIn::From(*this, Tell(), length_to_read);

  Advance(length_to_read);
  return true;
}

bool MessageIn::ReadAsCopy(ByteArray& to, int32 length_to_read) {
  if (ReadableLength() < length_to_read) {
    if (exceptions_enabled_) {
      throw MessageFormatException::TruncatedMessage();
    } else {
      return false;
    }
  }

  // copy
  to.ResizeUninitialized(length_to_read);
  UnsafeMemory::Memcpy(to.MutableData(), ReadablePtr(), length_to_read);

  Advance(length_to_read);
  return true;
}

bool MessageIn::ReadAsRaw(ByteArray& to, int32 length_to_read) {
  if (ReadableLength() < length_to_read) {
    if (exceptions_enabled_) {
      throw MessageFormatException::TruncatedMessage();
    } else {
      return false;
    }
  }

  // raw
  to = ByteArray::FromRawData((const char*)ReadablePtr(), length_to_read);

  Advance(length_to_read);
  return true;
}

bool MessageIn::Advance(int32 amount) {
  if (amount < 0) {
    if (exceptions_enabled_) {
      throw MessageFormatException(StringLiteral("amount must be >= 0"));
    } else {
      return false;
    }
  }

  if ((buffer_ + amount) > buffer_end_) {
    if (exceptions_enabled_) {
      throw MessageFormatException(StringLiteral("amount couldn't end over."));
    } else {
      return false;
    }
  }

  buffer_ += amount;
  return true;
}

bool MessageIn::SkipRead(int32 amount) {
  return Advance(amount);
}

int32 MessageIn::TryReadRawBytes(void* dst, int32 length) {
  if (length < 0) {
    if (exceptions_enabled_) {
      throw MessageFormatException(StringLiteral("length must be >= 0"));
    } else {
      return 0;
    }
  }

  const int32 readable_length = ReadableLength();
  if (length > readable_length) {
    length = readable_length;
  }

  if (length > 0) {
    UnsafeMemory::Memcpy(dst, buffer_, length);
    Advance(length);
  }
  return length;
}


//
// Fixed
//

bool MessageIn::ReadFixed8(uint8& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= sizeof(out_value)) {
    out_value = *buffer_;
    Advance(sizeof(out_value));
    return true;
  } else {
    if (exceptions_enabled_) {
      throw MessageFormatException::TruncatedMessage();
    } else {
      return false;
    }
  }
}

bool MessageIn::ReadFixed16(uint16& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= sizeof(out_value)) {
#if FUN_ARCH_BIG_ENDIAN
    out_value = (static_cast<uint32>(buffer_[0])) |
                (static_cast<uint32>(buffer_[1]) << 8);
#else
  #if FUN_REQUIRES_ALIGNED_ACCESS
    UnsafeMemory::Memcpy(&out_value, buffer_, sizeof(out_value));
  #else
    out_value = *((uint16*)buffer_);
  #endif
#endif
    Advance(sizeof(out_value));
    return true;
  } else {
    if (exceptions_enabled_) {
      throw MessageFormatException::TruncatedMessage();
    } else {
      return false;
    }
  }
}

bool MessageIn::ReadFixed32(uint32& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= sizeof(out_value)) {
#if FUN_ARCH_BIG_ENDIAN
    out_value = (static_cast<uint32>(buffer_[0])) |
                (static_cast<uint32>(buffer_[1]) << 8) |
                (static_cast<uint32>(buffer_[2]) << 16) |
                (static_cast<uint32>(buffer_[3]) << 24);
#else
  #if FUN_REQUIRES_ALIGNED_ACCESS
    UnsafeMemory::Memcpy(&out_value, buffer_, sizeof(out_value));
  #else
    out_value = *((const uint32*)buffer_);
  #endif
#endif
    Advance(sizeof(out_value));
    return true;
  } else {
    if (exceptions_enabled_) {
      throw MessageFormatException::TruncatedMessage();
    } else {
      return false;
    }
  }
}

bool MessageIn::ReadFixed64(uint64& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= sizeof(out_value)) {
#if FUN_ARCH_BIG_ENDIAN
    const uint32 part0 =  (static_cast<uint32>(buffer_[0])) |
                          (static_cast<uint32>(buffer_[1]) << 8) |
                          (static_cast<uint32>(buffer_[2]) << 16) |
                          (static_cast<uint32>(buffer_[3]) << 24);

    const uint32 part1 =  (static_cast<uint32>(buffer_[4])) |
                          (static_cast<uint32>(buffer_[5]) << 8) |
                          (static_cast<uint32>(buffer_[6]) << 16) |
                          (static_cast<uint32>(buffer_[7]) << 24);

    out_value = static_cast<uint64>(part0) | (static_cast<uint64>(part1) << 32);
#else
  #if FUN_REQUIRES_ALIGNED_ACCESS
    UnsafeMemory::Memcpy(&out_value, buffer_, sizeof(out_value));
  #else
    out_value = *((uint64*)buffer_);
  #endif
#endif
    Advance(sizeof(out_value));
    return true;
  } else {
    if (exceptions_enabled_) {
      throw MessageFormatException::TruncatedMessage();
    } else {
      return false;
    }
  }
}


//
// Varint
//

bool MessageIn::ReadVarint8(uint8& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= 1 && buffer_[0] < (1 << 7)) {
    // fastest path
    out_value = buffer_[0];
    Advance(1);
    return true;
  } else {
    return ReadVarint8Fallback(out_value);
  }
}

bool MessageIn::ReadVarint16(uint16& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= 1 && buffer_[0] < (1 << 7)) {
    // fastest path
    out_value = static_cast<uint16>(buffer_[0]);
    Advance(1);
    return true;
  } else {
    return ReadVarint16Fallback(out_value);
  }
}

bool MessageIn::ReadVarint32(uint32& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= 1 && buffer_[0] < (1 << 7)) {
    // fastest path
    out_value = static_cast<uint32>(buffer_[0]);
    Advance(1);
    return true;
  } else {
    return ReadVarint32Fallback(out_value);
  }
}

bool MessageIn::ReadVarint64(uint64& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= 1 && buffer_[0] < (1 << 7)) {
    // fastest path
    out_value = static_cast<uint64>(buffer_[0]);
    Advance(1);
    return true;
  } else {
    return ReadVarint64Fallback(out_value);
  }
}


//
// Varint fallback
//

bool MessageIn::ReadVarint8Fallback(uint8& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= MessageFormat::MaxVarint8Length ||
      // optimization: we're also safe if the buffer is non-empty and it ends
      // with a byte that would terminate a varint.
      (readable_length >= 1 && buffer_end_[-1] < 0x80)) {
    if (const uint8* end = ReadVarint8FromBuffer(buffer_, out_value)) {
      Advance((int32)(end - buffer_));
      return true;
    } else {
      if (exceptions_enabled_) {
        throw MessageFormatException::MalformedVarint();
      } else {
        return false;
      }
    }
  } else {
    return ReadVarint8Slow(out_value);
  }
}

bool MessageIn::ReadVarint16Fallback(uint16& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= MessageFormat::MaxVarint16Length ||
      // optimization: we're also safe if the buffer is non-empty and it ends
      // with a byte that would terminate a varint.
      (readable_length >= 1 && buffer_end_[-1] < 0x80)) {
    if (const uint8* end = ReadVarint16FromBuffer(buffer_, out_value)) {
      Advance((int32)(end - buffer_));
      return true;
    } else {
      if (exceptions_enabled_) {
        throw MessageFormatException::MalformedVarint();
      } else {
        return false;
      }
    }
  } else {
    return ReadVarint16Slow(out_value);
  }
}

bool MessageIn::ReadVarint32Fallback(uint32& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= MessageFormat::MaxVarint32Length ||
      // optimization: we're also safe if the buffer is non-empty and it ends
      // with a byte that would terminate a varint.
      (readable_length >= 1 && buffer_end_[-1] < 0x80)) {
    if (const uint8* end = ReadVarint32FromBuffer(buffer_, out_value)) {
      Advance((int32)(end - buffer_));
      return true;
    } else {
      if (exceptions_enabled_) {
        throw MessageFormatException::MalformedVarint();
      } else {
        return false;
      }
    }
  } else {
    return ReadVarint32Slow(out_value);
  }
}

bool MessageIn::ReadVarint64Fallback(uint64& out_value) {
  const int32 readable_length = ReadableLength();

  if (readable_length >= MessageFormat::MaxVarint64Length ||
      // optimization: we're also safe if the buffer is non-empty and it ends
      // with a byte that would terminate a varint.
      (readable_length >= 1 && buffer_end_[-1] < 0x80)) {
    if (const uint8* end = ReadVarint64FromBuffer(buffer_, out_value)) {
      Advance((int32)(end - buffer_));
      return true;
    } else {
      if (exceptions_enabled_) {
        throw MessageFormatException::MalformedVarint();
      } else {
        return false;
      }
    }
  } else {
    return ReadVarint64Slow(out_value);
  }
}

const uint8* MessageIn::ReadVarint8FromBuffer(const uint8* buffer_, uint8& out_value) {
  // fast path:  we have enough bytes left in the buffer to guarantee that
  // this read won't cross the end, so we can skip the checks.
  const uint8* ptr = buffer_;
  uint8 byte;
  uint8 result;

  byte = *ptr++; result = byte; if (!(byte & 0x80)) goto Done;
  result -= 0x80;

  byte = *ptr++; result += byte << 7; if (!(byte & 0x80)) goto Done;
  //"result -= 0x80 << 7;" is irrevelant.

  // if the input is larger than 8 bits, we still need to read it all
  // and discard the high-order bits.
  const int32 lookahead_length = MessageFormat::MaxVarint64Length - MessageFormat::MaxVarint8Length;
  for (int32 i = 0; i < lookahead_length; ++i) {
    byte = *ptr++; if (!(byte & 0x80)) goto Done;
  }

  // we have overrun the maximum size of a varint (10 bytes).  assume the data is corrupt.
  return nullptr;

Done:
  out_value = result;
  return ptr;
}

const uint8* MessageIn::ReadVarint16FromBuffer(const uint8* buffer, uint16& out_value) {
  // fast path:  we have enough bytes left in the buffer to guarantee that
  // this read won't cross the end, so we can skip the checks.
  const uint8* ptr = buffer;

  uint16 byte;
  uint16 result;

  byte = *ptr++; result = byte; if (!(byte & 0x80)) goto Done;
  result -= 0x80;

  byte = *ptr++; result += byte <<  7; if (!(byte & 0x80)) goto Done;
  result -= 0x80 << 7;

  byte = *ptr++; result += byte << 14; if (!(byte & 0x80)) goto Done;
  //"result -= 0x80 << 14;" is irrevelant.

  // if the input is larger than 16 bits, we still need to read it all
  // and discard the high-order bits.
  const int32 lookahead_length = MessageFormat::MaxVarint64Length - MessageFormat::MaxVarint16Length;
  for (int32 i = 0; i < lookahead_length; ++i) {
    byte = *ptr++;
    if (!(byte & 0x80)) {
      goto Done;
    }
  }

  // we have overrun the maximum size of a varint (10 bytes).  assume the data is corrupt.
  return nullptr;

Done:
  out_value = result;
  return ptr;
}

const uint8* MessageIn::ReadVarint32FromBuffer(const uint8* buffer, uint32& out_value) {
  // fast path: we have enough bytes left in the buffer to guarantee that
  // this read won't cross the end, so we can skip the checks.
  const uint8* ptr = buffer;

  uint32 byte;
  uint32 result;

  byte = *ptr++; result = byte; if (!(byte & 0x80)) goto Done;
  result -= 0x80;

  byte = *ptr++; result += byte <<  7; if (!(byte & 0x80)) goto Done;
  result -= 0x80 << 7;

  byte = *ptr++; result += byte << 14; if (!(byte & 0x80)) goto Done;
  result -= 0x80 << 14;

  byte = *ptr++; result += byte << 21; if (!(byte & 0x80)) goto Done;
  result -= 0x80 << 21;

  byte = *ptr++; result += byte << 28; if (!(byte & 0x80)) goto Done;
  // "result -= 0x80 << 28" is irrevelant.

  // if the input is larger than 32 bits, we still need to read it all
  // and discard the high-order bits.
  const int32 lookahead_length = MessageFormat::MaxVarint64Length - MessageFormat::MaxVarint32Length;
  for (int32 i = 0; i < lookahead_length; ++i) {
    byte = *ptr++;
    if (!(byte & 0x80)) {
      goto Done;
    }
  }

  // we have overrun the maximum size of a varint (10 bytes).  assume the data is corrupt.
  return nullptr;

Done:
  out_value = result;
  return ptr;
}

const uint8* MessageIn::ReadVarint64FromBuffer(const uint8* buffer, uint64& out_value) {
  // fast path: we have enough bytes left in the buffer to guarantee that
  // this read won't cross the end, so we can skip the checks.

  const uint8* ptr = buffer;

  uint32 byte;

  // splitting into 32-bit pieces gives better performance on 32-bit processors.
  uint32 part0 = 0, part1 = 0, part2 = 0;

  byte = *ptr++; part0 = byte; if (!(byte & 0x80)) goto Done; // 1
  part0 -= 0x80;

  byte = *ptr++; part0 += byte << 7; if (!(byte & 0x80)) goto Done; // 2
  part0 -= 0x80 << 7;

  byte = *ptr++; part0 += byte << 14; if (!(byte & 0x80)) goto Done; // 3
  part0 -= 0x80 << 14;

  byte = *ptr++; part0 += byte << 21; if (!(byte & 0x80)) goto Done; // 4
  part0 -= 0x80 << 21;

  byte = *ptr++; part1 = byte; if (!(byte & 0x80)) goto Done; // 5
  part1 -= 0x80;

  byte = *ptr++; part1 += byte << 7; if (!(byte & 0x80)) goto Done; // 6
  part1 -= 0x80 << 7;

  byte = *ptr++; part1 += byte << 14; if (!(byte & 0x80)) goto Done; // 7
  part1 -= 0x80 << 14;

  byte = *ptr++; part1 += byte << 21; if (!(byte & 0x80)) goto Done; // 8
  part1 -= 0x80 << 21;

  byte = *ptr++; part2 = byte; if (!(byte & 0x80)) goto Done; // 9
  part2 -= 0x80;

  byte = *ptr++; part2 += byte << 7; if (!(byte & 0x80)) goto Done; // 10 - worst case!
  // "part2 -= 0x80 << 7" is irrelevant because (0x80 << 7) << 56 is 0.

  // we have overrun the maximum size of a varint (10 bytes).  the data must be corrupt.
  return nullptr;

Done:
  out_value = (static_cast<uint64>(part0)) | (static_cast<uint64>(part1) << 28) | (static_cast<uint64>(part2) << 56);
  return ptr;
}

bool MessageIn::ReadVarint8Slow(uint8& out_value) {
  uint64 tmp;
  if (ReadVarint64Slow(tmp)) {
    out_value = static_cast<uint8>(tmp);
    return true;
  }
  return false;
}

bool MessageIn::ReadVarint16Slow(uint16& out_value) {
  uint64 tmp;
  if (ReadVarint64Slow(tmp)) {
    out_value = static_cast<uint16>(tmp);
    return true;
  }
  return false;
}

bool MessageIn::ReadVarint32Slow(uint32& out_value) {
  uint64 tmp;
  if (ReadVarint64Slow(tmp)) {
    out_value = static_cast<uint32>(tmp);
    return true;
  }
  return false;
}


// Read one byte at a time. I think I could stream here, but ...
// Do not support streaming for simplicity!
// If the MSB is 1, continue looping the dome.
// not read indefinitely but only up to 10 bytes, the maximum number of bytes of a 64-bit varint,
// If you try to read more then throw an exception.
bool MessageIn::ReadVarint64Slow(uint64& out_value) {
  uint64 result = 0;

  int32 length = 0;
  int32 shift = 0;
  uint8 byte;

  do {
    // If the maximum number of bytes of a 64-bit varint is exceeded, the base-128 encoding is incorrect
    // Failed because the data is broken.
    if (buffer_ == buffer_end_ || length == MessageFormat::MaxVarint64Length) {
      if (exceptions_enabled_) {
        throw MessageFormatException::MalformedVarint();
      } else {
        return false;
      }
    }

    byte = *buffer_;
    Advance(1);

    result |= static_cast<uint64>(byte & 0x7f) << shift;
    shift += 7;

    length++;
  }
  while (byte & 0x80);

  out_value = result;
  return true;
}


//
// Limitation
//

void MessageIn::SetRecursionLimit(int32 new_limit) {
  recursion_limit_ = new_limit;
}

void MessageIn::IncreaseRecursionDepth() {
  if (recursion_depth_ > recursion_limit_) {
    throw MessageFormatException::RecursionLimitExceeded();
  }

  recursion_depth_++;
}

void MessageIn::DecreaseRecursionDepth() {
  if (recursion_depth_ <= 0) {
    throw MessageFormatException::UnderflowRecursionDepth();
  }

  recursion_depth_--;
}

MessageInView MessageIn::PushView() {
  return MessageInView(buffer_start_, buffer_end_);
}

void MessageIn::PopView(const MessageInView& previous_view) {
  if (previous_view.offset < original_buffer_ || previous_view.end < buffer_end_) {
    throw IndexOutOfBoundsException();
  }

  buffer_start_ = previous_view.offset;
  buffer_end_ = previous_view.end;
}

void MessageIn::AdjustView(int32 length) {
  if (length < 0 || (buffer_ + length) > buffer_end_) {
    throw IndexOutOfBoundsException();
  }

  buffer_start_ = buffer_;
  buffer_end_ = buffer_ + length;
}

bool MessageIn::IsViewAdjusted() const {
  return buffer_end_ != original_buffer_end_;
}

void MessageIn::Detach() {
  fun_check(!IsViewAdjusted());

  if (!sharable_buffer_.IsDetached()) {
    const int32 old_position = Tell();
    const int32 old_master_view_offset = GetMasterViewOffset();
    const int32 old_master_view_length = GetMasterViewLength();

    // Detach shared buffer. (become to non-shared)
    sharable_buffer_.Detach();

    // 버퍼가 변경 되었을 수 있으므로, MasterView와 읽기 위치를 갱신해야함.
    SetupMasterView(old_master_view_offset, old_master_view_length);
    Seek(old_position);
  }
}

bool MessageIn::IsDetached() const {
  return sharable_buffer_.IsDetached();
}

void MessageIn::CheckConsistency() const {
  //TODO
}

void MessageIn::EnsureAtOrigin() {
  if (Tell() != 0) {
    throw IndexOutOfBoundsException();
  }
}

} // namespace net
} // namespace fun
