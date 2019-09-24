#pragma once

#include "fun/base/ftl/type_traits.h"
#include "fun/net/message/message.h"

namespace fun {
namespace net {

template <typename T>
class OptimalCounter {
 public:
  FUN_ALWAYS_INLINE OptimalCounter() : value_(0) {}

  FUN_ALWAYS_INLINE OptimalCounter(const T value) : value_(value) {}

  FUN_ALWAYS_INLINE OptimalCounter(const OptimalCounter& rhs)
      : value_(rhs.value_) {}

  FUN_ALWAYS_INLINE OptimalCounter& operator=(const OptimalCounter& rhs) {
    value_ = rhs.value_;
    return *this;
  }

  FUN_ALWAYS_INLINE OptimalCounter& operator=(const T value) {
    value_ = value;
    return *this;
  }

  FUN_ALWAYS_INLINE operator T() const { return value_; }

  FUN_ALWAYS_INLINE friend bool operator==(const OptimalCounter& a,
                                           const OptimalCounter& b) {
    return a.value_ == b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator!=(const OptimalCounter& a,
                                           const OptimalCounter& b) {
    return a.value_ != b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator<(const OptimalCounter& a,
                                          const OptimalCounter& b) {
    return a.value_ < b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator<=(const OptimalCounter& a,
                                           const OptimalCounter& b) {
    return a.value_ <= b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator>(const OptimalCounter& a,
                                          const OptimalCounter& b) {
    return a.value_ > b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator>=(const OptimalCounter& a,
                                           const OptimalCounter& b) {
    return a.value_ >= b.value_;
  }

  FUN_ALWAYS_INLINE friend bool operator==(const OptimalCounter& a, const T b) {
    return a.value_ == b;
  }
  FUN_ALWAYS_INLINE friend bool operator!=(const OptimalCounter& a, const T b) {
    return a.value_ != b;
  }
  FUN_ALWAYS_INLINE friend bool operator<(const OptimalCounter& a, const T b) {
    return a.value_ < b;
  }
  FUN_ALWAYS_INLINE friend bool operator<=(const OptimalCounter& a, const T b) {
    return a.value_ <= b;
  }
  FUN_ALWAYS_INLINE friend bool operator>(const OptimalCounter& a, const T b) {
    return a.value_ > b;
  }
  FUN_ALWAYS_INLINE friend bool operator>=(const OptimalCounter& a, const T b) {
    return a.value_ >= b;
  }

  FUN_ALWAYS_INLINE friend bool operator==(const T a, const OptimalCounter& b) {
    return a == b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator!=(const T a, const OptimalCounter& b) {
    return a != b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator<(const T a, const OptimalCounter& b) {
    return a < b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator<=(const T a, const OptimalCounter& b) {
    return a <= b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator>(const T a, const OptimalCounter& b) {
    return a > b.value_;
  }
  FUN_ALWAYS_INLINE friend bool operator>=(const T a, const OptimalCounter& b) {
    return a >= b.value_;
  }

  FUN_ALWAYS_INLINE friend uint32 HashOf(const OptimalCounter& v) {
    return HashOf(v.value_);
  }

  FUN_ALWAYS_INLINE friend Archive& operator&(Archive& ar, OptimalCounter& v) {
    return ar & v.value_;
  }

  // template <typename T>
  // FUN_ALWAYS_INLINE friend TextStream& operator << (TextStream& stream, const
  // OptimalCounter<T>& value)
  //{
  //  return stream << value.value_;
  //}

 private:
  T value_;
};

typedef OptimalCounter<int32> OptimalCounter32;
typedef OptimalCounter<int64> OptimalCounter64;

/**
specialized user-type인지 아닌지로 우선 구분하고,
     - specialized user-type일 경우 처리
     - enum or general일 경우를 나누어서 처리
          - enum일 경우
          - known일 경우(int, float, bool, ...)
*/
class FUN_NET_API LiteFormat : public MessageFormat {
 public:
  //
  // write
  //

  /**
  여러개의 값을 메시지 스트림에 기록합니다.
  */
  // template <typename... Types>
  // FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Types&...
  // args) {
  //  Write(output, args...);
  //}

  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7, typename Type8,
            typename Type9, typename Type10>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2, const Type3& value3,
                                       const Type4& value4, const Type5& value5,
                                       const Type6& value6, const Type7& value7,
                                       const Type8& value8, const Type9& value9,
                                       const Type10& value10) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
    Write(output, value4);
    Write(output, value5);
    Write(output, value6);
    Write(output, value7);
    Write(output, value8);
    Write(output, value9);
    Write(output, value10);
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7, typename Type8,
            typename Type9>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2, const Type3& value3,
                                       const Type4& value4, const Type5& value5,
                                       const Type6& value6, const Type7& value7,
                                       const Type8& value8,
                                       const Type9& value9) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
    Write(output, value4);
    Write(output, value5);
    Write(output, value6);
    Write(output, value7);
    Write(output, value8);
    Write(output, value9);
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7, typename Type8>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2, const Type3& value3,
                                       const Type4& value4, const Type5& value5,
                                       const Type6& value6, const Type7& value7,
                                       const Type8& value8) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
    Write(output, value4);
    Write(output, value5);
    Write(output, value6);
    Write(output, value7);
    Write(output, value8);
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2, const Type3& value3,
                                       const Type4& value4, const Type5& value5,
                                       const Type6& value6,
                                       const Type7& value7) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
    Write(output, value4);
    Write(output, value5);
    Write(output, value6);
    Write(output, value7);
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2, const Type3& value3,
                                       const Type4& value4, const Type5& value5,
                                       const Type6& value6) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
    Write(output, value4);
    Write(output, value5);
    Write(output, value6);
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2, const Type3& value3,
                                       const Type4& value4,
                                       const Type5& value5) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
    Write(output, value4);
    Write(output, value5);
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2, const Type3& value3,
                                       const Type4& value4) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
    Write(output, value4);
  }
  template <typename Type1, typename Type2, typename Type3>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2,
                                       const Type3& value3) {
    Write(output, value1);
    Write(output, value2);
    Write(output, value3);
  }
  template <typename Type1, typename Type2>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output, const Type1& value1,
                                       const Type2& value2) {
    Write(output, value1);
    Write(output, value2);
  }
  template <typename Type1>
  FUN_ALWAYS_INLINE static void Writes(IMessageOut& output,
                                       const Type1& value1) {
    Write(output, value1);
  }

