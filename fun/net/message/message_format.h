#pragma once

#include "fun/net/message/message.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {
namespace net {

enum class WireType {
  Varint = 0,
  Fixed8 = 1,
  Fixed16 = 2,
  Fixed32 = 3,
  Fixed64 = 4,
  LengthPrefixed = 5,
  Last = 6,
};

FUN_ALWAYS_INLINE TextStream& operator << (TextStream& stream, const WireType value) {
  switch (value) {
    case WireType::Varint: stream << StringLiteral("Varint"); break;
    case WireType::Fixed8: stream << StringLiteral("Fixed8"); break;
    case WireType::Fixed16: stream << StringLiteral("Fixed16"); break;
    case WireType::Fixed32: stream << StringLiteral("Fixed32"); break;
    case WireType::Fixed64: stream << StringLiteral("Fixed64"); break;
    case WireType::LengthPrefixed: stream << StringLiteral("LengthPrefixed"); break;
    case WireType::Last: stream << StringLiteral("Last"); break;
  }
  stream << StringLiteral("<unknown>");
  return stream;
}


class MessageFormat {
 public:
  static const int32 MaxVarint64Length = 10;
  static const int32 MaxVarint32Length = 5;
  static const int32 MaxVarint16Length = 3;
  static const int32 MaxVarint8Length = 2;

  /**
   * returns the number of bytes needed to encode the given value as a varint.
   */
  static int32 GetByteLength_Varint8(const uint8 value) {
    if (value < (1 << 7)) {
      return 1;
    } else {
      return 2;
    }
  }

  /**
   * returns the number of bytes needed to encode the given value as a varint.
   */
  static int32 GetByteLength_Varint16(const uint16 value) {
    if (value < (1 << 7)) {
      return 1;
    } else if (value < (1 << 14)) {
      return 2;
    } else {
      return 3;
    }
  }

  /**
   * returns the number of bytes needed to encode the given value as a varint.
   */
  static int32 GetByteLength_Varint32(const uint32 value) {
    if (value < (1 << 7)) {
      return 1;
    } else if (value < (1 << 14)) {
      return 2;
    } else if (value < (1 << 21)) {
      return 3;
    } else if (value < (1 << 28)) {
      return 4;
    } else {
      return 5;
    }
  }

  /**
   * returns the number of bytes needed to encode the given value as a varint.
   */
  static int32 GetByteLength_Varint64(const uint64 value) {
    if (value < (1ull << 35)) {
      if (value < (1ull << 7)) {
        return 1;
      } else if (value < (1ull << 14)) {
        return 2;
      } else if (value < (1ull << 21)) {
        return 3;
      } else if (value < (1ull << 28)) {
        return 4;
      } else {
        return 5;
      }
    } else {
      if (value < (1ull << 42)) {
        return 6;
      } else if (value < (1ull << 49)) {
        return 7;
      } else if (value < (1ull << 56)) {
        return 8;
      } else if (value < (1ull << 63)) {
        return 9;
      } else {
        return 10;
      }
    }
  }

  /**
   * if negative, 10 bytes.  Otherwise, same as GetByteLength_Varint8().
   */
  static int32 GetByteLength_Varint8SignExtended(const int8 value) {
    if (value < 0) {
      return MaxVarint64Length;
    } else {
      return GetByteLength_Varint8(static_cast<uint8>(value));
    }
  }

  /**
   * if negative, 10 bytes.  Otherwise, same as GetByteLength_Varint16().
   */
  static int32 GetByteLength_Varint16SignExtended(const int16 value) {
    if (value < 0) {
      return MaxVarint64Length;
    } else {
      return GetByteLength_Varint16(static_cast<uint16>(value));
    }
  }

  /**
   * if negative, 10 bytes.  Otherwise, same as GetByteLength_Varint32().
   */
  static int32 GetByteLength_Varint32SignExtended(const int32 value) {
    if (value < 0) {
      return MaxVarint64Length;
    } else {
      return GetByteLength_Varint32(static_cast<uint32>(value));
    }
  }

  //
  // float/double encoding/decoding
  //

  // 여기서는 endian issue는 생각할 필요 없음.
  // 실제로 serializing은 WriteFixed32로 할것인데, WriteFixed32에서
  // endian issue를 핸들링 하니까..
  static uint32 EncodeFloat(const float value) {
    union { float f; uint32 i; };
    f = value;
    return i;
  }

  // 여기서는 endian issue는 생각할 필요 없음.
  // 실제로 serializing은 ReadFixed32로 할것인데, ReadFixed32에서
  // endian issue를 핸들링 하니까..
  static float DecodeFloat(const uint32 value) {
    union { float f; uint32 i; };
    i = value;
    return f;
  }

  // 여기서는 endian issue는 생각할 필요 없음.
  // 실제로 serializing은 WriteFixed64로 할것인데, WriteFixed64에서
  // endian issue를 핸들링 하니까..
  static uint64 EncodeDouble(const double value) {
    union { double d; uint64 i; };
    d = value;
    return i;
  }

