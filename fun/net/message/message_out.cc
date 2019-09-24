#include "fun/net/message/message_out.h"
#include "fun/net/message/message_format_exception.h"

namespace fun {
namespace net {

MessageOut::MessageOut(int32 initial_capacity)
    : maximum_message_length_(MessageFormatConfig::message_max_length),
      written_length_(0),
      locked_ptr_(nullptr),
      locked_length_(-1) {
  min_capacity_ = initial_capacity > 0 ? initial_capacity
                                       : MessageFormatConfig::MessageMinLength;

  if (min_capacity_ > maximum_message_length_) {
    throw InvalidArgumentException(
        "Minimum buffer size can not exceed maximum_message_length_.");
  }
}

MessageOut::MessageOut(ByteArray data)
    : maximum_message_length_(MessageFormatConfig::message_max_length),
      locked_ptr_(nullptr),
      locked_length_(-1),
      sharable_buffer_(data),
      written_length_(data.Len()),
      min_capacity_(data.Len()) {}

MessageOut::MessageOut(const MessageOut& rhs)
    : maximum_message_length_(rhs.maximum_message_length_),
      written_length_(rhs.written_length_),
      locked_ptr_(nullptr),
      locked_length_(-1),
      sharable_buffer_(rhs.sharable_buffer_),
      min_capacity_(rhs.min_capacity_) {
  rhs.EnsureNotLocked();
}

MessageOut& MessageOut::operator=(const MessageOut& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    rhs.EnsureNotLocked();

    maximum_message_length_ = rhs.maximum_message_length_;
    written_length_ = rhs.written_length_;
    locked_ptr_ = nullptr;
    locked_length_ = -1;
    sharable_buffer_ = rhs.sharable_buffer_;
    min_capacity_ = rhs.min_capacity_;
  }