  /**
  특수화된 사용자 정의 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename T>
  FUN_ALWAYS_INLINE static
      typename EnableIf<HasMessageFieldTypeTraits<T>::Value, void>::Type
      Write(IMessageOut& output, const T& value) {
    WriteUserType(output, value);
  }

  /**
  특수화되지 않은 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename T>
  FUN_ALWAYS_INLINE static
      typename EnableIf<!HasMessageFieldTypeTraits<T>::Value, void>::Type
      Write(IMessageOut& output, const T& value) {
    WriteEnumOrWellKnownType(output, value);
  }

  /**
  특수화된 유저타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename T>
  FUN_ALWAYS_INLINE static void WriteUserType(IMessageOut& output,
                                              const T& value) {
    MessageFieldTypeTraits<T>::Write(output, value);
  }

  /**
  Native enum 값을 메시지 스트림에 기록합니다.
  */
  template <typename T>
  FUN_ALWAYS_INLINE static typename EnableIf<IsEnum<T>::Value, void>::Type
  WriteEnumOrWellKnownType(IMessageOut& output, const T& value) {
    WriteEnumType(output, value);
  }

  /**
  enum이 아닌 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename T>
  FUN_ALWAYS_INLINE static typename EnableIf<!IsEnum<T>::Value, void>::Type
  WriteEnumOrWellKnownType(IMessageOut& output, const T& value) {
    WriteWellKnownType(output, value);
  }

  /**
  특수화된 사용자 정의 타입, enum 그리고 LiteFormat내에서 다루는 타입이 아닌
  경우에는 assertion을 발생합니다.

  해당 타입에 대한 특수화를 해주어야 정상적으로 메시지 스트림에 읽고, 쓰기가
  가능해집니다.
  */
  template <typename T>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const T& value) {
    static_assert(0, "unhandled type");
  }

  /**
  Array<T,Allocator> 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename ElementType, typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output, const Array<ElementType, Allocator>& value) {
    // Counter
    WriteCounter(output, value.Count());

    // Elements
    for (const auto& elem : value) {
      Write(output, elem);
    }
  }

  /**
  Array<uint8,Allocator> 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output, const Array<uint8, Allocator>& value) {
    MessageFormat::WriteBytes(output, value.ConstData(), value.Count());
  }

  /**
  Array<int8,Allocator> 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output, const Array<int8, Allocator>& value) {
    MessageFormat::WriteBytes(output, value.ConstData(), value.Count());
  }

  /**
  Map<Key,value> 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename KeyType, typename ValueType, typename SetAllocator,
            typename KeyFuncs>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output,
      const Map<KeyType, valueType, SetAllocator, KeyFuncs>& value) {
    // Counter
    WriteCounter(output, value.Count());

    // Pairs
    for (const auto& pair : value) {
      Write(output, pair.key);
      Write(output, pair.value);
    }
  }

  /**
  Set<ElementType,KeyFuncs,Allocator> 타입의 값을 메시지 스트림에 기록합니다.
  */
  template <typename ElementType, typename KeyFuncs, typename Allocator>
  void WriteWellKnownType(IMessageOut& output,
                          const Set<ElementType, KeyFuncs, Allocator>& value) {
    // Counter
    WriteCounter(output, value.Count());

    // Elements
    for (const auto& elem : value) {
      Write(output, elem);
    }
  }

#if FUN_MESSAGEFORMAT_SUPPORT_STL
  template <typename ValueType, typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output, const std::vector<ValueType, Allocator>& value) {
    // Counter
    WriteCounter(output, static_cast<int32>(value.size()));

    // Elements
    for (const auto& elem : value) {
      Write(output, elem);
    }
  }

  template <typename KeyType, typename Compare, typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output, const std::set<KeyType, Compare, Allocator>& value) {
    // Counter
    WriteCounter(output, static_cast<int32>(value.size()));

    // Elements
    for (const auto& elem : value) {
      Write(output, elem);
    }
  }

  template <typename KeyType, typename MappedType, typename Compare,
            typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output,
      const std::map<KeyType, MappedType, Compare, Allocator>& value) {
    // Counter
    WriteCounter(output, static_cast<int32>(value.size()));

    // Pairs
    for (const auto& pair : value) {
      Write(output, pair.first);
      Write(output, pair.second);
    }
  }
