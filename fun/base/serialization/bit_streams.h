#pragma once

#include "fun/base/base.h"
#include "fun/base/serialization/archive.h"
#include "fun/base/container/array.h"

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

namespace fun {

//TODO UnsafeMemory쪽으로 옮겨주는게 어떨런지??
FUN_BASE_API void appBitsCpy(uint8* dst, int32 dest_bit, const uint8* src, int32 src_bit, int32 bit_count);

/**
 * Writes bitstreams.
 */
class FUN_BASE_API BitWriter : public Archive {
 public:
  friend class BitWriterMark;

  /**
   * Default constructor. Zeros everything.
   */
  BitWriter();

  /**
   * Constructor using known size the buffer needs to be
   */
  BitWriter(int64 max_bits, bool allow_resize = false);

  void SerializeBits(void* src, int64 bit_count);

  void SerializeInt(uint32& value, uint32 max);

  /**
   * Serializes the specified value, but does not bounds check against Max;
   * instead, it will wrap around if the value exceeds Max
   * (this differs from SerializeInt(), which clamps)
   *
   * @param value - the value to serialize
   * @param max - maximum value to write; wrap Result if it exceeds this
   */
  void WriteIntWrapped(uint32 value, uint32 max);

  void WriteBit(uint8 bit);

  void Serialize(void* src, int64 byte_count);

  /**
   * Returns a pointer to our internal buffer.
   */
  inline uint8* MutableData() {
    return buffer_.MutableData();
  }

  inline const uint8* ConstData() const {
    return buffer_.ConstData();
  }

  inline const Array<uint8>* GetBuffer() const {
    return &buffer_;
  }

  /**
   * Returns the number of bytes written.
   */
  inline int64 GetByteCount() const {
    return (count_ + 7) >> 3;
  }

  /**
   * Returns the number of bits written.
   */
  inline int64 GetBitCount() const {
    return count_;
  }

  /**
   * Returns the number of bits the buffer supports.
   */
  inline int64 GetMaxBits() const {
    return max_;
  }

  /**
   * Marks this bit writer as overflowed.
   */
  inline void SetOverflowed() {
    SetError();
  }

  inline bool AllowAppend(int64 bit_count) {
    if ((count_ + bit_count) > max_) {
      if (allow_resize_) {
        // Resize our buffer. The common case for resizing bitwriters
        // is hitting the max and continuing to add a lot of
        // small segments of data
        // Though we could just allow the Array buffer to handle the slack
        // and resizing, we would still constantly hit the BitWriter's max
        // and cause this block to be executed, as well as constantly zeroing
        // out memory inside AddZeroes (though the memory would be allocated
        // in chunks).

        max_ = MathBase::Max<int64>(max_ << 1, count_ + bit_count);
        int32 byte_max = (max_ + 7) >> 3;
        buffer_.AddZeroed(byte_max - buffer_.Count());
        fun_check((max_ + 7) >> 3 == buffer_.Count());
        return true;
      } else {
        return false;
      }
    }
    return true;
  }

  inline void SetAllowResize(bool allow) {
    allow_resize_ = allow;
  }

  /**
   * Resets the bit writer back to its initial state
   */
  void Reset();

  inline void WriteAlign() {
    count_ = (count_+ 7) & (~7);
  }

 private:
  Array<uint8> buffer_;
  int64 count_;
  int64 max_;
  bool allow_resize_;
};


/**
 * For pushing and popping BitWriter positions.
 */
class FUN_BASE_API BitWriterMark {
 public:
  BitWriterMark() : overflowed_(false), count_(0) {}

  BitWriterMark(BitWriter& writer) {
    Init(writer);
  }

  int64 GetBitCount() {
    return count_;
  }

  void Init(BitWriter& writer) {
    count_ = writer.count_;
    overflowed_ = writer.GetError();
  }

  void Pop(BitWriter& writer);

  void Copy(BitWriter& writer, Array<uint8>& buffer);

  void PopWithoutClear(BitWriter& writer);

 private:
  bool overflowed_;
  int64 count_;
};


/**
 * Reads bitstreams.
 */
class FUN_BASE_API BitReader : public Archive {
  friend class BitReaderMark;

 public:
  BitReader(uint8* src = nullptr, int64 bit_count = 0);

  void SetData(BitReader& src, int64 bit_count);

