#include "fun/net/ip_address.h"

namespace fun {
namespace net {

IpAddress IpAddress::IPv4Any(0);
IpAddress IpAddress::IPv4Loopback(0x7F000001);
IpAddress IpAddress::IPv4Broadcast(0xFFFFFFFF);
IpAddress IpAddress::IPv4None(0xFFFFFFFF);

static const uint16 IPv6AnyWords[8]      = {0,0,0,0,0,0,0,0};
static const uint16 IPv6LoopbackWords[8] = {0,0,0,0,0,0,0,1};
static const uint16 IPv6NoneWords[8]     = {0,0,0,0,0,0,0,0};

IpAddress IpAddress::IPv6Any(IPv6AnyWords, 0);
IpAddress IpAddress::IPv6Loopback(IPv6LoopbackWords, 0);
IpAddress IpAddress::IPv6None(IPv6NoneWords, 0);

IpAddress::IpAddress() {
  //scope_id_ = 0;
  //UnsafeMemory::Memset(ipv6_words_, 0, sizeof(ipv6_words_));
  SetIPv4MappedToIPv6(0); // IPv4 Any
}

IpAddress::IpAddress(const String& str) {
  *this = Parse(str);
}

IpAddress::IpAddress(uint32 ipv4_addr) {
  SetIPv4MappedToIPv6(ipv4_addr);
}

IpAddress::IpAddress(const uint16* ipv6_words, uint32 scope_id) {
  fun_check_ptr(ipv6_words);

  scope_id_ = scope_id;
  UnsafeMemory::Memcpy(this->ipv6_words_, ipv6_words_, IPv6AddressByteLength);
}

IpAddress::IpAddress(const uint8* bytes, int32 length, uint32 scope_id) {
  fun_check_ptr(bytes);

  if (length == IPv4AddressByteLength) {
    fun_check(scope_id == 0);

    SetIPv4MappedToIPv6(uint32(bytes[3] << 24) |
                        uint32(bytes[2] << 16) |
                        uint32(bytes[1] << 8) |
                        uint32(bytes[0]));
  } else if (length == IPv6AddressByteLength) {
    scope_id_ = scope_id;

    for (int32 i = 0; i < NumIPv6Words; ++i) {
      ipv6_words_[i] = uint16(bytes[i*2] + bytes[i*2+1] * 256);
    }
  } else {
    //TODO throw exception
  }
}

void IpAddress::Synthesize(const uint8* prefix, int32 prefix_len, uint32 ipv4_addr) {
  //TODO
  fun_check(0);
}

int32 IpAddress::GetAddressBytes(uint8* buffer, int32 length) const {
  if (IsIPv4MappedToIPv6()) {
    if (length < IPv4AddressByteLength) {
      //TODO throw exception
      fun_check(0);
    }

    uint8* dst = buffer;
    *dst++ = uint8(ipv4_addr_);
    *dst++ = uint8(ipv4_addr_ >> 8);
    *dst++ = uint8(ipv4_addr_ >> 16);
    *dst++ = uint8(ipv4_addr_ >> 24);
    return IPv4AddressByteLength;
  } else {
    if (length < IPv6AddressByteLength) {
      //TODO throw exception
      fun_check(0);
    }

    uint8* dst = buffer;
    for (int32 i = 0; i < NumIPv6Words; ++i) {
      *dst++ = uint8(ipv6_words_[i] & 0xFF);
      *dst++ = uint8(ipv6_words_[i] >> 8);
    }
    return IPv6AddressByteLength;
  }
}

IpAddress::IpAddress(const in_addr& in4) {
  FromNative(in4);
}

IpAddress::IpAddress(const in6_addr& in6, uint32 scope_id) {
  FromNative(in6, scope_id);
}

// IPv4 192.168.1.1 maps as ::FFFF:192.168.1.1
//IpAddress IpAddress::MapToIPv6() const {
//  if (Family == AddressFamily::IPv6) {
//    return *this;
//  } else {
//    uint16 ipv6_words[8] = {0};
//    ipv6_words[5] = 0xFFFF;
//    ipv6_words[6] = uint16(((ipv4_addr_ & 0x0000FF00) >> 8) | ((ipv4_addr_ & 0x000000FF) << 8));
//    ipv6_words[7] = uint16(((ipv4_addr_ & 0xFF000000) >> 24) | ((ipv4_addr_ & 0x00FF0000) >> 8));
//    return IpAddress(ipv6_words, 0);
//  }
//}
//
//IpAddress IpAddress::MapToIPv4() const {
//  if (GetFamily() == AddressFamily::IPv4) {
//    return *this;
//  } else {
//    //ipv6_words_[5] 값이 0xFFFF가 아니면 오동작아닌가??
//
//    const uint32 addr_ipv4 = ((((uint32)ipv6_words_[6] & 0x0000FF00) >> 8) | (((uint32)ipv6_words_[6] & 0x000000FF) << 8)) |
//                (((((uint32)ipv6_words_[7] & 0x0000FF00) >> 8) | (((uint32)ipv6_words_[7] & 0x000000FF) << 8)) << 16);
//    return IpAddress(addr_ipv4);
//  }
//}

void IpAddress::FromNative(const in_addr& in4) {
  SetIPv4MappedToIPv6(ByteOrder::FromBigEndian(in4.S_un.S_addr));
}

void IpAddress::FromNative(const in6_addr& in6, uint32 scope_id) {
#if FUN_ARCH_LITTLE_ENDIAN
  uint16 words[NumIPv6Words];
  for (int32 i = 0; i < NumIPv6Words; ++i) {
    words[i] = ByteOrder::FromBigEndian(in6.u.Word[i]);
  }
  SetIPv6Address(words, scope_id);
#else
  SetIPv6Address((const uint16*)in6.u.Word, scope_id);
#endif
}

void IpAddress::ToNative(in_addr& in4) const {
  fun_check(IsIPv4MappedToIPv6());
  //if (!IsIPv4MappedToIPv6()) {
  //  LOG(LogCore, Warning, "Invalid ipv4 address: %s", *ToString());
  //}

  UnsafeMemory::Memset(&in4, 0x00, sizeof(in4));
  in4.S_un.S_addr = ByteOrder::ToBigEndian(ipv4_addr_);
}

void IpAddress::ToNative(in6_addr& in6) const {
  UnsafeMemory::Memset(&in6, 0x00, sizeof(in6));

  //if (IsIPv4MappedToIPv6()) {
  //  LOG(LogCore,Info,"mapped address: %s", *ToString());
  //}

  if (*this == IPv4Any) {
    IPv6Any.ToNative(in6);
  } else if (*this == IPv4Loopback) {
    IPv6Loopback.ToNative(in6);
  } else {
    for (int32 i = 0; i < NumIPv6Words; ++i) {
      in6.u.Word[i] = ByteOrder::ToBigEndian(ipv6_words_[i]);
    }
  }
}

String IpAddress::ToString() const {
  if (IsIPv4MappedToIPv6()) {
    const uint8 a = uint8(ipv4_addr_ >> 24);
    const uint8 b = uint8(ipv4_addr_ >> 16);
    const uint8 c = uint8(ipv4_addr_ >> 8);
    const uint8 d = uint8(ipv4_addr_);
    //return String::Format("::ffff:%u.%u.%u.%u", a, b, c, d);
    return String::Format("%u.%u.%u.%u", a, b, c, d);
  } else {
    String result(64, ReservationInit);

    bool zero_sequence = false;
    int32 i = 0;
    while (i < 8) {
      if (!zero_sequence && ipv6_words_[i] == 0) {
        int32 zi = i;
        while (zi < 8 && ipv6_words_[zi] == 0) {
          ++zi;
        }

        if (zi > (i + 1)) {
          i = zi;
          result << AsciiString(":");
          zero_sequence = true;
        }
      }

      if (i > 0) {
        result << AsciiString(":");
      }

      if (i < 8) {
        result << String::Format("%x", ipv6_words_[i++]);
      }
    }

    if (scope_id_ != 0) {
      result << AsciiString("%");
#if FUN_PLATFORM_WINDOWS_FAMILY
      result << String::FromNumber(scope_id_);
#else
      char buffer[IFNAMSIZ];
      if (if_indextoname(scope_id_, buffer)) {
        result << AsciiString(buffer);
      } else {
        result << String::FromNumber(scope_id_);
      }
#endif
    }

    result.MakeLower();

    return result;
  }
}

IpAddress IpAddress::Parse(const String& addr) {
  if (addr.IsEmpty()) {
    return IpAddress();
  }

  if (addr == "*") {
    return IpAddress::IPv4Any;
  }

  //TODO 먼저 IP 리터럴로 해보고 안되면, getaddrinfo를 통해서 해야하지 않을까??

#if FUN_PLATFORM_WINDOWS_FAMILY
  struct addrinfo* ai;
  struct addrinfo hints;
  UnsafeMemory::Memset(&hints, 0, sizeof(hints));
  //힌트를 지정하면, FQDN이 resolve가 안되는 문제가 있네... API 문서를 확인해봐야할듯...
  //hints.ai_flags = AI_NUMERICHOST;
  //hints.ai_flags = AI_NUMERICHOST

  const int rc = getaddrinfo(addr.c_str(), nullptr, &hints, &ai);
  if (rc == 0) {
    IpAddress result;
    if (ai->ai_family == AF_INET6) {
      result = IpAddress(
          reinterpret_cast<struct sockaddr_in6*>(ai->ai_addr)->sin6_addr,
          static_cast<uint32>(reinterpret_cast<struct sockaddr_in6*>(ai->ai_addr)->sin6_scope_id));
    } else if (ai->ai_family == AF_INET) {
      result = IpAddress(reinterpret_cast<struct sockaddr_in*>(ai->ai_addr)->sin_addr);
    } else {
      // unsupported...
    }
    freeaddrinfo(ai);
    return result;
  } else {
    return IpAddress();
  }
#else
  const int32 percent_index = addr.IndexOf('%');
  if (percent_index >= 0) {
    const int32 start = ('[' == addr[0]) ? 1 : 0;
    String unscoped_addr(*addr + start, percent_index - start);
    String scope(*addr + percent_index + 1, addr.Len() - start - percent_index);

    uint32 scope_id_ = 0;
    if (!(scope_id_ = if_nametoindex(*scope))) {
      return IpAddress();
    }

    struct in6_addr ia6;
    if (inet_pton(AF_INET6, *unscoped_addr, &ia6) == 1) {
      return IpAddress(ia6, scope_id_);
    } else {
      return IpAddress();
    }
  } else {
    struct in6_addr ia6;
    struct in_addr ia4;
    if (inet_pton(AF_INET6, *addr, &ia6) == 1) {
      return IpAddress(&ia6);
    } else if (inet_pton(AF_INET4, *addr, &ia4) == 1) {
      return IpAddress(&ia4);
    } else {
      return IpAddress();
    }
  }
#endif
}

bool IpAddress::TryParse(const String& string, IpAddress& out_address) {
  const IpAddress result = Parse(string);
  if (result != IpAddress()) {
    out_address = result;
    return true;
  } else {
    out_address = IpAddress();
    return false;
  }
}

bool IpAddress::IsUnicast() const {
  if (IsIPv4MappedToIPv6()) {
    return *this != IPv4Any && *this != IPv4Broadcast && !IsMulticast();
  } else {
    return *this != IPv6Any && !IsMulticast();
  }
}

bool IpAddress::IsMulticast() const {
  if (IsIPv4MappedToIPv6()) {
    return (ipv4_addr_ & 0xF0000000) == 0xE0000000; // 224.0.0.0/24 to 239.0.0.0/24
  } else {
    return (ipv6_words_[0] & 0xFFE0) == 0xFF00;
  }
}

} // namespace net
} // namespace fun
