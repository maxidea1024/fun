#include "fun/net/self_pipe.h"

#if FUN_PLATFORM_UNIX_FAMILY

#include <fcntl.h>
#include <unistd.h>

namespace fun {
namespace net {

SelfPipe::SelfPipe() : fds_{-1, -1} {
  if (pipe(fds_) == -1) {
    // error handling..
  }
}

SelfPipe::SelfPipe() {
  if (fds_[0] != -1) {
    close(fds_[0]);
  }

  if (fds_[1] != -1) {
    close(fds_[1]);
  }
}

fd_t SelfPipe::GetReadFD() const {
  return fds_[0];
}

fd_t SelfPipe::GetWriteFD() const {
  return fds_[1];
}

void SelfPipe::Notify() {
  write(fds_[1], "a", 1);
}

void SelfPipe::ClearBuffer() {
  char buf[1024];
  read(fds_[0], buf, 1024);
}

} // namespace net
} // namespace fun

#endif // FUN_PLATFORM_UNIX_FAMILY
