#pragma once

namespace fun {
namespace net {

class InternalSocket;

/**
 */
class NetUtil {
 public:
  static Array<IpAddress>& LocalAddresses();
  static const IpAddress& LocalAddressAt(int32 Index);
  static void AssertCloseSocketWillReturnImmediately(SOCKET socket);
  static void SetTcpDefaultBehavior(InternalSocket* socket);
  static void SetUdpDefaultBehavior(InternalSocket* socket);
  // IPv4 혹은 IIPv4 compatible/mapped IPv6에서만 동작함.
  static bool IsSameSubnet24(const IpAddress& A, const IpAddress& B);

 private:
  static CCriticalSection2 CachedLocalAddressesCS;
  static Array<IpAddress> CachedLocalAddresses;
};

}  // namespace net
}  // namespace fun