#endif  // FUN_MESSAGEFORMAT_SUPPORT_STL

  template <typename T>
  FUN_ALWAYS_INLINE static void WriteEnumType(IMessageOut& output,
                                              const T& value) {
    // 설령 enum이라고 해도, 유저가 특수하게 시리얼라이징 하려고 하면 그걸
    // 우선적으로 처리함.
    output.WriteVarint32SignExtended(static_cast<int32>(value));
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output, const OptimalCounter32& value) {
    MessageFormat::WriteCounter(output, value.value);
  }
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output, const OptimalCounter64& value) {
    MessageFormat::WriteCounter64(output, value.value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const int8 value) {
    output.WriteFixed8((uint8)value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const int16 value) {
    output.WriteFixed16((uint16)value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const int32 value) {
    output.WriteFixed32((uint32)value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const int64 value) {
    output.WriteFixed64((uint64)value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const uint8 value) {
    output.WriteFixed8(value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const uint16 value) {
    output.WriteFixed16(value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const uint32 value) {
    output.WriteFixed32(value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const uint64 value) {
    output.WriteFixed64(value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const bool value) {
    output.WriteFixed8(value ? 1 : 0);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const float value) {
    output.WriteFixed32(MessageFormat::EncodeFloat(value));
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const double value) {
    output.WriteFixed64(MessageFormat::EncodeDouble(value));
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const DateTime& value) {
    output.WriteFixed64(static_cast<uint64>(value.ToUtcTicks()));
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const Timespan& value) {
    output.WriteFixed64(static_cast<uint64>(value.ToTicks()));
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const Uuid& value) {
    MessageFormat::WriteGuid(output, value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const ByteArray& value) {
    MessageFormat::WriteBytes(output, value);
  }

  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const String& value) {
    MessageFormat::WriteString(output, value);
  }

  // const char*
  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const char* value) {
    const int32 utf8_len = static_cast<int32>(strlen(value));
    MessageFormat::WriteCounter(output, utf8_len);
    output.WriteRawBytes(value, utf8_len);
  }

  // const wchar_t*
  FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output,
                                                   const wchar_t* value) {
    // TODO
    fun_check(0);

    // convert from utf8 to ucs2
    // const int32 Ucs2Len = static_cast<int32>(wcslen(value));
    // const int32 utf8_len = CUtfConv::GetLength_StringAsUtf8(value, Ucs2Len);
    //
    // TScopedWorkingBuffer<1024, uint8> Utf8Buf(utf8_len);
    // CUtfConv::Ucs2ToUtf8(value, Ucs2Len, Utf8Buf.GetBuffer(), utf8_len);
    //
    // MessageFormat::WriteCounter(output, utf8_len);
    // output.WriteRawBytes(Utf8Buf.GetBuffer(), utf8_len);

    // TODO wchar_t -> UTF8
  }

  // FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output, const
  // IpAddress& value)
  //{
  //  uint8 Buffer[IpAddress::IPv6AddressByteLength];
  //  const int32 Length = value.GetAddressBytes(Buffer, sizeof(Buffer));
  //  output.WriteFixed8((uint8)Length);
  //  output.WriteRawBytes(Buffer, Length);
  //}
  //
  // FUN_ALWAYS_INLINE static void WriteWellKnownType(IMessageOut& output, const
  // InetAddress& value)
  //{
  //  Write(output, value.GetHost());
  //  Write(output, value.GetPort());
  //}

#if FUN_MESSAGEFORMAT_SUPPORT_STL
  // std::basic_string<char>
  template <typename Traits, typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output,
      const std::basic_string<char, Traits, Allocator>& value) {
    const int32 utf8_len = static_cast<int32>(value.size());
    MessageFormat::WriteBytes(output, value.c_str(), utf8_len);
  }

  // std::basic_string<wchar_t>
  template <typename Traits, typename Allocator>
  FUN_ALWAYS_INLINE static void WriteWellKnownType(
      IMessageOut& output,
      const std::basic_string<wchar_t, Traits, Allocator>& value) {
    // TODO
    fun_check(0);

    ////@todo wchar_t가 4바이트 짜리라면... 대략 난감.. 이걸 대비해서 처리해
    ///줘야함.
    // const int32 Ucs2Len = static_cast<int32>(value.size());
    // const int32 utf8_len = CUtfConv::GetLength_StringAsUtf8(value.c_str(),
    // Ucs2Len);
    //
    // TScopedWorkingBuffer<1024, uint8> Utf8Buf(utf8_len);
    // CUtfConv::Ucs2ToUtf8(value.c_str(), Ucs2Len, Utf8Buf.GetBuffer(),
    // utf8_len);
    //
    // MessageFormat::WriteBytes(output, Utf8Buf.GetBuffer(), utf8_len);

    // TODO wchar_t -> UTF8
  }
#endif  // FUN_MESSAGEFORMAT_SUPPORT_STL

  //
  // read
  //

  // template <typename... Types>
  // FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Types&... args)
  //{
  //  FUN_DO_CHECKED(Read(input, args...));
  //  return true;
  //}
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7, typename Type8,
            typename Type9, typename Type10>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3,
                                      Type4& out_value4, Type5& out_value5,
                                      Type6& out_value6, Type7& out_value7,
                                      Type8& out_value8, Type9& out_value9,
                                      Type10& out_value10) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    FUN_DO_CHECKED(Read(input, out_value4));
    FUN_DO_CHECKED(Read(input, out_value5));
    FUN_DO_CHECKED(Read(input, out_value6));
    FUN_DO_CHECKED(Read(input, out_value7));
    FUN_DO_CHECKED(Read(input, out_value8));
    FUN_DO_CHECKED(Read(input, out_value9));
    FUN_DO_CHECKED(Read(input, out_value10));
    return true;
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7, typename Type8,
            typename Type9>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3,
                                      Type4& out_value4, Type5& out_value5,
                                      Type6& out_value6, Type7& out_value7,
                                      Type8& out_value8, Type9& out_value9) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    FUN_DO_CHECKED(Read(input, out_value4));
    FUN_DO_CHECKED(Read(input, out_value5));
    FUN_DO_CHECKED(Read(input, out_value6));
    FUN_DO_CHECKED(Read(input, out_value7));
    FUN_DO_CHECKED(Read(input, out_value8));
    FUN_DO_CHECKED(Read(input, out_value9));
    return true;
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7, typename Type8>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3,
                                      Type4& out_value4, Type5& out_value5,
                                      Type6& out_value6, Type7& out_value7,
                                      Type8& out_value8) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    FUN_DO_CHECKED(Read(input, out_value4));
    FUN_DO_CHECKED(Read(input, out_value5));
    FUN_DO_CHECKED(Read(input, out_value6));
    FUN_DO_CHECKED(Read(input, out_value7));
    FUN_DO_CHECKED(Read(input, out_value8));
    return true;
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6, typename Type7>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3,
                                      Type4& out_value4, Type5& out_value5,
                                      Type6& out_value6, Type7& out_value7) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    FUN_DO_CHECKED(Read(input, out_value4));
    FUN_DO_CHECKED(Read(input, out_value5));
    FUN_DO_CHECKED(Read(input, out_value6));
    FUN_DO_CHECKED(Read(input, out_value7));
    return true;
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5, typename Type6>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3,
                                      Type4& out_value4, Type5& out_value5,
                                      Type6& out_value6) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    FUN_DO_CHECKED(Read(input, out_value4));
    FUN_DO_CHECKED(Read(input, out_value5));
    FUN_DO_CHECKED(Read(input, out_value6));
    return true;
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4,
            typename Type5>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3,
                                      Type4& out_value4, Type5& out_value5) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    FUN_DO_CHECKED(Read(input, out_value4));
    FUN_DO_CHECKED(Read(input, out_value5));
    return true;
  }
  template <typename Type1, typename Type2, typename Type3, typename Type4>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3,
                                      Type4& out_value4) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    FUN_DO_CHECKED(Read(input, out_value4));
    return true;
  }
  template <typename Type1, typename Type2, typename Type3>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2, Type3& out_value3) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    FUN_DO_CHECKED(Read(input, out_value3));
    return true;
  }
  template <typename Type1, typename Type2>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1,
                                      Type2& out_value2) {
    FUN_DO_CHECKED(Read(input, out_value1));
    FUN_DO_CHECKED(Read(input, out_value2));
    return true;
  }
  template <typename Type1>
  FUN_ALWAYS_INLINE static bool Reads(IMessageIn& input, Type1& out_value1) {
    FUN_DO_CHECKED(Read(input, out_value1));
    return true;
  }

  template <typename T>
  FUN_ALWAYS_INLINE static
      typename EnableIf<HasMessageFieldTypeTraits<T>::Value, bool>::Type
      Read(IMessageIn& input, T& out_value) {
    return ReadUserType(input, out_value);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static
      typename EnableIf<!HasMessageFieldTypeTraits<T>::Value, bool>::Type
      Read(IMessageIn& input, T& out_value) {
    return ReadEnumOrWellKnownType(input, out_value);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static bool ReadUserType(IMessageIn& input, T& out_value) {
    return MessageFieldTypeTraits<T>::Read(input, out_value);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static typename EnableIf<IsEnum<T>::Value, bool>::Type
  ReadEnumOrWellKnownType(IMessageIn& input, T& out_value) {
    return ReadEnumType(input, out_value);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static typename EnableIf<!IsEnum<T>::Value, bool>::Type
  ReadEnumOrWellKnownType(IMessageIn& input, T& out_value) {
    return ReadWellKnownType(input, out_value);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  T& out_value) {
    static_assert(0, "unhandled type");
    return false;
  }

  // Array
  template <typename ElementType, typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input, Array<ElementType, Allocator>& out_value) {
    ScopedMessageInRecursionGuard recursion_guard(input);

    // Counter
    int32 count = 0;
    FUN_DO_CHECKED(MessageFormat::ReadCounter(input, count));

    out_value.Clear(count);  // just in case

    // Elements
    for (int32 element_index = 0; element_index < count; ++element_index) {
      ElementType elem;
      FUN_DO_CHECKED(Read(input, elem));
      out_value.Add(elem);
    }

    return true;
  }

  //@todo 이러한 특수화는 할지 여부를 진중하게 검토해야할듯 함..
  //@warning 엘리먼트 하나씩 시리얼라이징 했을때와, 다른 결과를 갖게 되므로..
  //이부분에 대해서 생각을 좀.. Array<uint8> 이놈은 특수하게 다루는게 좋을듯..
  template <typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input, Array<uint8, Allocator>& out_value) {
    // Counter
    int32 count;
    FUN_DO_CHECKED(MessageFormat::ReadCounter(input, count));

    out_value.ResizeUninitialized(count);  // just in case

    // Optimize: treat as bulk.
    FUN_DO_CHECKED(input.ReadRawBytes(out_value.MutableData(), count));
    return true;
  }

  // Array<int8> 이놈은 특수하게 다루는게 좋을듯..
  template <typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input, Array<int8, Allocator>& out_value) {
    // Counter
    OptimalCounter32 count;
    FUN_DO_CHECKED(Read(input, count));

    // TODO range check

    out_value.ResizeUninitialized(count);  // just in case

    // Optimize: treat as bulk.
    FUN_DO_CHECKED(input.ReadRawBytes(out_value.MutableData(), count));
    return true;
  }

  // Map
  template <typename KeyType, typename ValueType, typename SetAllocator,
            typename KeyFuncs>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input,
      Map<KeyType, valueType, SetAllocator, KeyFuncs>& out_value) {
    ScopedMessageInRecursionGuard recursion_guard(input);

    // Counter
    OptimalCounter32 count;
    FUN_DO_CHECKED(Read(input, count));

    // TODO range check

    out_value.Clear(count);  // just in case

    // Pairs
    for (int32 pair_index = 0; pair_index < count; ++pair_index) {
      KeyType key;
      ValueType value;
      FUN_DO_CHECKED(Read(input, key));
      FUN_DO_CHECKED(Read(input, value));
      out_value.Add(key, value);
    }

    return true;
  }

  // Set
  template <typename ElementType, typename KeyFuncs, typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input, Set<ElementType, KeyFuncs, Allocator>& out_value) {
    ScopedMessageInRecursionGuard recursion_guard(input);

    // Counter
    OptimalCounter32 count;
    FUN_DO_CHECKED(Read(input, count));

    // TODO range check

    out_value.Clear(count);  // just in case

    // Elements
    for (int32 element_index = 0; element_index < count; ++element_index) {
      ElementType elem;
      FUN_DO_CHECKED(Read(input, elem));
      out_value.Add(elem);
    }

    return true;
  }

