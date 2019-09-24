#pragma once

#include "fun/net/message.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {
namespace net {

#define FUN_MESSAGEFIELD_ENUM_FXED8(EnumType) \
  template <typename T> \
  struct MessageFieldTypeTraits<T, typename EnableIf<IsSame<T,EnumType>::Value>::Type> { \
    typedef EnumType CppValueType; \
    FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) { \
      output.WriteFixed8((uint8)value); \
    } \
    FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) { \
      uint8 tmp; \
      FUN_DO_CHECKED(input.ReadFixed8(tmp)); \
      out_value = (EnumType)tmp; \
      return true; \
    } \
  };

#define FUN_MESSAGEFIELD_ENUM_FXED16(EnumType) \
  template <typename T> \
  struct MessageFieldTypeTraits<T, typename EnableIf<IsSame<T,EnumType>::Value>::Type> { \
    typedef EnumType CppValueType; \
    FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) { \
      output.WriteFixed16((uint16)value); \
    } \
    FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) { \
      uint16 tmp; \
      FUN_DO_CHECKED(input.ReadFixed16(tmp)); \
      out_value = (EnumType)tmp; \
      return true; \
    } \
  };

#define FUN_MESSAGEFIELD_ENUM_FXED32(EnumType) \
  template <typename T> \
  struct MessageFieldTypeTraits<T, typename EnableIf<IsSame<T,EnumType>::Value>::Type> { \
    typedef EnumType CppValueType; \
    FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) { \
      output.WriteFixed32((uint32)value); \
    } \
    FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) { \
      uint32 tmp; \
      FUN_DO_CHECKED(input.ReadFixed32(tmp)); \
      out_value = (EnumType)tmp; \
      return true; \
    } \
  };

#define FUN_MESSAGEFIELD_ENUM_FXED64(EnumType) \
  template <typename T> \
  struct MessageFieldTypeTraits<T, typename EnableIf<IsSame<T,EnumType>::Value>::Type> { \
    typedef EnumType CppValueType; \
    FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) { \
      output.WriteFixed64((uint64)value); \
    } \
    FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) { \
      uint64 tmp; \
      FUN_DO_CHECKED(input.ReadFixed64(tmp)); \
      out_value = (EnumType)tmp; \
      return true; \
    } \
  };

//TODO signed extended varint로 해야할듯...
#define FUN_MESSAGEFIELD_ENUM_VARINT(EnumType) \
  template <typename T> \
  struct MessageFieldTypeTraits<T, typename EnableIf<IsSame<T,EnumType>::Value>::Type> { \
    typedef EnumType CppValueType; \
    FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) { \
      output.WriteVarint32((uint32)value); \
    } \
    FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) { \
      uint32 tmp; \
      FUN_DO_CHECKED(input.ReadVarint32(tmp)); \
      out_value = (EnumType)tmp; \
      return true; \
    } \
  };



//
// Engine types.
//

/**
 * Specialization for NamedInetAddress type
 */
template <> struct MessageFieldTypeTraits<NamedInetAddress> {
  typedef NamedInetAddress CppValueType;

  FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) {
    LiteFormat::Write(output, value.Address);
    LiteFormat::Write(output, value.Port);
  }

  FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) {
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.Address));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.Port));
    return true;
  }
};


/**
 * Specialization for IpAddress type
 */
template <> struct MessageFieldTypeTraits<IpAddress>
{
  typedef IpAddress CppValueType;

  FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) {
    uint8 buff[IpAddress::IPv6AddressByteLength];
    const int32 length = value.GetAddressBytes(buff, sizeof(buff));
    MessageFormat::WriteBytes(output, buff, length);
  }

  FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) {
    int32 length;
    FUN_DO_CHECKED(MessageFormat::ReadCounter(input, length));
    if (!(length == IpAddress::IPv4AddressByteLength ||
          length == IpAddress::IPv6AddressByteLength)) {
      return false;
    }

    uint8 buff[IpAddress::IPv6AddressByteLength];
    FUN_DO_CHECKED(input.ReadRawBytes(buff, length));

    out_value = IpAddress(buff, length, 0);
    return true;
  }
};


/**
 * Specialization for InetAddress type
 */
template <> struct MessageFieldTypeTraits<InetAddress> {
  typedef InetAddress CppValueType;

  FUN_ALWAYS_INLINE static void Write(IMessageOut& output, const CppValueType& value) {
    MessageFieldTypeTraits<IpAddress>::Write(output, value.GetHost());
    LiteFormat::Write(output, value.GetPort());
  }

  FUN_ALWAYS_INLINE static bool Read(IMessageIn& input, CppValueType& out_value) {
    IpAddress host;
    FUN_DO_CHECKED(MessageFieldTypeTraits<IpAddress>::Read(input, host));

    uint16 port;
    FUN_DO_CHECKED(LiteFormat::Read(input, port));

    out_value = InetAddress(host, port);
    return true;
  }
};

} // namespace net
} // namespace fun
