#include "socket2.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

SocketSelectContext* SocketSelectContext::New() {
  return new SocketSelectContextImpl();
}

void SocketSelectContextImpl::AddWriteWaiter(Socket2& socket) {
  //@todo dynamic_cast사용을 피할 방법은 없을까??

  Socket2Impl* ptr = nullptr;

  try {
    ptr = dynamic_cast<Socket2Impl*>(&socket);
  } catch (std::bad_cast& e) {
    OutputDebugString(UTF8_TO_TCHAR(e.what()));
    int32* X = nullptr;
    *X = 1;
  }

  if (ptr) {
    selection_context_.AddWriteWaiter(*(ptr->internal_socket_.Get()));
  }
}

void SocketSelectContextImpl::AddExceptionWaiter(Socket2& socket) {
  Socket2Impl* ptr = nullptr;

  try {
    ptr = dynamic_cast<Socket2Impl*>(&socket);
  } catch (std::bad_cast& e) {
    OutputDebugString(UTF8_TO_TCHAR(e.what()));
    int32* X = nullptr;
    *X = 1;
  }

  if (ptr) {
    selection_context_.AddExceptionWaiter(*(ptr->internal_socket_.Get()));
  }
}

void SocketSelectContextImpl::Wait(uint32 MSec) {
  selection_context_.Wait(MSec);
}

bool SocketSelectContextImpl::GetConnectResult(Socket2& socket,
                                               SocketErrorCode& out_code) {
  Socket2Impl* ptr = nullptr;

  try {
    ptr = dynamic_cast<Socket2Impl*>(&socket);
  } catch (std::bad_cast& e) {
    OutputDebugString(UTF8_TO_TCHAR(e.what()));
    int32* X = nullptr;
    *X = 1;
  }

  if (ptr) {
    return selection_context_.GetConnectResult(*(ptr->internal_socket_.Get()),
                                               out_code);
  }
  return false;
}

Socket2Impl::Socket2Impl(SOCKET exisiting_socket, ISocketDelegate* delegate)
    : internal_delegate_(delegate) {
  internal_socket_.Reset(
      new internal_socket_(exisiting_socket, (IInternalSocketDelegate*)this));
}

Socket2Impl::Socket2Impl(SocketType socket_type, ISocketDelegate* delegate)
    : internal_delegate_(delegate) {
  internal_socket_.Reset(
      new internal_socket_(sockettype, (IInternalSocketDelegate*)this));
}

bool Socket2Impl::Bind() { return internal_socket_->Bind(); }

bool Socket2Impl::Bind(int32 port) { return internal_socket_->Bind(port); }

bool Socket2Impl::Bind(const char* ip, int32 port) {
  return internal_socket_->Bind(ip, port);
}

SocketErrorCode Socket2Impl::Connect(const String& host, int32 port) {
  return internal_socket_->Connect(host, port);
}

SocketErrorCode Socket2Impl::IssueRecvFrom(int32 len) {
  return internal_socket_->IssueRecvFrom(len);
}

SocketErrorCode Socket2Impl::IssueSendTo(const uint8* data, int32 len,
                                         const InetAddress& sendto) {
  return internal_socket_->IssueSendTo(data, len, sendto);
}

SocketErrorCode Socket2Impl::IssueRecv(int32 len) {
  return internal_socket_->IssueRecv(len);
}

SocketErrorCode Socket2Impl::IssueSend(const uint8* data, int32 len) {
  return internal_socket_->IssueSend(data, len);
}

bool Socket2Impl::GetRecvOverlappedResult(
    bool wait_until_complete, OverlappedResult& out_overlapped_result) {
  return internal_socket_->GetRecvOverlappedResult(wait_until_complete,
                                                   out_overlapped_result);
}

bool Socket2Impl::GetSendOverlappedResult(
    bool wait_until_complete, OverlappedResult& out_overlapped_result) {
  return internal_socket_->GetSendOverlappedResult(wait_until_complete,
                                                   out_overlapped_result);
}

bool Socket2Impl::GetAcceptExOverlappedResult(
    bool wait_until_complete, OverlappedResult& out_overlapped_result) {
  return internal_socket_->GetAcceptExOverlappedResult(wait_until_complete,
                                                       out_overlapped_result);
}

InetAddress Socket2Impl::GetSockName() {
  return internal_socket_->GetSockName();
}

InetAddress Socket2Impl::GetPeerName() {
  return internal_socket_->GetPeerName();
}

void Socket2Impl::SetBlockingMode(bool blocking) {
  internal_socket_->SetBlockingMode(blocking);
}

uint8* Socket2Impl::GetRecvBufferPtr() {
  return internal_socket_->GetRecvBufferPtr();
}

Socket2* Socket2::New(SOCKET exisiting_socket, ISocketDelegate* delegate) {
  return new Socket2Impl(exisiting_socket, delegate);
}

Socket2* Socket2::New(SocketType socket_type, ISocketDelegate* delegate) {
  return new Socket2Impl(socket_type, delegate);
}

}  // namespace net
}  // namespace fun