#if FUN_MESSAGEFORMAT_SUPPORT_STL
  // std::vector
  template <typename ValueType, typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input, std::vector<ValueType, Allocator>& out_value) {
    ScopedMessageInRecursionGuard recursion_guard(input);

    // Counter
    OptimalCounter32 count;
    FUN_DO_CHECKED(Read(input, count));

    // TODO range check

    out_value.clear();  // just in case
    out_value.reserve(count);

    // Elements
    for (int32 element_index = 0; element_index < count; ++element_index) {
      ValueType elem;
      FUN_DO_CHECKED(Read(input, elem));
      out_value.push_back(elem);
    }

    return true;
  }

  // std::set
  template <typename KeyType, typename Compare, typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input, std::set<KeyType, Compare, Allocator>& out_value) {
    ScopedMessageInRecursionGuard recursion_guard(input);

    // Counter
    OptimalCounter32 count;
    FUN_DO_CHECKED(Read(input, count));

    // TODO range check

    out_value.clear();  // just in case

    // Elements
    for (int32 element_index = 0; element_index < count; ++element_index) {
      KeyType key;
      FUN_DO_CHECKED(Read(input, key));
      out_value.insert(key);
    }

    return true;
  }

  // std::map
  template <typename KeyType, typename MappedType, typename Compare,
            typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input,
      std::map<KeyType, MappedType, Compare, Allocator>& out_value) {
    IMessageIn::recursion_guard recursion_guard(input);

    // Counter
    OptimalCounter32 count;
    FUN_DO_CHECKED(Read(input, count));

    // TODO range check

    out_value.clear();  // just in case

    // Pairs
    for (int32 element_index = 0; element_index < count; ++element_index) {
      KeyType key;
      MappedType value;
      FUN_DO_CHECKED(Read(input, key));
      FUN_DO_CHECKED(Read(input, value));
      out_value.insert(std::make_pair(key, value));
    }

    return true;
  }
