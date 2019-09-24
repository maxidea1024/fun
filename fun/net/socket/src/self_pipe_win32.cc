#include "fun/net/self_pipe.h"

#if FUN_PLATFORM_WINDOWS_FAMILY

#include "fun/base/windows_less.h"

namespace fun {
namespace net {

SelfPipe::SelfPipe() : fd_(INVALID_SOCKET) {
  fd_ = socket(AF_INET, SOCK_DGRAM, 0);

  u_long flags = 1;
  ioctlsocket(fd_, FIONBIO, &flags);

  struct sockaddr_in inaddr;
  UnsafeMemory::Memset(&inaddr, 0, sizeof(inaddr));
  inaddr.sin_family = AF_INET;
  inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  inaddr.sin_port = 0;
  if (bind(fd_, (struct sockaddr*)&inaddr, sizeof(inaddr)) == SOCKET_ERROR) {
    // TODO
    //__TACOPIE_THROW(error, "fail bind()");
  }

  //! Retrieve server information
  addr_len = sizeof(addr_);
  UnsafeMemory::Memset(&addr_, 0, sizeof(addr_));
  if (getsockname(fd_, &addr_, &addr_len) == SOCKET_ERROR) {
    // TODO
    //__TACOPIE_THROW(error, "fail getsockname()");
  }

  //! connect read fd to the server
  if (connect(fd_, &addr_, addr_len) == SOCKET_ERROR) {
    // TODO
    //__TACOPIE_THROW(error, "fail connect()");
  }
}

SelfPipe::~SelfPipe() { closesocket(fd_); }

SOCKET SelfPipe::GetReadFD() const { return fd_; }

SOCKET SelfPipe::GetWriteFD() const { return fd_; }

void SelfPipe::Notify() { sendto(fd_, "a", 1, 0, &addr_, addr_len); }

void SelfPipe::ClearBuffer() {
  char buf[1024];
  recvfrom(fd_, buf, 1024, 0, &addr_, &addr_len);
}

}  // namespace net
}  // namespace fun

#endif  // FUN_PLATFORM_WINDOWS_FAMILY
