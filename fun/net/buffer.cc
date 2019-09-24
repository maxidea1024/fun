#include "fun/net/buffer.h"

namespace fun {
namespace net {

const size_t Buffer::kCheapPrepend = 8;
const size_t Buffer::INITIAL_BUFFER_SIZE = 1024;

// linux 전용.
// windows에서는 어떤식으로 처리해야할까??
ssize_t Buffer::ReadFd(int fd, int* saved_errno) {
  char extra_buf[65536]; // 64k씩이나....???
  struct iovec vec[2];
  const size_t writable_len = GetWritableLength();
  vec[0].iov_base = begin() + writer_index_;
  vec[0].iov_len = writable_len;
  vec[1].iov_base = extra_buf;
  vec[1].iov_len = sizeof extra_buf;
  const int iov_count = (writable_len < sizeof extra_buf) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iov_count);
  if (n < 0) {
    *saved_errno = errno;
  } else if ((size_t)n <= writable_len) {
    writer_index_ += n;
  } else {
    writer_index_ = buffer_.Count();
    Append(extra_buf, n - writable_len);
  }

  return n;
}

} // namespace net
} // namespace fun
