#include "fun/net/socket/stream_socket_impl.h"

namespace fun {

StreamSocketImpl::StreamSocketImpl() {
  Init();
}

StreamSocketImpl::StreamSocketImpl(SOCKET fd) : SocketImpl(fd) {}

StreamSocketImpl::~StreamSocketImpl() {}

int32 StreamSocketImpl::SendBytes(const void* data, int32 length, int32 flags) {
  const char* src = reinterpret_cast<const char*>(data);
  int32 remaining = length;
  int32 sent = 0;
  const bool blocking = GetBlocking();
  while (remaining > 0) {
    const int32 n = SocketImpl::SendBytes(src, remaining, flags); //note: send failure시에 예외를 던지는 부분에 대해서 감안해야함.
    fun_check(n >= 0);
    src += n;
    sent += n;
    remaining -= n;
    if (blocking && remaining > 0) {
      CPlatformProcess::Sleep(0);
    } else {
      break;
    }
  }

  return sent;
}

} // namespace fun
