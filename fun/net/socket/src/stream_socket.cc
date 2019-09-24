#include "fun/net/socket/stream_socket.h"
#include "fun/net/socket/stream_socket_impl.h"

namespace fun {

StreamSocket::StreamSocket() : Socket(new StreamSocketImpl) {}

StreamSocket::StreamSocket(const InetAddress& address)
    : Socket(new StreamSocketImpl()) {
  Connect(address);
}

StreamSocket::StreamSocket(const Socket& socket) : Socket(socket) {
  if (dynamic_cast<StreamSocketImpl*>(GetImpl()) == nullptr) {
    throw InvalidArgumentException(
        CStringLiteral("cannot assign incompatible socket"));
  }
}

StreamSocket::StreamSocket(SocketImpl* impl) : Socket(impl) {
  if (dynamic_cast<StreamSocketImpl*>(GetImpl()) == nullptr) {
    throw InvalidArgumentException(
        CStringLiteral("cannot assign incompatible socket"));
  }
}

StreamSocket::~StreamSocket() {}

StreamSocket& StreamSocket::operator=(const Socket& socket) {
  if (dynamic_cast<StreamSocketImpl*>(socket.GetImpl()) != nullptr) {
    Socket::operator=(socket);
  } else {
    throw InvalidArgumentException(
        CStringLiteral("cannot assign incompatible socket"));
  }

  return *this;
}

void StreamSocket::Connect(const InetAddress& address) {
  GetImpl()->Connect(address);
}

void StreamSocket::Connect(const InetAddress& address,
                           const Timespan& timeout) {
  GetImpl()->Connect(address, timeout);
}

void StreamSocket::ConnectNB(const InetAddress& address) {
  GetImpl()->ConnectNB(address);
}

void StreamSocket::ShutdownReceive() { GetImpl()->ShutdownReceive(); }

void StreamSocket::ShutdownSend() { GetImpl()->ShutdownSend(); }

void StreamSocket::Shutdown() { GetImpl()->Shutdown(); }

int32 StreamSocket::SendBytes(const void* data, int32 length, int32 flags) {
  return GetImpl()->SendBytes(data, length, flags);
}

int32 StreamSocket::SendBytes(FIFOBuffer& buffer) {
  ScopedLock lock(buffer.GetMutex());

  const int32 sent = GetImpl()->SendBytes(buffer.begin(), (int32)buffer.Used());
  if (sent > 0) {
    buffer.Drain(sent);  // 보낸만큼 큐에서 데이터를 빼줌. (dequeue)
  }
  return sent;
}

int32 StreamSocket::ReceiveBytes(void* buffer, int32 length, int32 flags) {
  return GetImpl()->ReceiveBytes(buffer, length, flags);
}

int32 StreamSocket::ReceiveBytes(FIFOBuffer& buffer) {
  ScopedLock lock(buffer.GetMutex());

  const int32 received_byte_count =
      GetImpl()->ReceiveBytes(buffer.Next(), (int32)buffer.Available());
  if (received_byte_count > 0) {
    buffer.Advance(
        received_byte_count);  // 받은만큼 큐에 데이터를 넣어줌. (enqueue)
  }
  return received_byte_count;
}

void StreamSocket::SendUrgent(uint8 data) { GetImpl()->SendUrgent(data); }

}  // namespace fun
