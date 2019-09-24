#pragma once

#include "fun/net/message/imessage_out.h"
#include "fun/net/message/message.h"

namespace fun {
namespace net {

class FUN_NET_API MessageOut : public IMessageOut {
 public:
  MessageOut(int32 min_capacity = -1);
  MessageOut(ByteArray data);
  MessageOut(const MessageOut& rhs);
  MessageOut& operator=(const MessageOut& rhs);

  // IMessageOut interface
  bool CountingOnly() const override { return false; }

  //복사를 줄이기 위해서 참조로 리턴.
  // ByteArrayView AsByteArrayView() const;

  const uint8* ConstData() const override;
  uint8* MutableData() override;

  int32 GetCapacity() const override;
  void SetCapacity(int32 new_capacity) override;

  int32 GetMessageMaxLength() const override;
  void SetMessageMaxLength(int32 maximum_length) override;

  int32 GetLength() const;
  void SetLength(int32 new_len) override;

  void Trim() override;
  bool IsTrimmed() const override;

  void AddWrittenBytes(int32 len) override;

  void RemoveRange(int32 index, int32 length_to_remove) override;
  void WriteRawBytes(const void* data, int32 len) override;

  void WriteFixed8(uint8 value) override;
  void WriteFixed16(uint16 value) override;
  void WriteFixed32(uint32 value) override;
  void WriteFixed64(uint64 value) override;

  void WriteVarint8(uint8 value) override;
  void WriteVarint16(uint16 value) override;
  void WriteVarint32(uint32 value) override;
  void WriteVarint64(uint64 value) override;

  // TODO 아래 함수들이 구태여 인터페이스일 필요는 없는데...
  void WriteVarint8SignExtended(int8 value) override;
  void WriteVarint16SignExtended(int16 value) override;
  void WriteVarint32SignExtended(int32 value) override;

  MessageIn ToMessageIn() const;                         // no copy
  MessageIn ToMessageIn(int32 offset, int32 len) const;  // no copy
  ByteArray ToBytesCopy() const;                         // ocationally copy
  ByteArray ToBytesCopy(int32 offset, int32 len) const;  // with copy
  ByteArray ToBytesRaw() const;
  ByteArray ToBytesRaw(int32 offset, int32 len) const;
  ByteArrayView ToBytesView() const;
  ByteArrayView ToBytesView(int32 offset, int32 len) const;

  uint8* LockForWrite(int32 len);
  void Unlock(int32 len = -1, bool trimming = false);
  bool IsLocked() const;

  void EnsureNotLocked() const;
  void EnsureLocked() const;

  bool IsDetached() const;
  void Detach();

 private:
  friend class MessageIn;

  ByteArray sharable_buffer_;

  int32 min_capacity_;
  int32 maximum_message_length_;
  int32 written_length_;

  uint8* locked_ptr_;
  int32 locked_length_;

  uint8* RequireWritableSpace(int32 len);
  void Advance(int32 len);

  /** like WriteFixed8() but writing directly to the dst buffer. */
  static uint8* WriteFixed8ToBuffer(uint8 value, uint8* dst);
  /** like WriteFixed16() but writing directly to the dst buffer. */
  static uint8* WriteFixed16ToBuffer(uint16 value, uint8* dst);
  /** like WriteFixed32() but writing directly to the dst buffer. */
  static uint8* WriteFixed32ToBuffer(uint32 value, uint8* dst);
  /** like WriteFixed64() but writing directly to the dst buffer. */
  static uint8* WriteFixed64ToBuffer(uint64 value, uint8* dst);

  /** like WriteVarint8() but writing directly to the dst buffer. */
  static uint8* WriteVarint8ToBuffer(uint8 value, uint8* dst);
  /** like WriteVarint16() but writing directly to the dst buffer. */
  static uint8* WriteVarint16ToBuffer(uint16 value, uint8* dst);
  /** like WriteVarint32() but writing directly to the dst buffer. */
  static uint8* WriteVarint32ToBuffer(uint32 value, uint8* dst);
  /** like WriteVarint64() but writing directly to the dst buffer. */
  static uint8* WriteVarint64ToBuffer(uint64 value, uint8* dst);

