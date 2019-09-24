#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * Used to force poll to wake up
 *
 * simply make poll watch for read events on one side of the pipe and
 * write to the other side
 */
class SelfPipe {
 public:
  SelfPipe();
  ~SelfPipe();

  SelfPipe(const SelfPipe&) = delete;
  SelfPipe& operator=(const SelfPipe&) = delete;

  /**
   * the read fd of the pipe
   */
  SOCKET GetReadFD() const;

  /**
   * the write fd of the pipe
   */
  SOCKET GetWriteFD() const;

  /**
   * notify the self pipe (basically write to the pipe)
   */
  void Notify();

  /**
   * Clear the pipe (basically read from the pipe)
   */
  void ClearBuffer();

 private:
#if FUN_PLATFORM_WINDOWS_FAMILY
  /**
   * socket fd.
   */
  SOCKET fd_;
  struct sockaddr addr_;
  int32 addr_len_;
#else
  /**
   * pipe file descriptors
   */
  fd_t fds_[2];
#endif
};

}  // namespace net
}  // namespace fun