#endif  // FUN_MESSAGEFORMAT_SUPPORT_STL

  /**
  enum 값을 메시지 스트림에서 읽어옵니다.
  */
  template <typename T>
  FUN_ALWAYS_INLINE static bool ReadEnumType(IMessageIn& input, T& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadVarint32(tmp));
    out_value = static_cast<T>(tmp);
    return true;
  }

  /**
  최적화된 32비트 카운터를 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  OptimalCounter32& out_value) {
    return MessageFormat::ReadCounter(input, out_value.value);
  }

  /**
  최적화된 64비트 카운터를 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  OptimalCounter64& out_value) {
    return MessageFormat::ReadCounter64(input, out_value.value);
  }

  /**
  int8 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  int8& out_value) {
    uint8 tmp;
    FUN_DO_CHECKED(input.ReadFixed8(tmp));
    out_value = (int8)tmp;
    return true;
  }

  /**
  int16 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  int16& out_value) {
    uint16 tmp;
    FUN_DO_CHECKED(input.ReadFixed16(tmp));
    out_value = (int16)tmp;
    return true;
  }

  /**
  int32 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  int32& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadFixed32(tmp));
    out_value = (int32)tmp;
    return true;
  }

  /**
  int64 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  int64& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadFixed64(tmp));
    out_value = (int64)tmp;
    return true;
  }

  /**
  uint8 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  uint8& out_value) {
    return input.ReadFixed8(out_value);
  }

  /**
  uint16 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  uint16& out_value) {
    return input.ReadFixed16(out_value);
  }

  /**
  uint32 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  uint32& out_value) {
    return input.ReadFixed32(out_value);
  }

  /**
  uint64 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  uint64& out_value) {
    return input.ReadFixed64(out_value);
  }

  /**
  bool 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  bool& out_value) {
    uint8 tmp;
    FUN_DO_CHECKED(input.ReadFixed8(tmp));
    out_value = (tmp != 0);
    return true;
  }

  /**
  float 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  float& out_value) {
    uint32 tmp;
    FUN_DO_CHECKED(input.ReadFixed32(tmp));
    out_value = MessageFormat::DecodeFloat(tmp);
    return true;
  }

  /**
  double 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  double& out_value) {
    uint64 tmp;
    FUN_DO_CHECKED(input.ReadFixed64(tmp));
    out_value = MessageFormat::DecodeDouble(tmp);
    return true;
  }

  /**
  DateTime 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  DateTime& out_value) {
    uint64 ticks;
    FUN_DO_CHECKED(input.ReadFixed64(ticks));
    out_value = DateTime::FromUtcTicks(ticks);
    return true;
  }

  /**
  Timespan 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  Timespan& out_value) {
    uint64 ticks;
    FUN_DO_CHECKED(input.ReadFixed64(ticks));
    out_value = Timespan((int64)ticks);
    return true;
  }

  /**
  Uuid 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  Uuid& out_value) {
    return MessageFormat::ReadGuid(input, out_value);
  }

  /**
  ByteArray 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  ByteArray& out_value) {
    // TODO range check
    return MessageFormat::ReadBytes(input, out_value);
  }

  /**
  String 타입의 값을 메시지 스트림에서 읽어옵니다.
  */
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
                                                  String& out_value) {
    // TODO range check
    return MessageFormat::ReadString(input, out_value);
  }

  ///**
  // IpAddress 타입의 값을 메시지 스트림에서 읽어옵니다.
  //*/
  // FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
  // IpAddress& out_value) {
  //  uint8 Length;
  //  FUN_DO_CHECKED(input.ReadFixed8(Length));
  //  if (Length != IpAddress::IPv4AddressByteLength || Length !=
  //  IpAddress::IPv6AddressByteLength) {
  //    return false;
  //  }
  //
  //  uint8 Buffer[IpAddress::IPv6AddressByteLength];
  //  FUN_DO_CHECKED(input.ReadRawBytes(Buffer, Length));
  //
  //  out_value = IpAddress(Buffer, Length, 0);
  //  return true;
  //}
  //
  ///**
  // InetAddress 타입의 값을 메시지 스트림에서 읽어옵니다.
  //*/
  // FUN_ALWAYS_INLINE static bool ReadWellKnownType(IMessageIn& input,
  // InetAddress& out_value) {
  //  IpAddress Host;
  //  FUN_DO_CHECKED(Read(input, Host));
  //
  //  uint16 Port;
  //  FUN_DO_CHECKED(Read(input, Port));
  //
  //  out_value = InetAddress(Host, Port);
  //  return true;
  //}

