//@todo length-delimited 계열은 전수 검사해야함.
//      GetByteLength도 전수 검사해야함.
//      모든 케이스 별로 검증을 해야할듯함!!

#pragma once

#include "fun/net/message/message.h"
#include "fun/net/message/message_format.h"

namespace fun {
namespace net {

class FUN_NET_API FlexFormat : public MessageFormat {
 public:
  //
  // Tagging
  //

  static const int32 TagTypeBits = 3; // 0~7
  static const int32 TagTypeMask = (1 << TagTypeBits) - 1;

  /** make a tag value with field-id and wire-type. */
  FUN_ALWAYS_INLINE static uint32 MakeTag(int32 field_id, WireType wire_type) {
    return static_cast<uint32>((field_id << TagTypeBits) | (uint32)wire_type);
  }

  /** extract wire-type from given tag. */
  FUN_ALWAYS_INLINE static WireType GetTagWireType(uint32 tag) {
    return static_cast<WireType>(tag & TagTypeMask);
  }

  /** extract field-id from given tag. */
  FUN_ALWAYS_INLINE static int32 GetTagFieldId(uint32 tag) {
    return static_cast<int32>(tag >> TagTypeBits);
  }

  /**
   * write a tag.  the Write*() functions typically include the tag, so
   * normally there's no need to call this unless using the Write*()
   * variants.
   */
  FUN_ALWAYS_INLINE static void WriteTag(IMessageOut& output, int32 field_id, WireType wire_type) {
    output.WriteVarint32(MakeTag(field_id, wire_type));
  }

  FUN_ALWAYS_INLINE static void WriteRawTag(IMessageOut& output, uint8 byte1, uint8 byte2, uint8 byte3, uint8 byte4, uint8 byte5) {
    output.WriteFixed8(byte1);
    output.WriteFixed8(byte2);
    output.WriteFixed8(byte3);
    output.WriteFixed8(byte4);
    output.WriteFixed8(byte5);
  }
  FUN_ALWAYS_INLINE static void WriteRawTag(IMessageOut& output, uint8 byte1, uint8 byte2, uint8 byte3, uint8 byte4) {
    output.WriteFixed8(byte1);
    output.WriteFixed8(byte2);
    output.WriteFixed8(byte3);
    output.WriteFixed8(byte4);
  }
  FUN_ALWAYS_INLINE static void WriteRawTag(IMessageOut& output, uint8 byte1, uint8 byte2, uint8 byte3) {
    output.WriteFixed8(byte1);
    output.WriteFixed8(byte2);
    output.WriteFixed8(byte3);
  }
  FUN_ALWAYS_INLINE static void WriteRawTag(IMessageOut& output, uint8 byte1, uint8 byte2) {
    output.WriteFixed8(byte1);
    output.WriteFixed8(byte2);
  }
  FUN_ALWAYS_INLINE static void WriteRawTag(IMessageOut& output, uint8 byte1) {
    output.WriteFixed8(byte1);
  }

  FUN_ALWAYS_INLINE static bool ReadTag(IMessageIn& input, uint32& out_tag) {
    out_tag = 0;

    const int32 readable_length = input.ReadableLength();
    const uint8* readable_data = (const uint8*)input.ReadablePtr();

    if (readable_length >= 1 && readable_data[0] < (1 << 7)) {
      out_tag = static_cast<uint64>(readable_data[0]);
      input.SkipRead(1);
      return true;
    }

    if (input.AtEnd()) {
      out_tag = 0;
      return true;
    }

    if (!input.ReadVarint32(out_tag)) {
      //TODO error handling.
      return false;
    }

    if (out_tag == 0) {
      //TODO error handling
      return false;
    }
    return true;
  }


  /** skip a field. */
  static bool SkipField(IMessageIn& input, uint32 tag);
  static bool SkipStruct(IMessageIn& input);


  //
  // Write fields, without tags.
  //

  FUN_ALWAYS_INLINE static void WriteInt8(IMessageOut& output, int8 value) {
    output.WriteVarint8SignExtended(value);
  }

  FUN_ALWAYS_INLINE static void WriteInt16(IMessageOut& output, int16 value) {
    output.WriteVarint16SignExtended(value);
  }

  FUN_ALWAYS_INLINE static void WriteInt32(IMessageOut& output, int32 value) {
    output.WriteVarint32SignExtended(value);
  }

  FUN_ALWAYS_INLINE static void WriteInt64(IMessageOut& output, int64 value) {
    output.WriteVarint64(static_cast<uint64>(value));
  }

  FUN_ALWAYS_INLINE static void WriteUInt8(IMessageOut& output, uint8 value) {
    output.WriteVarint8(value);
  }

