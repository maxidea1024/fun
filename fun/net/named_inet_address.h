#pragma once

#include "fun/net/inet_address.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

class FUN_NET_API NamedInetAddress {
 public:
  static NamedInetAddress None;

  NamedInetAddress();
  NamedInetAddress(const String& host_and_port);
  NamedInetAddress(const String& host, uint16 port);
  NamedInetAddress(const IpAddress& host, uint16 port);
  NamedInetAddress(const InetAddress& addr);
  NamedInetAddress(const NamedInetAddress& rhs);

  InetAddress ToInetAddress() const;

  void OverwriteHostNameIfExists(const String& host_name);

  bool operator==(const NamedInetAddress& rhs) const;
  bool operator!=(const NamedInetAddress& rhs) const;
  bool operator<(const NamedInetAddress& rhs) const;
  bool operator<=(const NamedInetAddress& rhs) const;
  bool operator>(const NamedInetAddress& rhs) const;
  bool operator>=(const NamedInetAddress& rhs) const;

  String ToString() const;

  bool IsLoopback() const;
  bool IsUnicast() const;

  FUN_ALWAYS_INLINE friend uint32 HashOf(const NamedInetAddress& addr) {
    return HashOf(addr.address_) ^ HashOf(addr.port_);
  }

  // FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar,
  // NamedInetAddress& addr)
  //{
  //  return ar & addr.address_ & addr.port_;
  //}

  // private:
 public:
  String address_;
  uint16 port_;
};

//
// inlines
//

FUN_ALWAYS_INLINE NamedInetAddress::NamedInetAddress() : address_(), port_(0) {}

FUN_ALWAYS_INLINE NamedInetAddress::NamedInetAddress(
    const String& host_and_port) {
  const String trimmed_host_and_port = host_and_port.Trimmed();

  //일단 address_ 문자열에는 포트 번호가 있으면 안됨.

  const int32 colon_index = trimmed_host_and_port.LastIndexOf(':');
  if (colon_index != INVALID_INDEX) {
    address_ = trimmed_host_and_port.Mid(0, colon_index);

    StringRef port_str = trimmed_host_and_port.MidRef(colon_index + 1);
    bool ok;
    const int32 port_value = port_str.ToInt32(&ok);
    if (!ok || port_value < 0 || port_value > 0xFFFF) {
      throw InvalidArgumentException(
          String::Format("Invalid port value: %s", *port_str));
    }

    port_ = uint16(port_value);
  } else {
    address_ = trimmed_host_and_port;
    port_ = 0;
  }

  if (address_.StartsWith('[') && address_.EndsWith(']')) {
    address_ = address_.Mid(1, address_.Len() - 2);
  }
}

FUN_ALWAYS_INLINE NamedInetAddress::NamedInetAddress(const String& host,
                                                     uint16 port)
    : address_(host), port_(port) {
  // Host 문자열에는 ":port"가 있어서는 안됨.
  address_.Trim();
}

FUN_ALWAYS_INLINE NamedInetAddress::NamedInetAddress(const IpAddress& host,
                                                     uint16 port)
    : address_(host.ToString()), port_(port) {}

FUN_ALWAYS_INLINE NamedInetAddress::NamedInetAddress(const InetAddress& addr)
    : address_(addr.GetHost().ToString()), port_(addr.GetPort()) {}

FUN_ALWAYS_INLINE NamedInetAddress::NamedInetAddress(
    const NamedInetAddress& rhs)
    : address_(rhs.address_), port_(rhs.port_) {
  fun_check(address_.SideSpaces() == 0);  // ensure that trimmed.
}

FUN_ALWAYS_INLINE InetAddress NamedInetAddress::ToInetAddress() const {
  // TODO 예외가 발생할려나... 처리를 해주어야겠지...
  //  일단은 귀찮으니 다음 코드리뷰시점에서...
  return InetAddress(address_, port_);
}

FUN_ALWAYS_INLINE void NamedInetAddress::OverwriteHostNameIfExists(
    const String& host_name) {
  const String trimmed_host_name = host_name.Trimmed();

  if (!trimmed_host_name.IsEmpty()) {
    address_ = trimmed_host_name;
  }
}

FUN_ALWAYS_INLINE bool NamedInetAddress::operator==(
    const NamedInetAddress& rhs) const {
  return address_ == rhs.address_ && port_ == rhs.port_;
}

FUN_ALWAYS_INLINE bool NamedInetAddress::operator!=(
    const NamedInetAddress& rhs) const {
  return address_ != rhs.address_ || port_ != rhs.port_;
}

FUN_ALWAYS_INLINE bool NamedInetAddress::operator<(
    const NamedInetAddress& rhs) const {
  if (address_ != rhs.address_) {
    if (address_ < rhs.address_) return true;
    if (address_ > rhs.address_) return false;
  }

  if (port_ != rhs.port_) {
    if (port_ < rhs.port_) return true;
    if (port_ > rhs.port_) return false;
  }

  return false;
}

FUN_ALWAYS_INLINE bool NamedInetAddress::operator<=(
    const NamedInetAddress& rhs) const {
  return !(rhs < *this);
}

FUN_ALWAYS_INLINE bool NamedInetAddress::operator>(
    const NamedInetAddress& rhs) const {
  return rhs < *this;
}

FUN_ALWAYS_INLINE bool NamedInetAddress::operator>=(
    const NamedInetAddress& rhs) const {
  return !(*this < rhs);
}

FUN_ALWAYS_INLINE String NamedInetAddress::ToString() const {
  // TODO optimization
  IpAddress addr(address_);

  if (addr.GetAddressFamily() == AddressFamily::IPv6) {
    return AsciiString("[") + address_ + AsciiString("]:") +
           String::FromNumber(port_);
  } else {
    return address_ + AsciiString(":") + String::FromNumber(port_);
  }
}

FUN_ALWAYS_INLINE bool NamedInetAddress::IsLoopback() const {
  // if (address_ == "localhost" || address_ == "127.0.0.1" || address_ ==
  // "::1")
  if (address_ == "localhost" || address_.StartsWith("127.") ||
      address_ == "::1") {
    return true;
  }

  // slow path
  IpAddress addr(address_);
  return addr.IsLoopback();
}

FUN_ALWAYS_INLINE bool NamedInetAddress::IsUnicast() const {
  if (port_ == 0 || port_ == 0xFFFF) {
    return false;
  }

  IpAddress addr(address_);
  return addr.IsUnicast();
}

FUN_ALWAYS_INLINE String ToString(const NamedInetAddress& addr) {
  return addr.ToString();
}

}  // namespace net
}  // namespace fun
