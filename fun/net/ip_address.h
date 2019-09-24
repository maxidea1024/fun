#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * TODO 어드레스 패밀리를 무시하면 처리가 간단해짐.
 * 최종 어드레스는 IPv6이고, IPv4 mapped만 지원하는 형태??
 * 
 * 어쨌거나, IPv6는 지원해야하지만 복잡해져서는 안됨.
 * 
 * 아래 주소는 가급적, IPv6 소켓에 사용해야함.
 * 
 * 
 * scopeid를 시리얼라이징에 적용해야하는가??
 * 
 * 
 * 각 어드레스 값은 native-endian 레이아웃을 기준으로 지정합니다.
 * 
 * 
 * 예를들어, localhost 즉, "127.0.0.1"인 경우에는
 * 0x7f000001이 됩니다.
 */
class FUN_NET_API IpAddress {
  friend class InetAddress;

 public:
  static IpAddress IPv4Any;
  static IpAddress IPv4Loopback;
  static IpAddress IPv4Broadcast;
  static IpAddress IPv4None;

  static IpAddress IPv6Any;
  static IpAddress IPv6Loopback;
  static IpAddress IPv6None;

  static const uint32 IPv4LoopbackMask = 0xFF0000; // 127.0.0.1

  static const int32 IPv4AddressByteLength = 4;
  static const int32 IPv6AddressByteLength = 16;

  static const int32 NumIPv6Words = IPv6AddressByteLength / 2;

  /**
   * Default constructor
   */
  IpAddress();

  /**
   * 127.0.0.1 = 0x7F000001
   */
  explicit IpAddress(uint32 ipv4_addr);

  IpAddress(const uint16* ipv6_words, uint32 scope_id = 0);

  IpAddress(const uint8* bytes, int32 length, uint32 scope_id = 0);

  explicit IpAddress(const String& str);

  FUN_ALWAYS_INLINE IpAddress(const IpAddress& rhs) {
    scope_id_ = rhs.scope_id_;

    for (int32 i = 0; i < NumIPv6Words; ++i) {
      ipv6_words_[i] = rhs.ipv6_words_[i];
    }
  }

  FUN_ALWAYS_INLINE IpAddress& operator = (const IpAddress& rhs) {
    scope_id_ = rhs.scope_id_;

    for (int32 i = 0; i < NumIPv6Words; ++i) {
      ipv6_words_[i] = rhs.ipv6_words_[i];
    }
    return *this;
  }

  void Synthesize(const uint8* prefix, int32 prefix_len, uint32 ipv4_addr);

  IpAddress(const in_addr& in4);
  IpAddress(const in6_addr& in6, uint32 scope_id);

  void FromNative(const in_addr& in4);
  void FromNative(const in6_addr& in6, uint32 scope_id);

  void ToNative(in_addr& in4) const;
  void ToNative(in6_addr& in6) const;

  /**
   * Returns the address family (AddressFamily::IPv4, AddressFamily::IPv6) of the address.
   */
  FUN_ALWAYS_INLINE AddressFamily GetAddressFamily() const {
    return (IsIPv4MappedToIPv6() || IsIPv4Compatible()) ? AddressFamily::IPv4 : AddressFamily::IPv6;
  }

  /**
   * Returns the address family (AF_INET or AF_INET6) of the address.
   */
  FUN_ALWAYS_INLINE int af() const {
    return GetAddressFamily() == AddressFamily::IPv4 ? AF_INET : AF_INET6;
  }

  FUN_ALWAYS_INLINE uint32 GetScopeId() const {
    fun_check(!IsIPv4MappedToIPv6());
    return scope_id_;
  }

  FUN_ALWAYS_INLINE static bool IsLoopback(const IpAddress& addr) {
    if (addr.IsIPv4MappedToIPv6()) {
      return ((addr.ipv4_addr_ & IPv4LoopbackMask) == (IPv4Loopback.ipv4_addr_ & IPv4LoopbackMask));
    } else {
      return addr == IPv6Loopback;
    }
  }

  FUN_ALWAYS_INLINE bool IsLoopback() const {
    return IsLoopback(*this);
  }

  FUN_ALWAYS_INLINE static bool IsBroadcast(const IpAddress& addr) {
    if (addr.IsIPv4MappedToIPv6()) {
      return addr == IPv4Broadcast;
    } else {
      // No such thing as a broadcast address for IPv6
      return false;
    }
  }

  FUN_ALWAYS_INLINE bool IsAny() const {
    return *this == IPv6Any || *this == IPv4Any;
  }

  /**
   * Always return false if IPv6
   */
  FUN_ALWAYS_INLINE bool IsBroadcast() const {
    return IsBroadcast(*this);
  }

  FUN_ALWAYS_INLINE bool IsIPv6Multicast() const {
    return (ipv6_words_[0] & 0xFF00) == 0xFF00;
  }

  FUN_ALWAYS_INLINE bool IsIPv6LinkLocal() const {
    return (ipv6_words_[0] & 0xFFC0) == 0xFE80;
  }

  FUN_ALWAYS_INLINE bool IsIPv6SiteLocal() const {
    return (ipv6_words_[0] & 0xFFC0) == 0xFEC0;
  }

