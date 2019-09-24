#pragma once

#include "fun/net/ip_address.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

class FUN_NET_API InetAddress {
 public:
  static const InetAddress None;

  InetAddress() : host_(), port_(0) {}

  explicit InetAddress(const IpAddress& host, uint16 port)
      : host_(host), port_(port) {}

  // TODO addressfamily가 먼저 오는게 자연스러울래나...
  /*
  INetAddress addr(AddressFamily::IPv4, 9090);
  INetAddress addr(AddressFamily::IPv6, 9090);
  */
  explicit InetAddress(uint16 port, AddressFamily family)
      : host_(family == AddressFamily::IPv6 ? IpAddress::IPv6Any
                                            : IpAddress::IPv4Any),
        port_(port) {}

  explicit InetAddress(uint16 port) : host_(IpAddress::IPv4Any), port_(port) {}

  explicit InetAddress(const String& host_str, uint16 port)
      : host_(host_str), port_(port) {}

  explicit InetAddress(const String& host_str, const String& port_str)
      : host_(host_str), port_(ResolveService(port_str)) {}

  explicit InetAddress(const String& host_and_port) {
    *this = Parse(host_and_port);
  }

  InetAddress(const InetAddress& rhs) : host_(rhs.host_), port_(rhs.port_) {}

  InetAddress& operator=(const InetAddress& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      host_ = rhs.host_;
      port_ = rhs.port_;
    }
    return *this;
  }

  InetAddress(InetAddress&& rhs)
      : host_(MoveTemp(rhs.host_)), port_(rhs.port_) {}

  InetAddress& operator=(InetAddress&& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      host_ = MoveTemp(rhs.host_);
      port_ = rhs.port_;
    }
    return *this;
  }

  InetAddress(const sockaddr_in& in4);
  InetAddress(const sockaddr_in6& in6);

  void FromNative(const sockaddr_in& in4);
  void FromNative(const sockaddr_in6& in6);

  void ToNative(sockaddr_in& in4) const;
  void ToNative(sockaddr_in6& in6) const;

  const IpAddress& GetHost() const { return host_; }

  void SetHost(const IpAddress& host) { host_ = host; }

  void SetHost(const String& host_str) { host_ = IpAddress::Parse(host_str); }

  uint16 GetPort() const { return port_; }

  void SetPort(uint16 port) { port_ = port; }

  void SetPort(const String& port_str) { port_ = ResolveService(port_str); }

  String ToIpString() const;
  String ToString() const;
  static InetAddress Parse(const String& host_and_port);
  static bool TryParse(const String& host_and_port, InetAddress& result);

  bool IsAny() const;
  bool IsUnicast() const;
  bool IsMulticast() const;

  bool IsSameHost(const IpAddress& other) const { return host_ == other; }

  bool IsSameHost(const InetAddress& other) const {
    return host_ == other.host_;
  }

  bool Equals(const InetAddress& other) const {
    return host_ == other.host_ && port_ == other.port_;
  }

  int32 Compare(const InetAddress& other) const {
    int32 host_diff = host_.Compare(other.host_);
    if (host_diff != 0) {
      return host_diff * 65536;
    }

    return (int32)port_ - (int32)other.port_;
  }

  bool operator==(const InetAddress& other) const { return Equals(other); }
  bool operator!=(const InetAddress& other) const { return !Equals(other); }
  bool operator<(const InetAddress& other) const { return Compare(other) < 0; }
  bool operator<=(const InetAddress& other) const {
    return Compare(other) <= 0;
  }
  bool operator>(const InetAddress& other) const { return Compare(other) > 0; }
  bool operator>=(const InetAddress& other) const {
    return Compare(other) >= 0;
  }

  friend uint32 HashOf(const InetAddress& addr) {
    return HashOf(addr.host_) ^ addr.port_;
  }

  // friend Archive& operator & (Archive& ar, InetAddress& addr) {
  //  return ar & addr.host_ & addr.port_;
  //}

  /*
    //for fun::ToString
    friend String& operator << (String& str, const InetAddress& addr) {
      if (host_.IsIPv4MappedToIPv6()) {
        return str << fun::Format("%s:%u", *host_.ToString(), port_);
      } else {
        return str << fun::Format("[%s]:%u", *host_.ToString(), port_);
      }
    }

    //for fun::FromString
    friend void operator >> (String& str, InetAddress& addr) {
      if (!TryParse(str, addr)) addr = None;
    }
  */

 private:
  IpAddress host_;
  uint16 port_;

  static uint16 ResolveService(const String& service);
};

// TODO 제거하자.
FUN_ALWAYS_INLINE String ToString(const InetAddress& addr) {
  return addr.ToString();
}

}  // namespace net
}  // namespace fun