  // 여기서는 endian issue는 생각할 필요 없음.
  // 실제로 serializing은 ReadFixed64로 할것인데, ReadFixed64에서
  // endian issue를 핸들링 하니까..
  static double DecodeDouble(const uint64 value) {
    union { double d; uint64 i; };
    i = value;
    return d;
  }


  //
  // zig-zag encoding/decoding for signed varints
  //

  // Helper functions for mapping signed integers to unsigned integers in
  // such a way that numbers with small magnitudes will encode to smaller
  // varints.  If you simply static_cast a negative number to an unsigned
  // number and varint-encode it, it will always take 10 bytes, defeating
  // the purpose of varint.  So, for the "sint32" and "sint64" field types,
  // we ZigZag-encode the values.

  static uint8 ZigZagEncode8(const int8 value) {
    // Note: the right-shift must be arithmetic
    return (value << 1) ^ (value >> 7);
  }

  static int8 ZigZagDecode8(const uint8 value) {
    return (value >> 1) ^ -static_cast<int8>(value & 1);
  }

  static uint16 ZigZagEncode16(const int16 value) {
    // Note: the right-shift must be arithmetic
    return (value << 1) ^ (value >> 15);
  }

  static int16 ZigZagDecode16(const uint16 value) {
    return (value >> 1) ^ -static_cast<int16>(value & 1);
  }

  static uint32 ZigZagEncode32(const int32 value) {
    // Note: the right-shift must be arithmetic
    return (value << 1) ^ (value >> 31);
  }

  static int32 ZigZagDecode32(const uint32 value) {
    return (value >> 1) ^ -static_cast<int32>(value & 1);
  }

  static uint64 ZigZagEncode64(const int64 value) {
    // Note: the right-shift must be arithmetic
    return (value << 1) ^ (value >> 63);
  }

  static int64 ZigZagDecode64(const uint64 value) {
    return (value >> 1) ^ -static_cast<int64>(value & 1);
  }


  static int32 GetByteLength_LengthPrefixed(const int32 length) {
    return GetByteLength_Counter(length) + length;
  }

  static void WriteBool(IMessageOut& output, const bool value) {
    output.WriteFixed8(value ? 1 : 0);
  }

  static bool ReadBool(IMessageIn& input, bool& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadVarint64(tmp));
    out_value = tmp != 0;
    return true;
  }

  static void WriteFloat(IMessageOut& output, const float value) {
    if (output.CountingOnly()) {
      output.AddWrittenBytes(4);
    } else {
      output.WriteFixed32(EncodeFloat(value));
    }
  }