  FUN_ALWAYS_INLINE static void WriteUInt16(IMessageOut& output, uint16 value) {
    output.WriteVarint16(value);
  }

  FUN_ALWAYS_INLINE static void WriteUInt32(IMessageOut& output, uint32 value) {
    output.WriteVarint32(value);
  }

  FUN_ALWAYS_INLINE static void WriteUInt64(IMessageOut& output, uint64 value) {
    output.WriteVarint64(value);
  }

  FUN_ALWAYS_INLINE static void WriteSInt8(IMessageOut& output, int8 value) {
    output.WriteVarint8(MessageFormat::ZigZagEncode8(value));
  }

  FUN_ALWAYS_INLINE static void WriteSInt16(IMessageOut& output, int16 value) {
    output.WriteVarint16(MessageFormat::ZigZagEncode16(value));
  }

  FUN_ALWAYS_INLINE static void WriteSInt32(IMessageOut& output, int32 value) {
    output.WriteVarint32(MessageFormat::ZigZagEncode32(value));
  }

  FUN_ALWAYS_INLINE static void WriteSInt64(IMessageOut& output, int64 value) {
    output.WriteVarint64(MessageFormat::ZigZagEncode64(value));
  }

  FUN_ALWAYS_INLINE static void WriteFixed8(IMessageOut& output, uint8 value) {
    output.WriteFixed8(value);
  }

  FUN_ALWAYS_INLINE static void WriteFixed16(IMessageOut& output, uint16 value) {
    output.WriteFixed16(value);
  }

  FUN_ALWAYS_INLINE static void WriteFixed32(IMessageOut& output, uint32 value) {
    output.WriteFixed32(value);
  }

  FUN_ALWAYS_INLINE static void WriteFixed64(IMessageOut& output, uint64 value) {
    output.WriteFixed64(value);
  }

  FUN_ALWAYS_INLINE static void WriteSFixed8(IMessageOut& output, int8 value) {
    output.WriteFixed8(static_cast<uint8>(value));
  }

  FUN_ALWAYS_INLINE static void WriteSFixed16(IMessageOut& output, int16 value) {
    output.WriteFixed16(static_cast<uint16>(value));
  }

  FUN_ALWAYS_INLINE static void WriteSFixed32(IMessageOut& output, int32 value) {
    output.WriteFixed32(static_cast<uint32>(value));
  }

  FUN_ALWAYS_INLINE static void WriteSFixed64(IMessageOut& output, int64 value) {
    output.WriteFixed64(static_cast<uint64>(value));
  }

  FUN_ALWAYS_INLINE static void WriteFloat(IMessageOut& output, float value) {
    MessageFormat::WriteFloat(output, value);
  }

  FUN_ALWAYS_INLINE static void WriteDouble(IMessageOut& output, double value) {
    MessageFormat::WriteDouble(output, value);
  }

  FUN_ALWAYS_INLINE static void WriteBool(IMessageOut& output, bool value) {
    MessageFormat::WriteBool(output, value);
  }

  template <typename EnumType>
  FUN_ALWAYS_INLINE static void WriteEnum(IMessageOut& output, const EnumType& value) {
    output.WriteVarint32SignExtended(static_cast<int32>(value));
  }

  /** @warning string is for utf-8 text only */
  FUN_ALWAYS_INLINE static void WriteString(IMessageOut& output, const String& value) {
    MessageFormat::WriteString(output, value);
  }

  //FUN_ALWAYS_INLINE static void WriteBytes(IMessageOut& output, const ByteArray& value)
  FUN_ALWAYS_INLINE static void WriteBytes(IMessageOut& output, const ByteArray& value) {
    MessageFormat::WriteBytes(output, value);
  }

  FUN_ALWAYS_INLINE static void WriteDateTime(IMessageOut& output, const DateTime& value) {
    output.WriteFixed64(static_cast<uint64>(value.ToUtcTicks()));
  }

  FUN_ALWAYS_INLINE static void WriteTimeSpan(IMessageOut& output, const Timespan& value) {
    output.WriteFixed64(value.ToTicks());
  }

  FUN_ALWAYS_INLINE static void WriteUuid(IMessageOut& output, const Uuid& value) {
    WriteCounter(output, 16); // length of guid(FlexFormat only!)
    MessageFormat::WriteUuid(output, value);
  }

  template <typename GeneratedStructTy>
  FUN_ALWAYS_INLINE static void WriteStruct(IMessageOut& output, const GeneratedStructTy& value) {
    MessageFormat::WriteStruct(output, value);
  }


  //
  // Reads
  //

