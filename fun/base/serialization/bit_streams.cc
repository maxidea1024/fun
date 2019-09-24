#include "fun/base/serialization/bit_streams.h"

namespace fun {

// Table.
static const uint8 SHIFT_TABLE[8] = {0x01, 0x02, 0x04, 0x08,
                                     0x10, 0x20, 0x40, 0x80};
static const uint8 MASK_TABLE[8] = {0x00, 0x01, 0x03, 0x07,
                                    0x0f, 0x1f, 0x3f, 0x7f};

// Optimized arbitrary bit range memory copy routine.

// TODO UnsafeMemory 쪽으로 옮겨주는게 어떨런지??
void appBitsCpy(uint8* dst, int32 dest_bit, const uint8* src, int32 src_bit,
                int32 bit_count) {
  if (bit_count == 0) {
    return;
  }

  // Special case - always at least one bit to copy,
  // a maximum of 2 bytes to read, 2 to write - only touch bytes that are
  // actually used.
  if (bit_count <= 8) {
    uint32 dst_index = dest_bit / 8;
    uint32 src_index = src_bit / 8;
    uint32 last_dest = (dest_bit + bit_count - 1) / 8;
    uint32 last_src = (src_bit + bit_count - 1) / 8;
    uint32 shift_src = src_bit & 7;
    uint32 shift_dest = dest_bit & 7;
    uint32 first_mask = 0xFF << shift_dest;
    uint32 last_mask =
        0xFE << ((dest_bit + bit_count - 1) & 7);  // Pre-shifted left by 1.
    uint32 accu;

    if (src_index == last_src) {
      accu = (src[src_index] >> shift_src);
    } else {
      accu =
          ((src[src_index] >> shift_src) | (src[last_src] << (8 - shift_src)));
    }

    if (dst_index == last_dest) {
      uint32 multi_mask = first_mask & ~last_mask;
      dst[dst_index] = ((dst[dst_index] & ~multi_mask) |
                        ((accu << shift_dest) & multi_mask));
    } else {
      dst[dst_index] = (uint8)((dst[dst_index] & ~first_mask) |
                               ((accu << shift_dest) & first_mask));
      dst[last_dest] = (uint8)((dst[last_dest] & last_mask) |
                               ((accu >> (8 - shift_dest)) & ~last_mask));
    }

    return;
  }

  // Main copier, uses byte sized shifting. Minimum size is 9 bits, so at least
  // 2 reads and 2 writes.
  uint32 dst_index = dest_bit / 8;
  uint32 first_src_mask = 0xFF << (dest_bit & 7);
  uint32 last_dest = (dest_bit + bit_count) / 8;
  uint32 last_src_mask = 0xFF << ((dest_bit + bit_count) & 7);
  uint32 src_index = src_bit / 8;
  uint32 last_src = (src_bit + bit_count) / 8;
  int32 shift_count = (dest_bit & 7) - (src_bit & 7);
  int32 dest_loop = last_dest - dst_index;
  int32 src_loop = last_src - src_index;
  uint32 full_loop;
  uint32 bit_accu;

  // Lead-in needs to read 1 or 2 source bytes depending on alignment.
  if (shift_count >= 0) {
    full_loop = MathBase::Max(dest_loop, src_loop);
    bit_accu = src[src_index] << shift_count;
    shift_count += 8;  // prepare for the inner loop.
  } else {
    shift_count += 8;  // turn shifts -7..-1 into +1..+7
    full_loop = MathBase::Max(dest_loop, src_loop - 1);
    bit_accu = src[src_index] << shift_count;
    src_index++;
    shift_count += 8;  // Prepare for inner loop.
    bit_accu = (((uint32)src[src_index] << shift_count) + (bit_accu)) >> 8;
  }

  // Lead-in - first copy.
  dst[dst_index] =
      (uint8)((bit_accu & first_src_mask) | (dst[dst_index] & ~first_src_mask));
  src_index++;
  dst_index++;

  // Fast inner loop.
  for (; full_loop > 1; full_loop--) {
    // shift_count ranges from 8 to 15 - all reads are relevant.
    bit_accu = (((uint32)src[src_index] << shift_count) + (bit_accu)) >>
               8;  // Copy in the new, discard the old.
    src_index++;
    dst[dst_index] = (uint8)bit_accu;  // Copy low 8 bits.
    dst_index++;
  }

  // Lead-out.
  if (last_src_mask != 0xFF) {
    if ((uint32)(src_bit + bit_count - 1) / 8 ==
        src_index) {  // Last legal byte ?
      bit_accu = (((uint32)src[src_index] << shift_count) + (bit_accu)) >> 8;
    } else {
      bit_accu = bit_accu >> 8;
    }

    dst[dst_index] =
        (uint8)((dst[dst_index] & last_src_mask) | (bit_accu & ~last_src_mask));
  }
}

//
// BitWriter.
//

BitWriter::BitWriter(int64 max_bit_count, bool allow_resize)
    : count_(0), max_(max_bit_count) {
  buffer_.AddUninitialized((max_bit_count + 7) >> 3);

  allow_resize_ = allow_resize;
  UnsafeMemory::Memzero(buffer_.MutableData(), buffer_.Count());
  // ArIsPersistent = ArIsSaving = 1;
  // ArNetVer |= 0x80000000;

  SetPersistent(true);
  SetSaving(true);

  // TODO
  // ArNetVer |= 0x80000000;
}

BitWriter::BitWriter() : count_(0), max_(0), allow_resize_(false) {}

void BitWriter::Reset() {
  Archive::Reset();

  count_ = 0;
  UnsafeMemory::Memzero(buffer_.MutableData(), buffer_.Count());

  // ArIsPersistent = ArIsSaving = 1;
  // ArNetVer |= 0x80000000;

  SetPersistent(true);
  SetSaving(true);

  // TODO
  // ArNetVer |= 0x80000000;
}

void BitWriter::SerializeBits(void* src, int64 bit_count) {
  if (AllowAppend(bit_count)) {
    // for (int32 i=0; i<bit_count; i++,count_++)
    //  if (((uint8*)src)[i>>3] & SHIFT_TABLE[i&7])
    //    buffer_(count_>>3) |= SHIFT_TABLE[count_&7];
    if (bit_count == 1) {
      if (((uint8*)src)[0] & 0x01) {
        buffer_[count_ >> 3] |= SHIFT_TABLE[count_ & 7];
      }
      count_++;
    } else {
      appBitsCpy(buffer_.MutableData(), count_, (uint8*)src, 0, bit_count);
      count_ += bit_count;
    }
  } else {
    SetError();
  }
}

void BitWriter::Serialize(void* src, int64 byte_count) {
  // warning: Copied and pasted from BitWriter::SerializeBits
  int64 bit_count = byte_count * 8;
  if (AllowAppend(bit_count)) {
    appBitsCpy(buffer_.MutableData(), count_, (uint8*)src, 0, bit_count);
    count_ += bit_count;
  } else {
    SetError();
  }
}

void BitWriter::SerializeInt(uint32& value, uint32 value_max) {
  uint32 write_value = value;
  if (write_value >= value_max) {
    // TODO
    // fun_log(LogSerialization, Error, "BitWriter::SerializeInt(): value out of
    // bounds (value: %u, value_max: %u)", write_value, value_max);
    // ENSURE_MSGF(false, "BitWriter::SerializeInt(): value out of bounds
    // (value: %u, value_max: %u)", write_value, value_max);
    write_value = value_max - 1;
  }

  if (AllowAppend((int32)MathBase::CeilLogTwo(value_max))) {
    uint32 new_value = 0;
    int64 local_count = count_;  // Use local var to avoid Lhs
    for (uint32 mask = 1; new_value + mask < value_max && mask;
         mask *= 2, local_count++) {
      if (write_value & mask) {
        buffer_[local_count >> 3] += SHIFT_TABLE[local_count & 7];
        new_value += mask;
      }
    }
    count_ = local_count;
  } else {
    SetError();
  }
}

/**
 * serializes the specified value, but does not bounds check against max_;
 * instead, it will wrap around if the value exceeds max_
 *
 * (this differs from SerializeInt(), which clamps)
 *
 * @param value - the value to serialize
 * @param value_max - maximum value to write; wrap Result if it exceeds this
 */
void BitWriter::WriteIntWrapped(uint32 value, uint32 value_max) {
  if (AllowAppend((int32)MathBase::CeilLogTwo(value_max))) {
    uint32 new_value = 0;
    for (uint32 mask = 1; new_value + mask < value_max && mask;
         mask *= 2, count_++) {
      if (value & mask) {
        buffer_[count_ >> 3] += SHIFT_TABLE[count_ & 7];
        new_value += mask;
      }
    }
  } else {
    SetError();
  }
}

void BitWriter::WriteBit(uint8 bit) {
  if (AllowAppend(1)) {
    if (bit) {
      buffer_[count_ >> 3] |= SHIFT_TABLE[count_ & 7];
    }
    count_++;
  } else {
    SetError();
  }
}

//
// BitWriterMark.
//

void BitWriterMark::Pop(BitWriter& writer) {
  fun_check_dbg(count_ <= writer.count_);
  fun_check_dbg(count_ <= writer.max_);

  if (count_ & 7) {
    writer.buffer_[count_ >> 3] &= MASK_TABLE[count_ & 7];
  }

  const int32 start = (count_ + 7) >> 3;
  const int32 end = (writer.count_ + 7) >> 3;
  if (end != start) {
    fun_check_dbg(start < writer.buffer_.Count());
    fun_check_dbg(end <= writer.buffer_.Count());
    UnsafeMemory::Memzero(&writer.buffer_[start], end - start);
  }
  writer.SetError(overflowed_);
  writer.count_ = count_;
}

void BitWriterMark::Copy(BitWriter& writer, Array<uint8>& buffer) {
  fun_check_dbg(count_ <= writer.count_);
  fun_check_dbg(count_ <= writer.max_);

  const int32 byte_count = (writer.count_ - count_ + 7) >> 3;
  if (byte_count > 0) {
    buffer.ResizeUninitialized(byte_count);  // This makes room but doesnt zero
    buffer[byte_count - 1] = 0;  // Make sure the last byte is 0 out, because
                                 // appBitsCpy wont touch the last bits
    appBitsCpy(buffer.MutableData(), 0, writer.buffer_.ConstData(), count_,
               writer.count_ - count_);
  }
}

void BitWriterMark::PopWithoutClear(BitWriter& writer) {
  writer.count_ = count_;
}

//
// BitReader.
//

BitReader::BitReader(uint8* src, int64 bit_count) : count_(bit_count), pos_(0) {
  // ArIsPersistent = ArIsLoading = 1;
  // ArNetVer |= 0x80000000;
  SetPersistent(true);
  SetLoading(true);

  // TODO
  // ArNetVer |= 0x80000000;

  buffer_.AddUninitialized((bit_count + 7) >> 3);
  if (src) {
    UnsafeMemory::Memcpy(buffer_.MutableData(), src, (bit_count + 7) >> 3);
  }
}

void BitReader::SetData(BitReader& src, int64 bit_count) {
  count_ = bit_count;
  pos_ = 0;
  SetError(false);
  buffer_.Clear();
  buffer_.AddUninitialized((bit_count + 7) >> 3);
  src.SerializeBits(buffer_.MutableData(), bit_count);
}

/**
 * This appends data from another BitReader.
 * It checks that this bit reader is byte-aligned so it can
 * just do a Array::Append instead of a bitcopy.
 *
 * It is intended to be used by performance minded code that wants to
 * ensure an appBitCpy is avoided.
 */
void BitReader::AppendDataFromChecked(BitReader& src) {
  fun_check(count_ % 8 == 0);
  src.AppendTo(buffer_);
  count_ += src.GetBitCount();
}

void BitReader::AppendDataFromChecked(uint8* src, uint32 bit_count) {
  fun_check(count_ % 8 == 0);
  uint32 byte_count = (bit_count + 7) >> 3;
  buffer_.AddUninitialized(byte_count);
  UnsafeMemory::Memcpy(&buffer_[count_ >> 3], src, byte_count);
  count_ += bit_count;
}

void BitReader::AppendTo(Array<uint8>& dest_buffer) {
  dest_buffer.Append(buffer_);
}

void BitReader::SetOverflowed() {
  // TODO
  // fun_log(LogNetSerialization, Error, "BitReader::SetOverflowed() called");
  SetError();
}

//
// BitReader.
//

void BitReaderMark::Copy(BitReader& reader, Array<uint8>& buffer) {
  fun_check_dbg(pos_ <= reader.pos_);

  int32 byte_count = (reader.pos_ - pos_ + 7) >> 3;
  if (byte_count > 0) {
    buffer.ResizeUninitialized(byte_count);  // This makes room but doesnt zero
    buffer[byte_count - 1] = 0;  // Make sure the last byte is 0 out, because
                                 // appBitsCpy wont touch the last bits
    appBitsCpy(buffer.MutableData(), 0, reader.buffer_.ConstData(), pos_,
               reader.pos_ - pos_);
  }
}

}  // namespace fun