#if FUN_MESSAGEFORMAT_SUPPORT_STL
  /**
  std::basic_string<char>
  */
  template <typename Traits, typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input,
      std::basic_string<char, Traits, Allocator>& out_value) {
    // Counter
    int32 utf8_len;
    FUN_DO_CHECKED(MessageFormat::ReadCounter(input, utf8_len));

    // TODO range check

    out_value.resize(utf8_len);

    // Optimize: treat as bulk.
    return input.ReadRawBytes(&out_value[0], utf8_len);
  }

  /**
  std::basic_string<wchar_t>
  */
  template <typename Traits, typename Allocator>
  FUN_ALWAYS_INLINE static bool ReadWellKnownType(
      IMessageIn& input,
      std::basic_string<wchar_t, Traits, Allocator>& out_value) {
    //// Counter
    // int32 utf8_len;
    // FUN_DO_CHECKED(MessageFormat::ReadCounter(input, utf8_len));
    //
    // TScopedWorkingBuffer<1024, char> Utf8Buf(utf8_len);
    // FUN_DO_CHECKED(input.ReadRawBytes(Utf8Buf.GetBuffer(), utf8_len));
    //
    // const int32 Ucs2Len = CUtfConv::GetLength_Utf8ToUcs2(Utf8Buf.GetBuffer(),
    // utf8_len); out_value.resize(Ucs2Len); //nul을 포함하는건지 어쩐지?
    // CUtfConv::Utf8ToUcs2(reinterpret_cast<const uint8*>(Utf8Buf.GetBuffer()),
    // utf8_len, &out_value[0], Ucs2Len);
    // TODO
    fun_check(0);

    // TODO
    // UTF8 -> wchar_t

    return true;
  }
