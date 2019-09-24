#include "fun/net/inet_address.h"

namespace fun {
namespace net {

const InetAddress InetAddress::None;

InetAddress::InetAddress(const sockaddr_in& in4) {
  FromNativeIPv4(in4);
}

InetAddress::InetAddress(const sockaddr_in6& in6) {
  FromNative(in6);
}

void InetAddress::FromNativeIPv4(const sockaddr_in& in4) {
  fun_check(in4.sin_family == AF_INET);

  host_.FromNativeIPv4(in4.sin_addr);
  port_ = ByteOrder::FromBigEndian(in4.sin_port);
}

void InetAddress::FromNative(const sockaddr_in6& in6) {
  fun_check(in6.sin6_family == AF_INET6);

  host_.FromNative(in6.sin6_addr, in6.sin6_scope_id);
  port_ = ByteOrder::FromBigEndian(in6.sin6_port);
}

void InetAddress::ToNative(sockaddr_in& in4) const {
  UnsafeMemory::Memzero(&in4, sizeof(in4));

  in4.sin_family = AF_INET;
  fun_set_sin_len(&in4);
  host_.ToNative(in4.sin_addr);
  in4.sin_port = ByteOrder::ToBigEndian(port_);
}

void InetAddress::ToNative(sockaddr_in6& in6) const {
  UnsafeMemory::Memzero(&in6, sizeof(in6));

  in6.sin6_family = AF_INET6;
  fun_set_sin6_len(&in6);
  host_.ToNative(in6.sin6_addr);
  in6.sin6_port = ByteOrder::ToBigEndian(port_);
  in6.sin6_scope_id = host_.ScopeId;
}

String InetAddress::ToIpString() const {
  if (host_.IsIPv4MappedToIPv6()) {
    return fun::Format("%s", *host_.ToString());
  } else {
    return fun::Format("[%s]", *host_.ToString());
  }
}

String InetAddress::ToString() const {
  if (host_.IsIPv4MappedToIPv6()) {
    return fun::Format("%s:%u", *host_.ToString(), port_);
  } else {
    return fun::Format("[%s]:%u", *host_.ToString(), port_);
  }
}

bool InetAddress::TryParse(const String& host_and_port, InetAddress& result) {
  try {
    result = Parse(host_and_port);
    return true;
  } catch (const Exception&) {
    result = InetAddress::None;
    return false;
  }
}

InetAddress InetAddress::Parse(const String& host_and_port) {
  fun_check(host_and_port.Len() > 0);

  String host(256, ReservationInit);
  String port(5, ReservationInit);

  int32 i = 0, end = host_and_port.Len();
//#if FUN_PLATFORM_UNIX_FAMILY
//  if (host_and_port[i] == '/') {
//    NewLocal(host_and_port);
//    return;
//  }
//#endif
  if (host_and_port[i] == '[') {
    ++i;
    while (i != end && host_and_port[i] != ']') {
      host += host_and_port[i++];
    }

    if (i == end) {
      throw InvalidArgumentException(StringLiteral("Malformed IPv6 address"));
    }

    ++i;
  } else {
    while (i != end && host_and_port[i] != ':') {
      host += host_and_port[i++];
    }
  }

  if (i != end && host_and_port[i] == ':') {
    ++i;
    while (i != end) {
      port += host_and_port[i++];
    }
  } else {
    //FIXME: 포트가 지정되지 않았을 경우에는 protocol에 사용되는 기본 포트를 배정하면 될듯헌데??
    throw InvalidArgumentException(StringLiteral("Missing port number"));
  }

  return InetAddress(IpAddress(host), ResolveService(port));
}

uint16 InetAddress::ResolveService(const String& service) {
  bool num_ok = false;
  const uint32 port = service.ToUInt32(&num_ok);
  if (num_ok && port <= 0xFFFF) {
    return uint16(port);
  } else {
    // find out port number by protocol
    if (struct servent* se = getservbyname(service.c_str(), nullptr)) {
      return se->s_port;
    } else {
      throw ServiceNotFoundException(service);
    }
  }
}

bool InetAddress::IsAny() const {
  return host_.IsAny() && port_ == 0;
}

bool InetAddress::IsUnicast() const {
  return host_.IsUnicast() && port_ != 0 && port_ != 0xFFFF;
}

bool InetAddress::IsMulticast() const {
  return host_.IsMulticast() && port_ != 0 && port_ != 0xFFFF;
}

} // namespace net
} // namespace fun
