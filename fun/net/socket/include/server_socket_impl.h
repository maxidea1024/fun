#pragma once

#include "socket_impl.h"

namespace fun {

class FUN_NETSOCKET_API ServerSocketImpl : public SocketImpl {
 public:
  ServerSocketImpl();

 protected:
  ~ServerSocketImpl();
};

} // namespace fun