  static bool ReadFloat(IMessageIn& input, float& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadFixed32(tmp));
    out_value = DecodeFloat(tmp);
    return true;
  }

  static void WriteDouble(IMessageOut& output, const double value) {
    if (output.CountingOnly()) {
      output.AddWrittenBytes(8);
    } else {
      output.WriteFixed64(EncodeDouble(value));
    }
  }

  static bool ReadDouble(IMessageIn& input, double& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadFixed64(tmp));
    out_value = DecodeDouble(tmp);
    return true;
  }

  static void WriteUuid(IMessageOut& output, const Uuid& value) {
    uint8 buf[Uuid::BYTE_LENGTH];
    value.ToBytes(buf);
    output.WriteRawBytes(buf, Uuid::BYTE_LENGTH);
  }

  static bool ReadUuid(IMessageIn& input, Uuid& out_value) {
    uint8 buf[Uuid::BYTE_LENGTH];
    FUN_DO_CHECKED(input.ReadRawBytes(buf, Uuid::BYTE_LENGTH));
    out_value = Uuid::FromBytes(buf);
    return true;
  }

  //
  // Optimal integers
  //

  static void WriteOptimalInt32(IMessageOut& output, const int32 value) {
    const uint32 encoded_value = ZigZagEncode32(value);
    if (output.CountingOnly()) {
      output.AddWrittenBytes(GetByteLength_Varint32(encoded_value));
    } else {
      output.WriteVarint32(encoded_value);
    }
  }

  static bool ReadOptimalInt32(IMessageIn& input, int32& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadVarint32(tmp));
    out_value = ZigZagDecode32(tmp);
    return true;
  }

  static int32 GetByteLength_OptimalInt32(const int32 value) {
    const uint32 encoded_value = ZigZagEncode32(value);
    return GetByteLength_Varint32(encoded_value);
  }

  static void WriteOptimalInt64(IMessageOut& output, const int64 value) {
    const uint64 encoded_value = ZigZagEncode64(value);
    if (output.CountingOnly()) {
      output.AddWrittenBytes(GetByteLength_Varint64(encoded_value));
    } else {
      output.WriteVarint64(encoded_value);
    }
  }

  static bool ReadOptimalInt64(IMessageIn& input, int64& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadVarint64(tmp));
    out_value = ZigZagDecode64(tmp);
    return true;
  }

  static int32 GetByteLength_OptimalInt64(const int64 value) {
    const uint64 encoded_value = ZigZagEncode64(value);
    return GetByteLength_Varint64(encoded_value);
  }


  static void WriteCounter(IMessageOut& output, const int32 value) {
    WriteOptimalInt32(output, value);
  }

  static bool ReadCounter(IMessageIn& input, int32& out_value) {
    FUN_DO_CHECKED(ReadOptimalInt32(input, out_value));
    return true;
  }

  static int32 GetByteLength_Counter(const int32 value) {
    return GetByteLength_OptimalInt32(value);
  }


  static void WriteCounter64(IMessageOut& output, const int64 value) {
    WriteOptimalInt64(output, value);
  }

  static bool ReadCounter64(IMessageIn& input, int64& out_value) {
    FUN_DO_CHECKED(ReadOptimalInt64(input, out_value));
    return true;
  }

  static int32 GetByteLength_Counter64(const int64 value) {
    return GetByteLength_OptimalInt64(value);
  }


  static void WriteBytes(IMessageOut& output, const void* Data, int32 length) {
    WriteCounter(output, length);
    output.WriteRawBytes(Data, length);
  }

  static void WriteBytes(IMessageOut& output, const ByteArray& value) {
    WriteBytes(output, value.ConstData(), value.Len());
  }

  static bool ReadBytes(IMessageIn& input, ByteArray& out_value) {
    int32 length;
    FUN_DO_CHECKED(ReadCounter(input, length));

    //TODO Error handling....
    if (length < 0 || length > input.MessageMaxLength()) {
      //예외를 throw하는게 바람직할듯 싶은데??
      return false;
    }

    const int32 readable_length = input.ReadableLength();

    if (readable_length >= length) {
      out_value.ResizeUninitialized(length);
      UnsafeMemory::Memcpy(out_value.MutableData(), input.ReadablePtr(), length);
      input.SkipRead(length);
      return true;
    }
    return false;
  }


  static void WriteString(IMessageOut& output, const String& value) {
    const ByteArray Utf8 = value.ToUtf8();
    WriteBytes(output, Utf8);
  }

  static bool ReadString(IMessageIn& input, String& out_value) {
    out_value.Clear();

    int32 length;
    FUN_DO_CHECKED(ReadCounter(input, length));

    const int32 readable_length = input.ReadableLength();
    if (readable_length >= length) {
      out_value = String((const char*)input.ReadablePtr(), length); // assume UTF8 string
      input.SkipRead(length);
      return true;
    }

    return false;
  }

  static int32 GetByteLength_String(const String& value) {
    //TODO CString에 LengthAsUtf8() 함수를 하나 추가해주는게 바람직해보임.
    return GetByteLength_LengthPrefixed(value.ToUtf8().Len());
  }

  // Generated struct.

  FUN_ALWAYS_INLINE static void WriteStruct(IMessageOut& output, const GeneratedStruct& value) {
    const int32 struct_length = value.GetByteLength();
    WriteCounter(output, struct_length);
    value.Write(output);
  }

  template <typename GeneratedStructTy>
  FUN_ALWAYS_INLINE static bool ReadStruct(IMessageIn& input, GeneratedStructTy& out_value) {
    ScopedMessageInRecursionGuard recursion_guard(input);
    ScopedMessageInViewGuard view_guard(input);

    // Read byte-length of struct.
    int32 byte_length;
    FUN_DO_CHECKED(ReadCounter(input, byte_length));

    input.AdjustView(byte_length);
    const bool ok = out_value.Read(input);

    if (!input.AtEnd()) {
      throw MessageFormatException::MoreDataAvailable();
    }

    return ok;
  }
};



// note:
// enum은 type constraint을 별도로 주어야 컴파일러가 각 enum타입별로 다르게 취급함.
// https://stackoverflow.com/questions/12726954/how-can-i-partially-specialize-a-class-template-for-all-enums

template <typename T, typename = void>
struct MessageFieldTypeTraits {
  //@note THasMessageFieldTypeTraits에서 사용자 타입을 취급하는 방법이 정의 되어 있는지 여부를,
  //CppValueType이 void(기본형)인지 여부로 판단 하도록 되어 있으므로, 기본은 void로 해두어야함!

  //typedef InCppType CppValueType;
  typedef void CppValueType;

  static void Write(IMessageOut& output, const T& value) {
    //TODO error handling.
    fun_check(!"todo exception handling.");
  }

  static bool Read(IMessageIn& input, T& out_value) {
    //TODO error handling.
    fun_check(!"todo exception handling.");
    return false;
  }

  static int32 GetByteLength(const T& value) {
    //TODO error handling.
    fun_check(!"todo exception handling.");
    return -1;
  }

  static String ToString(const T& value) {
    return fun::ToString(value);
  }
};


template <typename T>
struct HasMessageFieldTypeTraits
  : BoolConstant<!IsSame<typename MessageFieldTypeTraits<T>::CppValueType, void>::value> {
};

} // namespace net
} // namespace fun
