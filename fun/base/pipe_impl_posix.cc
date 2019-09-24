#include "fun/base/pipe_impl_posix.h"
#include "fun/base/exception.h"

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

namespace fun {

PipeImpl::PipeImpl() {
  int fds[2];
  int rc = pipe(fds);
  if (rc == 0) {
    read_fd_ = fds[0];
    write_fd_ = fds[1];
  } else {
    throw CreateFileException("anonymous pipe");
  }
}

PipeImpl::~PipeImpl() {
  CloseRead();
  CloseWrite();
}

int PipeImpl::WriteBytes(const void* data, int len) {
  fun_check(write_fd_ != -1);

  int n;
  do {
    n = write(write_fd_, data, len);
  } while (n < 0 && errno == EINTR);

  if (n >= 0) {
    return n;
  } else {
    throw WriteFileException("anonymous pipe");
  }
}

int PipeImpl::ReadBytes(void* buf, int len) {
  fun_check(read_fd_ != -1);

  int n;
  do {
    n = read(read_fd_, buf, len);
  } while (n < 0 && errno == EINTR);

  if (n >= 0) {
    return n;
  } else {
    throw ReadFileException("anonymous pipe");
  }
}

PipeImpl::Handle PipeImpl::ReadHandle() const {
  return read_fd_;
}

PipeImpl::Handle PipeImpl::WriteHandle() const {
  return write_fd_;
}

void PipeImpl::CloseRead() {
  if (read_fd_ != -1) {
    close(read_fd_);
    read_fd_ = -1;
  }
}

void PipeImpl::CloseWrite() {
  if (write_fd_ != -1) {
    close(write_fd_);
    write_fd_ = -1;
  }
}

} // namespace fun