  FUN_ALWAYS_INLINE_DEBUGGABLE void SerializeBits(void* dst, int64 bit_count) {
    if (GetError() || pos_ + bit_count > count_) {
      if (!GetError()) {
        SetOverflowed();
        //TODO?
        //fun_log(LogNetSerialization, Error, "BitReader::SerializeBits: pos_ + bit_count > count_");
      }
      UnsafeMemory::Memzero(dst, (bit_count + 7) >> 3);
      return;
    }
    //for (int32 i = 0; i < bit_count; ++i, ++pos_)
    //  if (buffer(pos_>>3) & GShift[pos_&7])
    //      ((uint8*)dst)[i>>3] |= GShift[i&7];
    if (bit_count == 1) {
      ((uint8*)dst)[0] = 0;
      if (buffer_[pos_ >> 3] & Shift(pos_ & 7)) {
        ((uint8*)dst)[0] |= 0x01;
      }
      pos_++;
    } else if (bit_count != 0) {
      ((uint8*)dst)[((bit_count + 7) >> 3) - 1] = 0;
      appBitsCpy((uint8*)dst, 0, buffer_.ConstData(), pos_, bit_count);
      pos_ += bit_count;
    }
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE
  void SerializeInt(uint32& out_value, uint32 value_max) {
    if (GetError()) {
      return;
    }

    uint32 value = 0; // Use local variable to avoid Load-Hit-Store
    int64 local_pos = pos_;
    const int64 local_count = count_;

    for (uint32 mask = 1; value + mask < value_max && mask; mask *= 2, ++local_pos) {
      if (local_pos >= local_count) {
        SetOverflowed();
        //fun_log(LogNetSerialization, Error, "BitReader::SerializeInt: local_pos >= local_count");
        break;
      }
      if (buffer_[local_pos >> 3] & Shift(local_pos & 7)) {
        value |= mask;
      }
    }
    // Now write back
    pos_ = local_pos;
    out_value = value;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE uint32 ReadInt(uint32 max) {
    uint32 value = 0;
    SerializeInt(value, max);
    return value;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE uint8 ReadBit() {
    uint8 bit = 0;
    //SerializeBits(&bit, 1);
    if (!GetError()) {
      int64 local_pos = pos_;
      const int64 local_count = count_;
      if (local_pos >= local_count) {
        SetOverflowed();
        //fun_log(LogNetSerialization, Error, "BitReader::SerializeInt: local_pos >= local_count");
      } else {
        bit = !!(buffer_[local_pos >> 3] & Shift(local_pos & 7));
        pos_++;
      }
    }
    return bit;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE void Serialize(void* dst, int64 byte_count) {
    SerializeBits(dst, byte_count * 8);
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE const uint8* ConstData() const {
    return buffer_.ConstData();
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE const uint8* ConstDataPosChecked() const {
    fun_check((pos_ & 7) == 0);
    return &buffer_[pos_ >> 3];
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE uint32 GetBytesLeft() const {
    return ((count_ - pos_) + 7) >> 3;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE uint32 GetBitsLeft() const {
    return (count_ - pos_);
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE bool AtEnd() const {
    return GetError() || pos_ >= count_;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE int64 GetByteCount() const {
    return (count_ + 7) >> 3;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE int64 GetBitCount() const {
    return count_;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE int64 GetPosBits() const {
    return pos_;
  }

  FUN_ALWAYS_INLINE_DEBUGGABLE void EatByteAlign() {
    // Skip over remaining bits in current byte
    pos_ = (pos_ + 7) & (~0x07);
    if (pos_ > count_) {
      //fun_log(LogNetSerialization, Error, "BitReader::EatByteAlign: pos_ > count_");
      SetOverflowed();
    }
  }

  void SetOverflowed();

  void AppendDataFromChecked(BitReader& src);

  void AppendDataFromChecked(uint8* src, uint32 bit_count);

  void AppendTo(Array<uint8>& buffer);

 protected:
  Array<uint8> buffer_;
  int64 count_;
  int64 pos_;

 private:
  inline uint8 Shift(const uint8 n) const {
    return 1 << n;
  }
};


/**
 * For pushing and popping BitWriter positions.
 */
class FUN_BASE_API BitReaderMark {
 public:
  BitReaderMark() : pos_(0) {}

  BitReaderMark(BitReader& reader) : pos_(reader.pos_) {}

  int64 GetPos() { return pos_; }

  void Pop(BitReader& reader) { reader.pos_ = pos_; }

  void Copy(BitReader& reader, Array<uint8>& buffer);

 private:
  int64 pos_;
};

} // namespace fun
