#include "fun/net/net.h"

namespace fun {
namespace net {

CompletionPort::CompletionPort( ICompletionPortCallbacks* event,
                                int32 max_concurrent_thread_count) {
  event_ = event;

  // Create real IOCP if user needs and OS suits
  iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                        nullptr,
                                        0,
                                        max_concurrent_thread_count);
  if (iocp_handle_ == nullptr) {
    throw Exception("Cannot create IOCP object!");
  }
}

void CompletionPort::PostWin32Warning(const char* where) {
  event_->OnCompletionPortWarning(this, *String::Format("failure at %s: %d", where, GetLastError()));
}

void CompletionPort::AssociateSocket(InternalSocket* sock) {
  if (!sock_->completion_context_) {
    throw Exception("first, set the completion context of the socket!");
  }

  if (sock_->associated_iocp_) {
    throw Exception("it has already been Associated!");
  }

  if (!::CreateIoCompletionPort((HANDLE)sock_->sock_,
                                iocp_handle_,
                                (ULONG_PTR)socket,
                                0)) {
    PostWin32Warning(__FUNCTION__);
  }
}

bool CompletionPort::GetQueuedCompletionStatus( CompletionStatus* out_result,
                                                uint32 timeout_msec) {
  DWORD bytes = 0;
  ULONG_PTR key = 0;
  OverlappedEx* overlapped = nullptr;

  *out_result = CompletionStatus();

  SetLastError(0);

  // warning: 반환 값은 고려하지 않음.
  ::GetQueuedCompletionStatus(iocp_handle_, &bytes, &key, (OVERLAPPED**)&overlapped, timeout_msec);

  // Overlapped가 null일 경우에는 timeout등이 발생한 경우이므로, 일단은 false를 반환하고 다음 호출을 처리.
  if (overlapped == nullptr) {
    out_result->sock_error = (SocketErrorCode)GetLastError();
    return false;
  }

  //TODO 액션 형태로 모두 재정의해서 사용하면 구조가 간단해질듯 싶은데...

  // InternalSocket의 가상 소멸자 상속관련해서, 포인터 전달시 문제가 있음.
  // 현재 InternalSocket이 상속받는 ListNode<>, ICompletionKey의 가상 소멸자를
  // 제거하여 문제를 피하고 있지만, 조금더 살펴봐야할듯 싶다.
  auto completion_key = (ICompletionKey*)key; // InternalSocket or null

  if (completion_key) {
    out_result->completion_context = completion_key->GetCompletionContext();
    out_result->key = completion_key;
  } else {
    out_result->completion_context = nullptr;
  }

  if (IocpCustomValueInRange((INT_PTR)overlapped)) {
    //fun_check(key == 0);
    out_result->type = CompletionType::ReferCustomValue;
    out_result->completed_length = bytes;
    out_result->custom_value = (UINT_PTR)overlapped;
    out_result->sock_error = SocketErrorCode::Ok;
    out_result->recvfrom_addr = InetAddress::None;
    out_result->recv_flags = 0;
    return true;
  }

  // 여기까지 왔다면, key는 InternalSocket
  auto spi = (InternalSocket*)key;

  // 여기서부터 Overlapped는 유효한 주소값이다.
  if (spi && overlapped == &spi->recv_ovl_) {
    out_result->type = CompletionType::Receive;
    out_result->recv_flags = spi->recv_flags_;
#if FUN_DISABLE_IPV6
    out_result->recvfrom_addr.FromNativeIPv4(spi->recvfrom_addr);
#else
    out_result->recvfrom_addr.FromNativeIPv6(spi->recvfrom_addr);
#endif
    spi->recv_ovl_.issued = false;
  } else if (spi && overlapped == &spi->send_ovl_) {
    out_result->type = CompletionType::Send;
    //out_result->sendto_addr = InetAddress((sockaddr*)&spi->sendto_addr, spi->SendToAddrLen); //@note 이건 테스트용...
    spi->send_ovl_.issued = false;
  } else if (spi && overlapped == &spi->accept_ex_ovl_) {
    out_result->type = CompletionType::AcceptEx;
    spi->accept_ex_ovl_.issued = false;
  } else if (spi && overlapped == &spi->connect_ex_ovl_) {
    out_result->type = CompletionType::ConnectEx;
    spi->connect_ex_ovl_.issued = false;
  } else {
    out_result->type = CompletionType::ReferCustomValue;
    out_result->custom_value = (UINT_PTR)overlapped;
  }

  out_result->completed_length = bytes;

  if ((int32)bytes < 0) {
    out_result->sock_error = (SocketErrorCode)GetLastError();
  } else {
    out_result->sock_error = SocketErrorCode::Ok;
  }

  if (GetLastError() == WSAEMSGSIZE) {
    // 해커가 보낸 메시지로도 생길 수 있으므로 ShowUserMisuseError는 과잉진압. 하지만 분명히 경고는 남겨서 해결을 도모해야 할지도.
    //@todo
    //LOG(LogNetEngine, Warning, "FunNet always send MTU-length aware packets, thus, never occurs WSAEMSGSIZE! You are missing something! WSAEMSGSIZE will cause ResultCode::InvalidPacketFormat!");
  }

  return true;
}

bool CompletionPort::GetQueuedCompletionStatusEx( Array<CompletionStatus>& out_result,
                                                  int32& dequeue_count,
                                                  uint32 timeout_msec) {
  if (out_result.Count() < GQCS_EX_REMOVED_COUNT) {
    return false;
  }

  FUN_OVERLAPPED_ENTRY overlapped_entry[GQCS_EX_REMOVED_COUNT];
  //UnsafeMemory::Memzero(overlapped_entry, GQCS_EX_REMOVED_COUNT);
  ULONG removed_entry_count = 0;

  SetLastError(0); // GQCS가 값을 업뎃 안하는 경우도 커버 위함

  if (!::GetQueuedCompletionStatusEx( iocp_handle_,
                                      overlapped_entry,
                                      GQCS_EX_REMOVED_COUNT,
                                      &removed_entry_count,
                                      timeout_msec,
                                      FALSE)) {
    DWORD error = GetLastError();

    switch (error) {
      case ERROR_SUCCESS:
      case WAIT_TIMEOUT:
        break;

      default:
        fun_unexpected();
        break;
    }

    return false;
  }

  // no detection GQCS
  if (removed_entry_count <= 0) {
    return false;
  }

  dequeue_count = removed_entry_count;

  for (uint32 i = 0; i < removed_entry_count; ++i) {
    CompletionStatus status;

    if (overlapped_entry[i].lpOverlapped == nullptr) {
      status.sock_error = SocketErrorCode::Error;
      out_result[i] = status;
      continue;
    }

    auto completion_key = (ICompletionKey*)overlapped_entry[i].lpCompletionKey;
    if (overlapped_entry[i].lpCompletionKey != 0) {
      status.completion_context = completion_key->GetCompletionContext();
      status.key = completion_key;
    } else {
      status.completion_context = nullptr;
    }

    if (IocpCustomValueInRange((INT_PTR)overlapped_entry[i].lpOverlapped)) {
      status.type = CompletionType::ReferCustomValue;
      status.completed_length = overlapped_entry[i].dwNumberOfBytesTransferred; // 0이 아니라.
      status.custom_value = (UINT_PTR)overlapped_entry[i].lpOverlapped;
      status.sock_error = SocketErrorCode::Ok;
      status.recvfrom_addr = InetAddress::None;
      status.recv_flags = 0;

      out_result[i] = status;
      continue;
    }

    // 여기 까지 왔다면, key는 InternalSocket이다.
    auto spi = (InternalSocket*)overlapped_entry[i].lpCompletionKey;

    // 여기서부터 Overlapped는 유효한 주소값이다.
    if (spi && overlapped_entry[i].lpOverlapped == &spi->recv_ovl_) {
      status.type = CompletionType::Receive;
      status.recv_flags = spi->recv_flags_;
#if FUN_DISABLE_IPV6
      status.recvfrom_addr.FromNativeIPv4(spi->recvfrom_addr);
#else
      status.recvfrom_addr.FromNativeIPv6(spi->recvfrom_addr);
#endif
      spi->recv_ovl_.issued = false;
    } else if (spi && overlapped_entry[i].lpOverlapped == &spi->send_ovl_) {
      status.type = CompletionType::Send;
      spi->send_ovl_.issued = false;
    } else if (spi && overlapped_entry[i].lpOverlapped == &spi->accept_ex_ovl_) {
      status.type = CompletionType::AcceptEx;
      spi->accept_ex_ovl_.issued = false;
    } else if (spi && overlapped_entry[i].lpOverlapped == &spi->connect_ex_ovl_) {
      status.type = CompletionType::ConnectEx;
      spi->connect_ex_ovl_.issued = false;
    } else {
      status.type = CompletionType::ReferCustomValue;
      status.custom_value = (UINT_PTR)overlapped_entry[i].lpOverlapped;
    }

    status.completed_length = overlapped_entry[i].dwNumberOfBytesTransferred;

    if ((int32)overlapped_entry[i].dwNumberOfBytesTransferred < 0) {
      status.sock_error = SocketErrorCode::Error;
    } else {
      status.sock_error = SocketErrorCode::Ok;
    }

    out_result[i] = status;
  }

  return true;
}

void CompletionPort::Post(ICompletionKey* key, uintptr_t value) {
  // Key에는 InternalSocket 객체의 주소가 들어간다.
  // GQCS에서는 T/F 값 대신 Overlapped가 NULL인지로 체크할터이니.
  fun_check(IocpCustomValueInRange(value));

  // CustomValue는 OVERLAPPED의 주소 값 대신으로써 들어간다.
  ::PostQueuedCompletionStatus(iocp_handle_, 0, (ULONG_PTR)key, (OVERLAPPED*)value);
}

CompletionPort::~CompletionPort() {
  if (iocp_handle_) {
    CloseHandle(iocp_handle_);
    iocp_handle_ = nullptr;
  }
}

} // namespace net
} // namespace fun