  FUN_ALWAYS_INLINE bool IsIPv6Tredo() const {
    return ipv6_words_[0] == 0x2001 && ipv6_words_[1] == 0;
  }

  FUN_ALWAYS_INLINE bool IsIPv4MappedToIPv6() const {
    return  ipv6_words_[0] == 0 &&
            ipv6_words_[1] == 0 &&
            ipv6_words_[2] == 0 &&
            ipv6_words_[3] == 0 &&
            ipv6_words_[4] == 0 &&
            ipv6_words_[5] == 0xFFFF;
  }

  FUN_ALWAYS_INLINE bool IsIPv4Compatible() const {
    return  ipv6_words_[0] == 0 &&
            ipv6_words_[1] == 0 &&
            ipv6_words_[2] == 0 &&
            ipv6_words_[3] == 0 &&
            ipv6_words_[4] == 0 &&
            ipv6_words_[5] == 0;
  }

  bool IsUnicast() const;

  bool IsMulticast() const;

  // IPv4 192.168.1.1 maps as ::FFFF:192.168.1.1
  //IpAddress MapToIPv6() const;
  //IpAddress MapToIPv4() const;

  FUN_ALWAYS_INLINE bool GetIPv4Address(uint32& out_ipv4_addr) const {
    if (!IsIPv4MappedToIPv6()) {
      return false;
    }

    out_ipv4_addr = ipv4_addr_;
    return true;
  }

  FUN_ALWAYS_INLINE void SetIPv4MappedToIPv6(uint32 ipv4_addr) {
    scope_id_ = 0;

    ipv6_words_[0] = 0;
    ipv6_words_[1] = 0;
    ipv6_words_[2] = 0;
    ipv6_words_[3] = 0;
    ipv6_words_[4] = 0;
    ipv6_words_[5] = 0xFFFF;

    ipv4_addr_ = ipv4_addr;
  }

  FUN_ALWAYS_INLINE void SetIPv6Address(const uint16* ipv6_words, uint32 scope_id) {
    *this = IpAddress(ipv6_words, scope_id);
  }

  int32 GetAddressBytes(uint8* buffer, int32 length) const;

  String ToString() const;
  static IpAddress Parse(const String& string);
  static bool TryParse(const String& string, IpAddress& out_address);

  FUN_ALWAYS_INLINE int32 Compare(const IpAddress& other) const {
    return UnsafeMemory::Memcmp(ipv6_words_, other.ipv6_words_, IPv6AddressByteLength);
  }

  FUN_ALWAYS_INLINE bool Equals(const IpAddress& other, bool compare_scoped_id) const {
    for (int32 i = 0; i < NumIPv6Words; ++i) {
      if (ipv6_words_[i] != other.ipv6_words_[i]) {
        return false;
      }
    }

    if (compare_scoped_id) {
      if (scope_id_ != other.scope_id_) {
        return false;
      }
    }

    return true;
  }

  FUN_ALWAYS_INLINE bool Equals(const IpAddress& other) const {
    return Equals(other, true);
  }

  FUN_ALWAYS_INLINE bool operator == (const IpAddress& other) const { return  Equals(other); }
  FUN_ALWAYS_INLINE bool operator != (const IpAddress& other) const { return !Equals(other); }
  FUN_ALWAYS_INLINE bool operator <  (const IpAddress& other) const { return Compare(other) <  0; }
  FUN_ALWAYS_INLINE bool operator <= (const IpAddress& other) const { return Compare(other) <= 0; }
  FUN_ALWAYS_INLINE bool operator >  (const IpAddress& other) const { return Compare(other) >  0; }
  FUN_ALWAYS_INLINE bool operator >= (const IpAddress& other) const { return Compare(other) >= 0; }

  FUN_ALWAYS_INLINE friend uint32 HashOf(const IpAddress& addr) {
    uint32 hash = 0;
    for (int32 i = 0; i < NumIPv6Words; ++i) {
      hash ^= addr.ipv6_words_[i];
    }

    return hash;
  }

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, IpAddress& addr) {
    //Little endian 기준으로 시리얼라이징됨...
//#if FUN_ARCH_LITTLE_ENDIAN
//#else
//#endif
    for (int32 i = 0; i < NumIPv6Words; ++i) {
      ar & addr.ipv6_words_[i]; // 여기서 endian 이슈를 다루므로, 문제 없음.
    }

    ar & addr.scope_id_;

    return ar;
  }

 private:
  /**
   * address field
   */
  alignas(4) union {
    alignas(4) struct {
      uint8 filler_[IPv6AddressByteLength - IPv4AddressByteLength];
      uint32 ipv4_addr_;
    };
    uint8 ipv6_bytes_[IPv6AddressByteLength];
    uint16 ipv6_words_[NumIPv6Words];
  };

  /** Scope ID(valid IPv6 only) */
  uint32 scope_id_;
};

//TODO TextStream 연산자로 처리하는게 좋을듯...
FUN_ALWAYS_INLINE String ToString(const IpAddress& addr) {
  return addr.ToString();
}

} // namespace net
} // namespace fun
