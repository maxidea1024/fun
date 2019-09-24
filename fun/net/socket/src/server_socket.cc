#include "net/socket/server_socket.h"
#include "net/socket/server_socket_impl.h"

namespace fun {
namespace net {

ServerSocket::ServerSocket(SocketImpl* impl, bool) : Socket(impl) {
  if (!dynamic_cast<ServerSocketImpl*>(GetImpl())) {
    throw InvalidArgumentException(
        CStringLiteral("cannot assign incompatible socket"));
  }
}

ServerSocket::ServerSocket() : Socket(new ServerSocketImpl) {}

ServerSocket::ServerSocket(const Socket& socket) : Socket(socket) {
  if (!dynamic_cast<ServerSocketImpl*>(GetImpl())) {
    throw InvalidArgumentException(
        CStringLiteral("cannot assign incompatible socket"));
  }
}

ServerSocket::ServerSocket(const InetAddress& addr, int32 backlog)
    : Socket(new ServerSocketImpl) {
  GetImpl()->Bind(addr, true);
  GetImpl()->Listen(backlog);
}

ServerSocket::ServerSocket(uint16 port, int32 backlog)
    : Socket(new ServerSocketImpl) {
  IpAddress wildcard_addr;
  InetAddress addr(wildcard_addr, port);
  GetImpl()->Bind(addr, true);
  GetImpl()->Listen(backlog);
}

ServerSocket::~ServerSocket() {}

ServerSocket& ServerSocket::operator=(const Socket& socket) {
  if (dynamic_cast<ServerSocketImpl*>(socket.GetImpl())) {
    Socket::operator=(socket);
  } else {
    throw InvalidArgumentException(
        CStringLiteral("cannot assign incompatible socket"));
  }

  return *this;
}

void ServerSocket::Bind(const InetAddress& addr, bool reuse_addr) {
  GetImpl()->Bind(addr, reuse_addr);
}

void ServerSocket::Bind(const InetAddress& addr, bool reuse_addr,
                        bool reuse_port) {
  GetImpl()->Bind(addr, reuse_addr, reuse_port);
}

void ServerSocket::Bind(uint16 port, bool reuse_addr) {
  IpAddress wildcard_addr;
  InetAddress addr(wildcard_addr, port);
  GetImpl()->Bind(addr, reuse_addr);
}

void ServerSocket::Bind(uint16 port, bool reuse_addr, bool reuse_port) {
  IpAddress wildcard_addr;
  InetAddress addr(wildcard_addr, port);
  GetImpl()->Bind(addr, reuse_addr, reuse_port);
}

void ServerSocket::Bind6(const InetAddress& addr, bool reuse_addr,
                         bool ipv6_only) {
  GetImpl()->Bind6(addr, reuse_addr, ipv6_only);
}

void ServerSocket::Bind6(const InetAddress& addr, bool reuse_addr,
                         bool reuse_port, bool ipv6_only) {
  GetImpl()->Bind6(addr, reuse_addr, reuse_port, ipv6_only);
}

void ServerSocket::Bind6(uint16 port, bool reuse_addr, bool ipv6_only) {
  IpAddress wildcard_addr(IpAddress::IPv6Any);
  InetAddress addr(wildcard_addr, port);
  GetImpl()->Bind6(addr, reuse_addr, ipv6_only);
}

void ServerSocket::Bind6(uint16 port, bool reuse_addr, bool reuse_port,
                         bool ipv6_only) {
  IpAddress wildcard_addr(IpAddress::IPv6Any);
  InetAddress addr(wildcard_addr, port);
  GetImpl()->Bind6(addr, reuse_addr, reuse_port, ipv6_only);
}

void ServerSocket::Listen(int32 backlog) { GetImpl()->Listen(backlog); }

StreamSocket ServerSocket::AcceptConnection(InetAddress& addr) {
  return StreamSocket(GetImpl()->AcceptConnection(addr));
}

StreamSocket ServerSocket::AcceptConnection() {
  InetAddress addr;
  return StreamSocket(GetImpl()->AcceptConnection(addr));
}

}  // namespace net
}  // namespace fun