  static uint8* WriteVarint8SignExtendedToBuffer(int8 value, uint8* dst);
  static uint8* WriteVarint16SignExtendedToBuffer(int16 value, uint8* dst);
  static uint8* WriteVarint32SignExtendedToBuffer(int32 value, uint8* dst);

  static uint8* WriteRawBytesToBuffer(const void* data, int32 len, uint8* dst);

  static uint8* WriteVarint8FallbackToBuffer(uint8 value, uint8* dst);
  static uint8* WriteVarint8FallbackToBufferInline(uint8 value, uint8* dst);

  static uint8* WriteVarint16FallbackToBuffer(uint16 value, uint8* dst);
  static uint8* WriteVarint16FallbackToBufferInline(uint16 value, uint8* dst);

  static uint8* WriteVarint32FallbackToBuffer(uint32 value, uint8* dst);
  static uint8* WriteVarint32FallbackToBufferInline(uint32 value, uint8* dst);
  static uint8* WriteVarint64ToBufferInline(uint64 value, uint8* dst);
};

//
// inlines
//

FUN_ALWAYS_INLINE uint8* MessageOut::WriteFixed8ToBuffer(uint8 value,
                                                         uint8* dst) {
  *dst = value;
  return dst + sizeof(value);
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteFixed16ToBuffer(uint16 value,
                                                          uint8* dst) {
#if FUN_ARCH_BIG_ENDIAN
  dst[0] = static_cast<uint8>(value);
  dst[1] = static_cast<uint8>(value >> 8);
#else
#if FUN_REQUIRES_ALIGNED_ACCESS
  UnsafeMemory::Memcpy(dst, &value, sizeof(value));
#else
  *((uint16*)dst) = value;
#endif
#endif
  return dst + sizeof(value);
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteFixed32ToBuffer(uint32 value,
                                                          uint8* dst) {
#if FUN_ARCH_BIG_ENDIAN
  dst[0] = static_cast<uint8>(value);
  dst[1] = static_cast<uint8>(value >> 8);
  dst[2] = static_cast<uint8>(value >> 16);
  dst[3] = static_cast<uint8>(value >> 24);
#else
#if FUN_REQUIRES_ALIGNED_ACCESS
  UnsafeMemory::Memcpy(dst, &value, sizeof(value));
#else
  *((uint32*)dst) = value;
#endif
#endif
  return dst + sizeof(value);
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteFixed64ToBuffer(uint64 value,
                                                          uint8* dst) {
#if FUN_ARCH_BIG_ENDIAN
  const uint32 part0 = static_cast<uint32>(value);
  const uint32 part1 = static_cast<uint32>(value >> 32);

  dst[0] = static_cast<uint8>(part0);
  dst[1] = static_cast<uint8>(part0 >> 8);
  dst[2] = static_cast<uint8>(part0 >> 16);
  dst[3] = static_cast<uint8>(part0 >> 24);
  dst[4] = static_cast<uint8>(part1);
  dst[5] = static_cast<uint8>(part1 >> 8);
  dst[6] = static_cast<uint8>(part1 >> 16);
  dst[7] = static_cast<uint8>(part1 >> 24);
#else
#if FUN_REQUIRES_ALIGNED_ACCESS
  UnsafeMemory::Memcpy(dst, &value, sizeof(value));
#else
  *((uint64*)dst) = value;
#endif
#endif
  return dst + sizeof(value);
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint8ToBuffer(uint8 value,
                                                          uint8* dst) {
  if (value < (1 << 7)) {
    *dst = value;
    return dst + 1;
  } else {
    return WriteVarint8FallbackToBuffer(value, dst);
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint16ToBuffer(uint16 value,
                                                           uint8* dst) {
  if (value < (1 << 7)) {
    *dst = static_cast<uint8>(value);
    return dst + 1;
  } else {
    return WriteVarint16FallbackToBuffer(value, dst);
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint32ToBuffer(uint32 value,
                                                           uint8* dst) {
  if (value < (1 << 7)) {
    *dst = static_cast<uint8>(value);
    return dst + 1;
  } else {
    return WriteVarint32FallbackToBuffer(value, dst);
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint64ToBuffer(uint64 value,
                                                           uint8* dst) {
  return WriteVarint64ToBufferInline(value, dst);
}

FUN_ALWAYS_INLINE void MessageOut::WriteVarint8SignExtended(int8 value) {
  if (value < 0) {
    WriteVarint64(static_cast<uint64>(value));
  } else {
    WriteVarint8(static_cast<uint8>(value));
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint8SignExtendedToBuffer(
    int8 value, uint8* dst) {
  if (value < 0) {
    return WriteVarint64ToBuffer(static_cast<uint64>(value), dst);
  } else {
    return WriteVarint8ToBuffer(static_cast<uint8>(value), dst);
  }
}

FUN_ALWAYS_INLINE void MessageOut::WriteVarint16SignExtended(int16 value) {
  if (value < 0) {
    WriteVarint64(static_cast<uint64>(value));
  } else {
    WriteVarint16(static_cast<uint16>(value));
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint16SignExtendedToBuffer(
    int16 value, uint8* dst) {
  if (value < 0) {
    return WriteVarint64ToBuffer(static_cast<uint64>(value), dst);
  } else {
    return WriteVarint16ToBuffer(static_cast<uint16>(value), dst);
  }
}

FUN_ALWAYS_INLINE void MessageOut::WriteVarint32SignExtended(int32 value) {
  if (value < 0) {
    WriteVarint64(static_cast<uint64>(value));
  } else {
    WriteVarint32(static_cast<uint32>(value));
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint32SignExtendedToBuffer(
    int32 value, uint8* dst) {
  if (value < 0) {
    return WriteVarint64ToBuffer(static_cast<uint64>(value), dst);
  } else {
    return WriteVarint32ToBuffer(static_cast<uint32>(value), dst);
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteRawBytesToBuffer(const void* data,
                                                           int32 len,
                                                           uint8* dst) {
  UnsafeMemory::Memcpy(dst, data, len);
  return dst + len;
}

// FUN_ALWAYS_INLINE ByteArrayView MessageOut::AsByteArrayView() const {
//  return ByteArrayView(ConstData(), GetLength());
//}

FUN_ALWAYS_INLINE const uint8* MessageOut::ConstData() const {
  return (const uint8*)sharable_buffer_.ConstData();
}

FUN_ALWAYS_INLINE uint8* MessageOut::MutableData() {
  return (uint8*)sharable_buffer_.MutableData();
}

FUN_ALWAYS_INLINE int32 MessageOut::GetCapacity() const {
  return sharable_buffer_.Len();
}

FUN_ALWAYS_INLINE void MessageOut::SetCapacity(int32 new_capacity) {
  sharable_buffer_.ResizeUninitialized(new_capacity);
}

FUN_ALWAYS_INLINE int32 MessageOut::GetLength() const {
  return written_length_;
}

FUN_ALWAYS_INLINE void MessageOut::SetLength(int32 new_len) {
  fun_check(new_len >= 0);
  sharable_buffer_.ResizeUninitialized(new_len);
  written_length_ = new_len;
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint8FallbackToBuffer(uint8 value,
                                                                  uint8* dst) {
  return WriteVarint8FallbackToBufferInline(value, dst);
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint8FallbackToBufferInline(
    uint8 value, uint8* dst) {
  if (value < 0x80) {
    dst[0] = value;
    return dst + 1;
  } else {
    dst[0] = (value & 0x7F) | 0x80;
    dst[1] = value >> 7;
    return dst + 2;
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint16FallbackToBuffer(uint16 value,
                                                                   uint8* dst) {
  return WriteVarint16FallbackToBufferInline(value, dst);
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint16FallbackToBufferInline(
    uint16 value, uint8* dst) {
  dst[0] = static_cast<uint8>(value | 0x80);
  if (value >= (1 << 7)) {
    dst[1] = static_cast<uint8>((value >> 7) | 0x80);
    if (value >= (1 << 14)) {
      dst[2] = static_cast<uint8>(value >> 14);
      return dst + 3;
    } else {
      dst[1] &= 0x7F;
      return dst + 2;
    }
  } else {
    dst[0] &= 0x7F;
    return dst + 1;
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint32FallbackToBuffer(uint32 value,
                                                                   uint8* dst) {
  return WriteVarint32FallbackToBufferInline(value, dst);
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint32FallbackToBufferInline(
    uint32 value, uint8* dst) {
  dst[0] = static_cast<uint8>(value | 0x80);
  if (value >= (1 << 7)) {
    dst[1] = static_cast<uint8>((value >> 7) | 0x80);
    if (value >= (1 << 14)) {
      dst[2] = static_cast<uint8>((value >> 14) | 0x80);
      if (value >= (1 << 21)) {
        dst[3] = static_cast<uint8>((value >> 21) | 0x80);
        if (value >= (1 << 28)) {
          dst[4] = static_cast<uint8>(value >> 28);
          return dst + 5;
        } else {
          dst[3] &= 0x7F;
          return dst + 4;
        }
      } else {
        dst[2] &= 0x7F;
        return dst + 3;
      }
    } else {
      dst[1] &= 0x7F;
      return dst + 2;
    }
  } else {
    dst[0] &= 0x7F;
    return dst + 1;
  }
}

FUN_ALWAYS_INLINE uint8* MessageOut::WriteVarint64ToBufferInline(uint64 value,
                                                                 uint8* dst) {
  // Splitting into 32-bit pieces gives better performance on 32-bit processors.
  const uint32 part0 = static_cast<uint32>(value);
  const uint32 part1 = static_cast<uint32>(value >> 28);
  const uint32 part2 = static_cast<uint32>(value >> 56);

  int32 len;

  // Here we can't really optimize for small numbers, since the value is
  // split into three parts.  Checking for numbers < 128, for instance,
  // would require three comparisons, since you'd have to make sure part1
  // and part2 are zero.  However, if the caller is using 64-bit integers,
  // it is likely that they expect the numbers to often be very large, so
  // we probably don't want to optimize for small numbers anyway.  Thus,
  // we end up with a hard-coded binary search tree...
  if (part2 == 0) {
    if (part1 == 0) {
      if (part0 < (1 << 14)) {
        if (part0 < (1 << 7)) {
          len = 1;
          goto Length01;
        } else {
          len = 2;
          goto Length02;
        }
      } else {
        if (part0 < (1 << 21)) {
          len = 3;
          goto Length03;
        } else {
          len = 4;
          goto Length04;
        }
      }
    } else {
      if (part1 < (1 << 14)) {
        if (part1 < (1 << 7)) {
          len = 5;
          goto Length05;
        } else {
          len = 6;
          goto Length06;
        }
      } else {
        if (part1 < (1 << 21)) {
          len = 7;
          goto Length07;
        } else {
          len = 8;
          goto Length08;
        }
      }
    }
  } else {
    if (part2 < (1 << 7)) {
      len = 9;
      goto Length09;
    } else {
      len = 10;
      goto Length10;
    }
  }

  fun_check(0);  // Couldn't reached at here!

Length10:
  dst[9] = static_cast<uint8>((part2 >> 7) | 0x80);
Length09:
  dst[8] = static_cast<uint8>((part2) | 0x80);
Length08:
  dst[7] = static_cast<uint8>((part1 >> 21) | 0x80);
Length07:
  dst[6] = static_cast<uint8>((part1 >> 14) | 0x80);
Length06:
  dst[5] = static_cast<uint8>((part1 >> 7) | 0x80);
Length05:
  dst[4] = static_cast<uint8>((part1) | 0x80);
Length04:
  dst[3] = static_cast<uint8>((part0 >> 21) | 0x80);
Length03:
  dst[2] = static_cast<uint8>((part0 >> 14) | 0x80);
Length02:
  dst[1] = static_cast<uint8>((part0 >> 7) | 0x80);
Length01:
  dst[0] = static_cast<uint8>((part0) | 0x80);

  dst[len - 1] &= 0x7F;
  return dst + len;
}

}  // namespace net
}  // namespace fun