  return *this;
}

int32 MessageOut::GetMessageMaxLength() const {
  return maximum_message_length_;
}

void MessageOut::SetMessageMaxLength(int32 maximum_length) {
  maximum_message_length_ = maximum_length;

  if (written_length_ > maximum_message_length_) {
    written_length_ = maximum_message_length_;
  }
}

//
// Fixed
//

void MessageOut::WriteFixed8(uint8 value) {
  if (uint8* target = RequireWritableSpace(1)) {
    WriteFixed8ToBuffer(value, target);
    Advance(1);
  }
}

void MessageOut::WriteFixed16(uint16 value) {
  if (uint8* target = RequireWritableSpace(2)) {
    WriteFixed16ToBuffer(value, target);
    Advance(2);
  }
}

void MessageOut::WriteFixed32(uint32 value) {
  if (uint8* target = RequireWritableSpace(4)) {
    WriteFixed32ToBuffer(value, target);
    Advance(4);
  }
}

void MessageOut::WriteFixed64(uint64 value) {
  if (uint8* target = RequireWritableSpace(8)) {
    WriteFixed64ToBuffer(value, target);
    Advance(8);
  }
}

//
// Varint
//

void MessageOut::WriteVarint8(uint8 value) {
  if (uint8* target = RequireWritableSpace(MessageFormat::MaxVarint8Length)) {
    uint8* end = WriteVarint8FallbackToBufferInline(value, target);
    Advance(int32(end - target));
  }
}

void MessageOut::WriteVarint16(uint16 value) {
  if (uint8* target = RequireWritableSpace(MessageFormat::MaxVarint16Length)) {
    uint8* end = WriteVarint16FallbackToBufferInline(value, target);
    Advance(int32(end - target));
  }
}

void MessageOut::WriteVarint32(uint32 value) {
  if (uint8* target = RequireWritableSpace(MessageFormat::MaxVarint32Length)) {
    uint8* end = WriteVarint32FallbackToBufferInline(value, target);
    Advance(int32(end - target));
  }
}

void MessageOut::WriteVarint64(uint64 value) {
  if (uint8* target = RequireWritableSpace(MessageFormat::MaxVarint64Length)) {
    uint8* end = WriteVarint64ToBufferInline(value, target);
    Advance(int32(end - target));
  }
}

void MessageOut::WriteRawBytes(const void* data, int32 length) {
  if (uint8* target = RequireWritableSpace(length)) {
    UnsafeMemory::Memcpy(target, data, length);
    Advance(length);
  }
}

void MessageOut::RemoveRange(int32 index, int32 length_to_remove) {
  if (index < 0 || index > written_length_ || length_to_remove < 0 ||
      (index + length_to_remove) > written_length_) {
    throw IndexOutOfBoundsException();
  }

  sharable_buffer_.Remove(index, length_to_remove);
  written_length_ -= length_to_remove;
}

uint8* MessageOut::RequireWritableSpace(int32 length) {
  const int32 required_buffer_length = written_length_ + length;

  if (required_buffer_length > sharable_buffer_.Len()) {
    if (required_buffer_length > maximum_message_length_) {
      throw MessageFormatException::MessageOutLengthLimited(
          required_buffer_length, maximum_message_length_);
    } else {
      int32 need_more_length = required_buffer_length - sharable_buffer_.Len();
      if (need_more_length < min_capacity_) {
        need_more_length = min_capacity_;
      }

      sharable_buffer_.AppendUninitialized(need_more_length);
    }
  }

  return (uint8*)sharable_buffer_.MutableData() + written_length_;
}

void MessageOut::Advance(int32 length) {
  if ((written_length_ + length) > sharable_buffer_.Len()) {
    throw IndexOutOfBoundsException();
  }

  written_length_ += length;
}

void MessageOut::AddWrittenBytes(int32 length) {
  throw MessageFormatException::Misuse(
      StringLiteral("the AddWrittenBytes function can only be called from "
                    "MessageByteCounter."));
}

MessageIn MessageOut::ToMessageIn() const { return MessageIn(*this); }

MessageIn MessageOut::ToMessageIn(int32 offset, int32 length) const {
  return MessageIn(*this, offset, length);
}

ByteArray MessageOut::ToBytesCopy() const {
  ByteArray ret = sharable_buffer_;
  ret.Truncate(GetLength());  // copy or ref
  return ret;
}

ByteArray MessageOut::ToBytesCopy(int32 offset, int32 length) const {
  if (offset < 0 || offset > written_length_ || length < 0 ||
      (offset + length) > written_length_) {
    throw IndexOutOfBoundsException();
  }

  if (offset == 0) {
    ByteArray ret = sharable_buffer_;
    ret.Truncate(length);  // copy or ref
    return ret;
  } else {
    return sharable_buffer_.Mid(offset, length);  // copy
  }
}

ByteArray MessageOut::ToBytesRaw() const {
  return ByteArray::FromRawData(sharable_buffer_.ConstData(), written_length_);
}

ByteArray MessageOut::ToBytesRaw(int32 offset, int32 length) const {
  if (offset < 0 || offset > written_length_ || length < 0 ||
      (offset + length) > written_length_) {
    throw IndexOutOfBoundsException();
  }

  return ByteArray::FromRawData(sharable_buffer_.ConstData() + offset, length);
}

ByteArrayView MessageOut::ToBytesView() const {
  return ByteArrayView(sharable_buffer_.ConstData(), written_length_);
}

ByteArrayView MessageOut::ToBytesView(int32 offset, int32 length) const {
  if (offset < 0 || offset > written_length_ || length < 0 ||
      (offset + length) > written_length_) {
    throw IndexOutOfBoundsException();
  }

  return ByteArrayView(sharable_buffer_.ConstData() + offset, length);
}

void MessageOut::Trim() {
  if (sharable_buffer_.Len() > 0 && written_length_ != sharable_buffer_.Len()) {
    sharable_buffer_.Truncate(written_length_);
  }
}

bool MessageOut::IsTrimmed() const {
  return sharable_buffer_.Len() == written_length_;
}

uint8* MessageOut::LockForWrite(int32 length) {
  EnsureNotLocked();

  if (length < 0) {
    throw IndexOutOfBoundsException();
  }

  if ((written_length_ + length) > maximum_message_length_) {
    throw IndexOutOfBoundsException();
  }

  locked_ptr_ = RequireWritableSpace(length);
  locked_length_ = length;
  return locked_ptr_;
}

void MessageOut::Unlock(int32 length, bool trimming) {
  EnsureLocked();

  if (length < 0) {
    length = locked_length_;
  } else {
    if (length > locked_length_) {
      throw IndexOutOfBoundsException();
    }
  }

  Advance(length);

  locked_ptr_ = nullptr;
  locked_length_ = -1;

  if (trimming) {
    Trim();
  }
}

bool MessageOut::IsLocked() const { return !!locked_ptr_; }

void MessageOut::EnsureNotLocked() const {
  if (locked_ptr_) {
    throw LogicException("Must be not locked.");
  }
}

void MessageOut::EnsureLocked() const {
  if (!locked_ptr_) {
    throw LogicException("Must be locked.");
  }
}

void MessageOut::Detach() {
  EnsureNotLocked();

  sharable_buffer_.Detach();
}

bool MessageOut::IsDetached() const { return sharable_buffer_.IsDetached(); }

}  // namespace net
}  // namespace fun
