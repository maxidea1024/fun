#pragma once

#include "socket_impl.h"

namespace fun {
namespace net {

/**
 * This class implements a TCP socket.
 */
class FUN_NETSOCKET_API StreamSocketImpl : public SocketImpl {
 public:
  /**
   * Creates a SocketImpl, with the underlying
   * socket initialized
   */
  explicit StreamSocketImpl();

  /**
   * Creates a StreamSocketImpl using the given native socket.
   */
  StreamSocketImpl(SOCKET fd);

  /**
   * Ensures that all data in buffer is sent if the socket
   * is blocking. In case of a non-blocking socket, sends as
   * many bytes as possible.
   *
   * Returns the number of bytes sent. The return value may also be
   * negative to denote some special condition.
   */
  int32 SendBytes(const void* data, int32 len, int32 flags = 0) override;

 protected:
  virtual ~StreamSocketImpl();
};

}  // namespace net
}  // namespace fun