  FUN_ALWAYS_INLINE static bool ReadInt8(IMessageIn& input, int8& out_value) {
    uint8 tmp;
    FUN_DO_CHECKED(input.ReadVarint8(tmp));
    out_value = static_cast<int8>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadInt16(IMessageIn& input, int16& out_value) {
    uint16 tmp;
    FUN_DO_CHECKED(input.ReadVarint16(tmp));
    out_value = static_cast<int16>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadInt32(IMessageIn& input, int32& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadVarint32(tmp));
    out_value = static_cast<int32>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadInt64(IMessageIn& input, int64& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadVarint64(tmp));
    out_value = static_cast<int64>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadUInt8(IMessageIn& input, uint8& out_value) {
    return input.ReadVarint8(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadUInt16(IMessageIn& input, uint16& out_value) {
    return input.ReadVarint16(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadUInt32(IMessageIn& input, uint32& out_value) {
    return input.ReadVarint32(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadUInt64(IMessageIn& input, uint64& out_value) {
    return input.ReadVarint64(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadFixed8(IMessageIn& input, uint8& out_value) {
    return input.ReadFixed8(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadFixed16(IMessageIn& input, uint16& out_value) {
    return input.ReadFixed16(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadFixed32(IMessageIn& input, uint32& out_value) {
    return input.ReadFixed32(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadFixed64(IMessageIn& input, uint64& out_value) {
    return input.ReadFixed64(out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadSInt8(IMessageIn& input, int8& out_value) {
    uint8 tmp;
    FUN_DO_CHECKED(input.ReadVarint8(tmp));
    out_value = MessageFormat::ZigZagDecode8(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadSInt16(IMessageIn& input, int16& out_value) {
    uint16 tmp;
    FUN_DO_CHECKED(input.ReadVarint16(tmp));
    out_value = MessageFormat::ZigZagDecode16(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadSInt32(IMessageIn& input, int32& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadVarint32(tmp));
    out_value = MessageFormat::ZigZagDecode32(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadSInt64(IMessageIn& input, int64& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadVarint64(tmp));
    out_value = MessageFormat::ZigZagDecode64(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadSFixed8(IMessageIn& input, int8& out_value) {
    uint8 tmp;
    FUN_DO_CHECKED(input.ReadFixed8(tmp));
    out_value = static_cast<int8>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadSFixed16(IMessageIn& input, int16& out_value) {
    uint16 tmp;
    FUN_DO_CHECKED(input.ReadFixed16(tmp));
    out_value = static_cast<int16>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadSFixed32(IMessageIn& input, int32& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadFixed32(tmp));
    out_value = static_cast<int32>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadSFixed64(IMessageIn& input, int64& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadFixed64(tmp));
    out_value = static_cast<int64>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadBool(IMessageIn& input, bool& out_value) {
    return MessageFormat::ReadBool(input, out_value);
  }

  template <typename CppEnumType>
  FUN_ALWAYS_INLINE static bool ReadEnum(IMessageIn& input, CppEnumType& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadVarint32(tmp));
    out_value = static_cast<CppEnumType>(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadFloat(IMessageIn& input, float& out_value) {
    return MessageFormat::ReadFloat(input, out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadDouble(IMessageIn& input, double& out_value) {
    return MessageFormat::ReadDouble(input, out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadString(IMessageIn& input, String& out_value) {
    return MessageFormat::ReadString(input, out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadBytes(IMessageIn& input, ByteArray& out_value) {
    return MessageFormat::ReadBytes(input, out_value);
  }

  FUN_ALWAYS_INLINE static bool ReadDateTime(IMessageIn& input, DateTime& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadFixed64(tmp));
    out_value = DateTime::FromUtcTicks(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadTimeSpan(IMessageIn& input, Timespan& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadFixed64(tmp));
    out_value = Timespan::FromTicks(tmp);
    return true;
  }

  FUN_ALWAYS_INLINE static bool ReadUuid(IMessageIn& input, Uuid& out_value) {
    // length of guid(FlexFormat only!)
    int32 length;
    FUN_DO_CHECKED(MessageFormat::ReadCounter(input, length));

    if (length != 16) {
      throw MessageFormatException(String::Format("malformed guid. ({0} != {1})", length, 16));
    }

    return MessageFormat::ReadUuid(input, out_value);
  }

  template <typename GeneratedStructTy>
  FUN_ALWAYS_INLINE static bool ReadStruct(IMessageIn& input, GeneratedStructTy& out_value) {
    return MessageFormat::ReadStruct(input, out_value);
  }

  //NOTE DEPRECATED MessageFieldTypeTraits<T>::Read로 처리함.
  //template <typename CppUserType>
  //FUN_ALWAYS_INLINE static bool ReadUserType(IMessageIn& input, CppUserType& out_value) {
  //  if (MessageFieldTypeTraits<CppUserType>::GetWireTypeForFlexFormatOnly() == WireType::LengthPrefixed) { //enable_if 로 분기를 해주는게 좋을듯??
  //    int32 byte_length;
  //    FUN_DO_CHECKED(input.ReadCounter(byte_length));
  //
  //    //@todo 여기서 읽을 수 있는 바이트 길이를 제한하는 것이 좋을듯?
  //  }
  //
  //  return MessageFieldTypeTraits<CppUserType>::Read(input, out_value);
  //}


  //
  // GetByteLength_XXX
  //

  FUN_ALWAYS_INLINE static int32 GetByteLength_Int8(const int8 value) {
    return MessageFormat::GetByteLength_Varint8SignExtended(value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Int16(const int16 value) {
    return MessageFormat::GetByteLength_Varint16SignExtended(value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Int32(const int32 value) {
    return MessageFormat::GetByteLength_Varint32SignExtended(value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Int64(const int64 value) {
    return MessageFormat::GetByteLength_Varint64((uint64)value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_UInt8(const uint8 value) {
    return MessageFormat::GetByteLength_Varint8(value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_UInt16(const uint16 value) {
    return MessageFormat::GetByteLength_Varint16(value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_UInt32(const uint32 value) {
    return MessageFormat::GetByteLength_Varint32(value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_UInt64(const uint64 value) {
    return MessageFormat::GetByteLength_Varint64(value);
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Fixed8(const uint8 value) {
    return 1;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Fixed16(const uint16 value) {
    return 2;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Fixed32(const uint32 value) {
    return 4;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Fixed64(const uint64 value) {
    return 8;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SInt8(const int8 value) {
    return MessageFormat::GetByteLength_Varint8(MessageFormat::ZigZagEncode8(value));
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SInt16(const int16 value) {
    return MessageFormat::GetByteLength_Varint16(MessageFormat::ZigZagEncode16(value));
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SInt32(const int32 value) {
    return MessageFormat::GetByteLength_Varint32(MessageFormat::ZigZagEncode32(value));
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SInt64(const int64 value) {
    return MessageFormat::GetByteLength_Varint64(MessageFormat::ZigZagEncode64(value));
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SFixed8(const int8 value) {
    return 1;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SFixed16(const int16 value) {
    return 2;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SFixed32(const int32 value) {
    return 4;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_SFixed64(const int64 value) {
    return 8;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Bool(const bool value) {
    return 1;
  }

  template <typename CppEnumType>
  FUN_ALWAYS_INLINE static int32 GetByteLength_Enum(const CppEnumType value) {
    return MessageFormat::GetByteLength_Varint32SignExtended(static_cast<int32>(value));
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Float(const float value) {
    return 4;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Double(const double value) {
    return 8;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_String(const String value) {
    return MessageFormat::GetByteLength_String(value);
  }

  //FUN_ALWAYS_INLINE static int32 GetByteLength_Bytes(const ByteArray& value)
  FUN_ALWAYS_INLINE static int32 GetByteLength_Bytes(const ByteArray& value) {
    return MessageFormat::GetByteLength_LengthPrefixed(value.Len());
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_DateTime(const DateTime& value) {
    return 8;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_TimeSpan(const Timespan& value) {
    return 8;
  }

  FUN_ALWAYS_INLINE static int32 GetByteLength_Uuid(const Uuid& value) {
    return 1 + 16; // = MessageFormat::GetByteLength_LengthPrefixed(16);
  }

  template <typename GeneratedStructTy>
  FUN_ALWAYS_INLINE static int32 GetByteLength_Struct(const GeneratedStructTy& value) {
    return MessageFormat::GetByteLength_LengthPrefixed(value.GetByteLength());
  }

  //NOTE DEPRECATED MessageFieldTypeTraits<T>::GetByteLength로 처리함.
  //template <typename CppUserType>
  //FUN_ALWAYS_INLINE static int32 GetByteLength_UserType(const CppUserType& value) {
  //  int32 byte_length = MessageFieldTypeTraits<CppUserType>::GetByteLength(value);
    //
  //  if (MessageFieldTypeTraits<CppUserType>::GetWireTypeForFlexFormatOnly() == WireType::LengthPrefixed) {
  //    byte_length = MessageFormat::GetByteLength_LengthPrefixed(byte_length);
  //  }
    //
  //  return byte_length;
  //}
};

} // namespace net
} // namespace fun
