#pragma once

#include "socket.h"

namespace fun {

class SocketImpl;

class FUN_NETSOCKET_API ServerSocket : public Socket {
 public:
  ServerSocket();
  ServerSocket(const Socket& socket);
  ServerSocket(const InetAddress& addr, int32 backlog = 64);
  ServerSocket(uint16 port, int32 backlog = 64);
  virtual ~ServerSocket();

  ServerSocket& operator = (const Socket& socket);

  virtual void Bind(const InetAddress& addr, bool reuse_addr = false);
  virtual void Bind(const InetAddress& addr, bool reuse_addr, bool reuse_port);
  virtual void Bind(uint16 port, bool reuse_addr = false);
  virtual void Bind(uint16 port, bool reuse_addr, bool reuse_port);
  virtual void Bind6(const InetAddress& addr, bool reuse_addr = false, bool ipv6_only = false);
  virtual void Bind6(const InetAddress& addr, bool reuse_addr, bool reuse_port, bool ipv6_only);
  virtual void Bind6(uint16 port, bool reuse_addr = false, bool ipv6_only = false);
  virtual void Bind6(uint16 port, bool reuse_addr, bool reuse_port, bool ipv6_only);
  virtual void Listen(int32 backlog = 64);
  virtual StreamSocket AcceptConnection(InetAddress& addr);
  virtual StreamSocket AcceptConnection();

 protected:
  ServerSocket(SocketImpl* impl, bool);
};

} // namespace fun