#endif  // FUN_MESSAGEFORMAT_SUPPORT_STL

  //
  // GetByteLength
  //
  // public:
  //  template <typename T> FUN_ALWAYS_INLINE static
  //  typename EnableIf<HasMessageFieldTypeTraits<T>::Value, int32>::Type
  //    GetByteLength(const T& value) {
  //    return GetByteLengthUserType(value);
  //  }
  //
  //  template <typename T> FUN_ALWAYS_INLINE static
  //  typename EnableIf<!HasMessageFieldTypeTraits<T>::Value, int32>::Type
  //    GetByteLength(const T& value) {
  //    return GetByteLengthEnumOrWellKnownType(value);
  //  }
  //
  //  template <typename T> FUN_ALWAYS_INLINE static
  //  typename EnableIf<IsEnum<T>::Value, int32>::Type
  //    GetByteLengthEnumOrWellKnownType(const T& value) {
  //    return GetByteLengthEnumType(value);
  //  }
  //
  //  template <typename T> FUN_ALWAYS_INLINE static
  //  typename EnableIf<!IsEnum<T>::Value, int32>::Type
  //    GetByteLengthEnumOrWellKnownType(const T& value) {
  //    return GetByteLengthWellKnownType(value);
  //  }
  //
  //
  //  template <typename T> FUN_ALWAYS_INLINE static
  //  int32 GetByteLengthUserType(const T& value) {
  //    //TODO MessageFieldTypeTraits<T>::GetByteLength 이 구현되어 있다면
  //    그것을 사용하도록 하는게 좋을듯 한데...??
  //    //return MessageFieldTypeTraits<T>::GetByteLength(value);
  //    //TODO HasMethod 구현
  //
  //    MessageByteCounter Counter;
  //    MessageFieldTypeTraits<T>::Write(Counter, value);
  //    return Counter.Length();
  //  }
  //
  //  template <typename T> FUN_ALWAYS_INLINE static
  //  int32 GetByteLengthWellKnownType(const T& value) {
  //    static_assert(0, "unhandled type");
  //    return 0;
  //  }
  //
  //  //Array
  //  template <typename ElementType, typename Allocator> FUN_ALWAYS_INLINE
  //  static int32 GetByteLengthWellKnownType(const Array<ElementType,
  //  Allocator>& value) {
  //    // Counter
  //    int32 length = MessageFormat::GetByteLength_Counter(Array.Count());
  //
  //    // Elements
  //    for (const auto& elem : value) {
  //      length += GetByteLength(elem);
  //    }
  //
  //    return length;
  //  }
  //
  //  //Array<uint8>
  //  template <typename Allocator>
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const
  //  Array<uint8, Allocator>& value) {
  //    return MessageFormat::GetByteLength_LengthPrefixed(value.Count());
  //  }
  //
  //  //Array<int8>
  //  template <typename Allocator>
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const
  //  Array<int8, Allocator>& value) {
  //    return MessageFormat::GetByteLength_LengthPrefixed(value.Count());
  //  }
  //
  //  //Map
  //  template <typename KeyType, typename ValueType> FUN_ALWAYS_INLINE static
  //  int32 GetByteLengthWellKnownType(const Map<KeyType, valueType>& value) {
  //    // Counter
  //    int32 Length = MessageFormat::GetByteLength_Counter(value.Count());
  //
  //    // Pairs
  //    for (const auto& pair : value) {
  //      Length += GetByteLength(pair.GetFirst());
  //      Length += GetByteLength(pair.GetSecond());
  //    }
  //    return Length;
  //  }
  //
  //  //@todo supports Set
  //
  //#if FUN_MESSAGEFORMAT_SUPPORT_STL
  //  //std::vector
  //  template <typename ValueType, typename Allocator> FUN_ALWAYS_INLINE static
  //  int32 GetByteLengthWellKnownType(const std::vector<ValueType, Allocator>&
  //  value)
  //  {
  //    // Counter
  //    int32 Length =
  //    MessageFormat::GetByteLength_Counter(static_cast<int32>(value.size()));
  //
  //    // Elements
  //    for (const auto& elem : value)
  //    {
  //      Length += GetByteLength(elem);
  //    }
  //
  //    return Length;
  //  }
  //
  //  //std::set
  //  template <typename KeyType, typename Compare, typename Allocator>
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const
  //  std::set<KeyType, Compare, Allocator>& value)
  //  {
  //    // Counter
  //    int32 Length =
  //    MessageFormat::GetByteLength_Counter(static_cast<int32>(value.size()));
  //
  //    // Elements
  //    for (const auto& elem : value) {
  //      Length += GetByteLength(elem);
  //    }
  //
  //    return Length;
  //  }
  //
  //  //std::map
  //  template <typename KeyType, typename MappedType, typename Compare,
  //  typename Allocator> FUN_ALWAYS_INLINE static int32
  //  GetByteLengthWellKnownType(const std::map<KeyType, MappedType, Compare,
  //  Allocator>& value)
  //  {
  //    // Counter
  //    int32 Length =
  //    MessageFormat::GetByteLength_Counter(static_cast<int32>(value.size()));
  //
  //    // Pairs
  //    for (const auto& pair : value) {
  //      Length += GetByteLength(pair.first);
  //      Length += GetByteLength(pair.second);
  //    }
  //
  //    return Length;
  //  }
  //#endif // FUN_MESSAGEFORMAT_SUPPORT_STL
  //
  //  template <typename T> FUN_ALWAYS_INLINE static
  //  int32 GetByteLengthEnumType(const T& value)
  //  {
  //    return MessageFormat::GetByteLength_Varint32SignExtended(value);
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const int8
  //  value)
  //  {
  //    return 1;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const int16
  //  value)
  //  {
  //    return 2;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const int32
  //  value)
  //  {
  //    return 4;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const int64
  //  value)
  //  {
  //    return 8;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const uint8
  //  value)
  //  {
  //    return 1;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const uint16
  //  value)
  //  {
  //    return 2;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const uint32
  //  value)
  //  {
  //    return 4;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const uint64
  //  value)
  //  {
  //    return 8;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const bool
  //  value)
  //  {
  //    return 1;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const float
  //  value)
  //  {
  //    return 4;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const double
  //  value)
  //  {
  //    return 8;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const DateTime&
  //  value)
  //  {
  //    return 8;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const Timespan&
  //  value)
  //  {
  //    return 8;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const Uuid&
  //  value)
  //  {
  //    return 1 + 16;
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const ByteArray&
  //  value)
  //  {
  //    return MessageFormat::GetByteLength_LengthPrefixed(value.Count());
  //  }
  //
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const String&
  //  value)
  //  {
  //    return MessageFormat::GetByteLength_String(value);
  //  }
  //
  //#if FUN_MESSAGEFORMAT_SUPPORT_STL
  //  //std::basic_string<char>
  //  template <typename Traits, typename Allocator>
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const
  //  std::basic_string<char, Traits, Allocator>& value)
  //  {
  //    const int32 utf8_len = static_cast<int32>(value.size());
  //    return MessageFormat::GetByteLength_LengthPrefixed(utf8_len);
  //  }
  //
  //  //std::basic_string<wchar_t>
  //  template <typename Traits, typename Allocator>
  //  FUN_ALWAYS_INLINE static int32 GetByteLengthWellKnownType(const
  //  std::basic_string<wchar_t, Traits, Allocator>& value)
  //  {
  //    const int32 utf8_len = CUtfConv::GetLength_StringAsUtf8(value.c_str(),
  //    static_cast<int32>(value.size())); return
  //    MessageFormat::GetByteLength_LengthPrefixed(utf8_len);
  //  }
  //#endif // FUN_MESSAGEFORMAT_SUPPORT_STL
};  // end of struct LiteFormat

}  // namespace net
}  // namespace fun
