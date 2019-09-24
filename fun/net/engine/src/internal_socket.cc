// TODO 윈도우즈 전용이므로, 폴더를 따로 관리하는게 좋을듯함.
//@maxidea: todo: 소켓 작업 실패시 오류 메시지를 좀더 자세하게 보여줄수 있도록
//하자.

// IPV6_V6ONLY 이슈
//윈도우즈에서는 기본값이 0이 아니라함. 즉, IPv4 주소는 허용하지 않는다는...
//쩝... http://www.minzkn.com/moniwiki/wiki.php/DualStackIPForWindows

#include "fun/net/net.h"

#include "InternalSocket.h"
#include "completion_port_.h"

namespace fun {
namespace net {

#pragma comment(lib, "Mswsock.lib")  // GetAcceptExSockaddrs

AtomicCounter g_msg_size_error_count = 0;  // WSAEMSGSIZE 에러난 횟수
AtomicCounter g_net_reset_error_count = 0;  // WSAENETRESET(10052) 에러난 횟수
AtomicCounter g_conn_reset_error_count = 0;  // WSAECONNRESET(10054) 에러난 횟수

// MSDN에 다르면, 최대길이(즉, IPv6)보다 16바이트 커야한다고함.
// https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms737524(v=vs.85).aspx
// const int32 AddrLengthInTdi = InetAddress::MAX_ADDRESS_LENGTH + 16;

// NOTE 이렇게 잡아주는게 맞을래나??
// InetAddress::MAX_ADDRESS_LENGTH + 16로 잡아주었더니, 주소가 null이 나오네...
// 아후..
const int32 AddrLengthInTdi = sizeof(SOCKADDR_STORAGE) + 16;

#if FUN_DISABLE_IPV6
static const int32 SOCKET_ADDR_FAMILY = AF_INET;
#else
static const int32 SOCKET_ADDR_FAMILY = AF_INET6;
#endif

//이게 더 안전할것 같은데...??
// const int32 AddrLengthInTdi = sizeof(SOCKADDR_STORAGE) + 16;
//실제로 sizeof(SOCKADDR_STORAGE)이 InetAddress::MAX_ADDRESS_LENGTH와 같은
//값일까??? SOCKADDR_STORAGE이 표준에 의한 형태인지??

FUN_ALIGNED_VOLATILE LPFN_ACCEPTEX InternalSocket::lpfnAcceptEx = nullptr;

#if _WIN32_WINNT >= 0x0501  // connectex 0x0501이상에서만 사용가능.
FUN_ALIGNED_VOLATILE LPFN_CONNECTEX InternalSocket::lpfnConnectEx = nullptr;
#endif

void InternalSocket::Restore(bool bIsTcp) {
  CScopedLock2 Lock(socket_closed_mutex_);

  if (!socket_closed_or_closing_mutex_protected_) {
    throw InvalidArgumentException();
  }

  if (bIsTcp) {
    socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_STREAM, 0, nullptr, 0,
                        WSA_FLAG_OVERLAPPED);
  } else {
    socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_DGRAM, 0, nullptr, 0,
                        WSA_FLAG_OVERLAPPED);
  }

  if (socket_ == INVALID_SOCKET) {
    throw Exception(String::Format("socket creation failure: error={0}",
                                   WSAGetLastError()));
  }

  socket_closed_or_closing_mutex_protected_ = false;  // 이게 아래로 와야!
  // RestoredCount++;

  SetIPv6Only(false);
}

InternalSocket::InternalSocket(SocketType type,
                               IInternalSocketDelegate* delegate)
    : io_completion_delegate_(nullptr) {
  delegate_ = delegate;

  broadcast_option_enabled_ = false;

  InitOthers();

  switch (type) {
    case SocketType::Tcp:
      socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_STREAM, 0, nullptr, 0,
                          WSA_FLAG_OVERLAPPED);
      break;

    case SocketType::Udp:
      socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_DGRAM, 0, nullptr, 0,
                          WSA_FLAG_OVERLAPPED);
      break;

    case SocketType::Raw:
      socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_RAW, 0, nullptr, 0,
                          WSA_FLAG_OVERLAPPED);
      break;

    default:
      LOG(LogNetEngine, Warning, "Invalid parameter in InternalSocket ctor!");
      break;
  }

  if (socket_ == INVALID_SOCKET) {
    throw Exception(
        String::Format("socket creation failure: error=%d", WSAGetLastError()));
  }

  // <=== 이 기능은 사용하지 말자. 무슨 부작용이 있을지 모르니까!!!
  // 어차피 MTU 커버리지가 낮은 라우터는 이 기능을 사용하더라도 문제 해결에
  // 도움을 주지 못한다. 따라서 불필요!!

  // don't frag를 끈다. 즉 frag를 허용한다.
  // allow frag를 해두어야 ICMP까지도 다 막아버린 과잉진압 방화벽에 대해서
  // path MTU discovery 과정에서 blackhole connection이 발생하지 않을 것이다.
  // 확인된 바는 없고 추정일 뿐이지만 말이다.
  // AllowPacketFragmentation(true);

  SetIPv6Only(false);
}

InternalSocket::InternalSocket(SOCKET socket, IInternalSocketDelegate* delegate)
    : io_completion_delegate_(nullptr) {
  socket_ = socket;
  delegate_ = delegate;

  broadcast_option_enabled_ = false;

  InitOthers();

  SetIPv6Only(false);
}

InternalSocket::InternalSocket(
    SOCKET socket, IInternalSocketDelegate* delegate,
    ISocketIoCompletionDelegate* io_completion_delegate) {
  socket_ = socket;
  delegate_ = delegate;
  io_completion_delegate_ = io_completion_delegate;

  broadcast_option_enabled_ = false;

  InitOthers();

  SetIPv6Only(false);
}

InternalSocket::InternalSocket(
    SocketType type, IInternalSocketDelegate* delegate,
    ISocketIoCompletionDelegate* io_completion_delegate) {
  delegate_ = delegate;
  io_completion_delegate_ = io_completion_delegate;

  broadcast_option_enabled_ = false;

  InitOthers();

  switch (type) {
    case SocketType::Tcp:
      socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_STREAM, 0, nullptr, 0,
                          WSA_FLAG_OVERLAPPED);
      break;

    case SocketType::Udp:
      socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_DGRAM, 0, nullptr, 0,
                          WSA_FLAG_OVERLAPPED);
      break;

    case SocketType::Raw:
      socket_ = WSASocket(SOCKET_ADDR_FAMILY, SOCK_RAW, 0, nullptr, 0,
                          WSA_FLAG_OVERLAPPED);
      break;

    default:
      LOG(LogNetEngine, Warning, "Invalid parameter in InternalSocket ctor!");
      break;
  }

  if (socket_ == INVALID_SOCKET) {
    throw Exception(String::Format("socket creation failure: error={0}",
                                   WSAGetLastError()));
  }

  // <=== 이 기능은 사용하지 말자. 무슨 부작용이 있을지 모르니까!!!
  // 어차피 MTU 커버리지가 낮은 라우터는 이 기능을 사용하더라도 문제 해결에
  // 도움을 주지 못한다. 따라서 불필요!!

  // don't frag를 끈다. 즉 frag를 허용한다.
  // allow frag를 해두어야 ICMP까지도 다 막아버린 과잉진압 방화벽에 대해서
  // path MTU discovery 과정에서 blackhole connection이 발생하지 않을 것이다.
  // 확인된 바는 없고 추정일 뿐이지만 말이다.
  // AllowPacketFragmentation(true);

  SetIPv6Only(false);
}

bool InternalSocket::Bind() { return Bind(0); }

bool InternalSocket::Bind(int32 port) {
  // InetAddress WildcardEndPoint(IpAddress::Wildcard(IpAddress::IPv6), port);
  InetAddress any(port);
  return Bind(any);
}

bool InternalSocket::Bind(const char* host, int32 port) {
  if (host == nullptr || *host == 0 ||
      CStringTraitsU::Stricmp(host, "localhost") == 0 ||
      CStringTraitsU::Stricmp(host, "::1") == 0) {
    return Bind(port);
  } else {
    return Bind(InetAddress(host, port));
  }
}

bool InternalSocket::Bind(const InetAddress& local_addr) {
#if FUN_DISABLE_IPV6
  sockaddr_in sa;
  local_addr.ToNative(sa);
#else
  sockaddr_in6 sa;
  local_addr.ToNative(sa);
#endif

  if (bind(socket_, (const sockaddr*)&sa, sizeof(sa)) == 0) {
    return true;
  } else {
    PostSocketWarning(WSAGetLastError(), __FUNCTION__);
    return false;
  }
}

void InternalSocket::PostSocketWarning(DWORD error_code, const char* where) {
  if (!InternalSocket::IsPendingErrorCode((SocketErrorCode)error_code)) {
    // if (verbose_) {
    const String text =
        String::Format("{0}: fails (error_code: {1})", where, error_code);
    delegate_->OnSocketWarning(this, text);
    //}
  }
}

InternalSocket* InternalSocket::Accept(int32& out_error) {
  SOCKET accepted_socket = accept(socket_, nullptr, nullptr);
  if (accepted_socket == INVALID_SOCKET) {
    out_error = WSAGetLastError();
    return nullptr;
  } else {
    out_error = 0;  // no error
    return new InternalSocket(accepted_socket, delegate_);
  }
}

// TODO 주소에 맞춰서, 소켓을 생성해야하나??
//주소체계에 따라서 소켓을 달리생성해야하나??
//이게 맞는듯 싶은데??

SocketErrorCode InternalSocket::Connect(const String& host, int32 port) {
  try {
    InetAddress addr(host, port);

    // TODO Networker_C에서 connect가 여러번 불리는 문제가 있는데...
    // 의도인건가??
    LOG(LogNetEngine, Warning, "%s: connect to %s", __FUNCTION__,
        *addr.ToString());

    // TODO address family에 따라서 소켓을 달리 생성해야함.

    /*
    sockaddr_in6 native_addr;
    addr.ToNative(native_addr);

    if (connect(socket_, (const sockaddr*)&native_addr, sizeof(native_addr)) ==
    SOCKET_ERROR) { const DWORD error = WSAGetLastError();
      PostSocketWarning(error, __FUNCTION__);
      return (SocketErrorCode)error;
    }
    return SocketErrorCode::Ok;
    */

    // if (addr.GetHost().IsIPv4MappedToIPv6()) {
    //  sockaddr_in sa;
    //  addr.ToNative(sa);
    //  if (connect(socket_, (const sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
    //  {
    //    const DWORD error = WSAGetLastError();
    //    PostSocketWarning(error, __FUNCTION__);
    //    return (SocketErrorCode)error;
    //  }
    //}
    // else {
#if FUN_DISABLE_IPV6
    sockaddr_in sa;
    addr.ToNative(sa);
#else
    sockaddr_in6 sa;
    addr.ToNative(sa);
#endif

    if (connect(socket_, (const sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) {
      const DWORD error = WSAGetLastError();
      PostSocketWarning(error, __FUNCTION__);
      return (SocketErrorCode)error;
    }
  }
  return SocketErrorCode::Ok;
}
catch (Exception&) {
  // TODO logging???
  //뭐라고 해야하나???
  // TODO
  return SocketErrorCode::ConnectionRefused;
}
}  // namespace net

SocketErrorCode InternalSocket::IssueRecv(int32 length) {
  if (RecvIssued()) {
    // 경고만 낼 뿐 무시 처리는 안한다. 이건 검사 목적일 뿐이고 정확하지
    // 않으니까.
    delegate_->OnSocketWarning(this, "WARNING: IssueRecv is duplicated!");
  }

  if (length == 0) {
    return SocketErrorCode::InvalidArgument;  // 0 이하의 수신 자체는
                                              // 무의미한데다 socket 오동작을
                                              // 유발하므로
  }

  // recv_buffer_.ResizeUninitialized(length);
  recv_buffer_.ResizeZeroed(length);

  WSABUF buf;
  buf.buf = (char*)recv_buffer_.MutableData();
  buf.len = length;

  recv_ovl_.issued = true;

  recv_flags_ = 0;

  DWORD recv_byte_count = 0;
  DWORD error;
  const int32 rc = WSARecv(socket_, &buf, 1, &recv_byte_count, &recv_flags_,
                           &recv_ovl_, nullptr);
  if (rc != 0 && (error = WSAGetLastError()) != WSA_IO_PENDING) {
    recv_ovl_.issued = false;

    PostSocketWarning(error, __FUNCTION__);
    return (SocketErrorCode)error;
  }

  return SocketErrorCode::Ok;
}

SocketErrorCode InternalSocket::IssueRecvFrom(int32 length) {
  if (RecvIssued()) {
    // 경고만 낼 뿐 무시 처리는 안한다. 이건 검사 목적일 뿐이고 정확하지
    // 않으니까.
    delegate_->OnSocketWarning(this, "WARNING: IssueRecvFrom is duplicated!");
  }

  if (length == 0) {
    return SocketErrorCode::InvalidArgument;  // 0 이하의 수신 자체는
                                              // 무의미한데다 socket 오동작을
                                              // 유발하므로
  }

  // recv_buffer_.ResizeUninitialized(length);
  recv_buffer_.ResizeZeroed(length);

  WSABUF buf;
  buf.buf = (char*)recv_buffer_.MutableData();
  buf.len = length;

  recv_flags_ = 0;

  DWORD out_done = 0;
  int32 retry_count = 0;
  while (true) {
    // TODO Address family에 따라서 달리해야 정상동작하나봄!!
    // TODO Address family에 따라서 달리해야 정상동작하나봄!!
    // TODO Address family에 따라서 달리해야 정상동작하나봄!!
    // TODO Address family에 따라서 달리해야 정상동작하나봄!!
    // TODO Address family에 따라서 달리해야 정상동작하나봄!!
    // TODO Address family에 따라서 달리해야 정상동작하나봄!!
    // TODO Address family에 따라서 달리해야 정상동작하나봄!!
    // int32 recvfrom_addr_len_ = sizeof(recvfrom_addr_);
    recvfrom_addr_len_ = sizeof(recvfrom_addr_);

    recv_ovl_.issued = true;

    const int32 rc = WSARecvFrom(socket_, &buf, 1, &out_done, &recv_flags_,
                                 (SOCKADDR*)&recvfrom_addr_,
                                 &recvfrom_addr_len_, &recv_ovl_, nullptr);
    if (rc == 0) {
      return SocketErrorCode::Ok;  // 성공한거다.
    }

    const DWORD error = WSAGetLastError();
    switch (error) {
      case WSA_IO_PENDING:
        return SocketErrorCode::Ok;  // 성공한거다.

      case WSAEMSGSIZE:
        // 수신은 되었지만 나머지 패킷들은 모두 무시된다.
        // 부분이 잘린 패킷을 받았으므로 사용하는 것도 거시기하다. 그냥 버려야
        // 한다. 차피 RZ 자체가 UDP coalesce를 MTU 크기 기반으로 하므로 정상적
        // 상황에서는 이런 오류가 올 일이 없어야 한다. DN에서는 WSA_IO_PENDING
        // 아니면 no completion signal이므로 시도로 이어져야 한다.
        g_msg_size_error_count.Increment();
        break;

      case WSAENETRESET:
        // NetReset의 경우도 connreset과 동일하게 처리. WSARecvFrom 도움말에
        // 의하면 ttl expired 패킷이 와도 이게 온다고라. 근거:
        // http://msdn.microsoft.com/en-us/library/ms741686(v=vs.85).aspx
        g_net_reset_error_count.Increment();
        break;

      case WSAECONNRESET:
        // 실험 결과,
        // 이미 닫힌 소켓은 10054가 절대 안온다.
        // ICMP host unreachable이 도착한 횟수만큼만 WSARecvFrom가 온다. 즉
        // 무한루프의 위험이 없다. (그럼 VTG CASE는 뭐지?) GQCS는 안온다.
        //
        // 따라서 횟수 제한 없이 재시도를 해도 된다. 즉 이 로직이 맞다능.
        //
        // if (IsClosedOrClosing()) {
        //  LOG(LogNetEngine, Warning, "임시~~~WSAECONNRESET!");
        // } else {
        //  String text;
        //  text.Format("건재한 UDP 소켓에서 WSAECONNRESET이 옴. 재시도
        //  횟수={0}\n", retry_count); OutputDebugString(text); retry_count++;
        // }
        g_conn_reset_error_count.Increment();
        break;

      default:
        // 에러가 난 케이스다.
        recv_ovl_.issued = false;

        PostSocketWarning(error, __FUNCTION__);
        return (SocketErrorCode)error;
    }
  }
  return SocketErrorCode::Ok;
}

//데이터를 보내기전에 복사가 일어나는데 구지 이렇게 해야하나 싶은데...  줄일수도
//있지 않을까??
SocketErrorCode InternalSocket::IssueSend(const uint8* data, int32 data_len) {
  if (SendIssued()) {
    // 경고만 낼 뿐 무시 처리는 안한다. 이건 검사 목적일 뿐이고 정확하지
    // 않으니까.
    delegate_->OnSocketWarning(this, "WARNING: IssueSend is duplicated!");
  }

  if (data_len <=
      0) {  // 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로
    return SocketErrorCode::InvalidArgument;
  }

  //@todo 여기서 카피를 꼭해야하나??  상위레벨에서 버퍼를 유지하는게 좋을듯
  //싶은데...
  send_buffer_.ResizeUninitialized(data_len);
  UnsafeMemory::Memcpy(send_buffer_.MutableData(), data, data_len);

  WSABUF buf;
  buf.buf = (char*)send_buffer_.MutableData();
  buf.len = data_len;

  send_ovl_.issued = true;

  DWORD error;
  DWORD out_done = 0;
  DWORD flags = 0;
  const int32 rc =
      WSASend(socket_, &buf, 1, &out_done, flags, &send_ovl_, nullptr);
  if (rc < 0 && (error = WSAGetLastError()) != WSA_IO_PENDING) {
    send_ovl_.issued = false;

    PostSocketWarning(error, __FUNCTION__);
    return (SocketErrorCode)error;
  } else {
    io_pending_count_++;
  }

  // if (rc == 0) {
  //  bLastIssueSendCompletedBeforeReturn = true;
  //} else {
  //  bLastIssueSendCompletedBeforeReturn = false;
  //}

  return SocketErrorCode::Ok;
}

//주의:
// FragmentedBuffer 안의 메모리 참조들이 가리키고 있는 메모리들은 전송이
// 완료되거나, 중단하는 경우까지 유지하고 있어야함. 그렇지 않을 경우, 메모리
//access violation이 발생함!!
SocketErrorCode InternalSocket::IssueSend_NoCopy(
    FragmentedBuffer& send_buffer) {
  if (SendIssued()) {
    delegate_->OnSocketWarning(this, "WARNING: IssueSend is duplicated!");
  }

  if (send_buffer_.Length() <= 0) {
    return SocketErrorCode::InvalidArgument;  // 0 이하의 수신 자체는
                                              // 무의미한데다 socket 오동작을
                                              // 유발하므로
  }

  send_ovl_.issued = true;

  DWORD error;
  DWORD out_done = 0;
  DWORD flags = 0;
  const int32 rc = WSASend(socket_, send_buffer_.buffer_.ConstData(),
                           send_buffer_.buffer_.Count(), &out_done, flags,
                           &send_ovl_, nullptr);
  if (rc < 0 && (error = WSAGetLastError()) != WSA_IO_PENDING) {
    send_ovl_.issued = false;

    PostSocketWarning(error, __FUNCTION__);
    return (SocketErrorCode)error;
  } else {
    io_pending_count_++;
    // delegate_->OnSocketWarning(this, String::Format("iopendingcount:{0}",
    // io_pending_count_));
  }

  // if (rc == 0) {
  //  bLastIssueSendCompletedBeforeReturn = true;
  //} else {
  //  bLastIssueSendCompletedBeforeReturn = false;
  //}

  return SocketErrorCode::Ok;
}

SocketErrorCode InternalSocket::IssueSendTo_NoCopy_TempTtl(
    FragmentedBuffer& send_buffer, const InetAddress& sendto, int32 ttl) {
  if (SendIssued()) {
    delegate_->OnSocketWarning(this, "WARNING: IssueSendTo is duplicated!");
  }

  const int32 total_length = send_buffer_.Length();
  if (total_length <= 0) {
    return SocketErrorCode::InvalidArgument;  // 0 이하의 수신 자체는
                                              // 무의미한데다 socket 오동작을
                                              // 유발하므로
  }

  //멀티캐스트 주소까지는 인정을 해주어야할듯함...
  // TODO 검증 후 다시 적용하도록 하자.
  // if (!(sendto.IsUnicast() || sendto.IsMulticast()))
  if (!(sendto.IsUnicast())) {
    return SocketErrorCode::AccessError;
  }

  if (ttl >= 0) {
    // 임시로 ttl을 변경한다. 복원은 send completion에서 수행하도록 하자.
    int32 OrigTtl;
    if (GetTtl(OrigTtl) == SocketErrorCode::Ok &&
        SetTTL(ttl) == SocketErrorCode::Ok) {
      ttl_to_restore_on_send_completion_ = OrigTtl;
      ttl_should_restore_on_send_completion_ = true;
    } else {
      fun_check(0);  // UNEXPECTED!
    }
  }

  send_ovl_.issued = true;

#if FUN_DISABLE_IPV6
  sockaddr_in sa;
  sendto.ToNative(sa);
#else
  sockaddr_in6 sa;
  sendto.ToNative(sa);
#endif

  DWORD error;
  DWORD out_done = 0;
  DWORD flags = 0;
  const int32 rc = WSASendTo(
      socket_, send_buffer_.buffer_.ConstData(), send_buffer_.buffer_.Count(),
      &out_done, flags, (const sockaddr*)&sa, sizeof(sa), &send_ovl_, nullptr);
  if (rc != 0 && (error = WSAGetLastError()) != WSA_IO_PENDING) {
    send_ovl_.issued = false;

    // NOTE: 10054, 10052 or WSAEMSGSIZE는 상대방에게 성공적으로 전달했으나 no
    // completion signal이므로 그냥 오류 처리해도 무방.
    PostSocketWarning(error, __FUNCTION__);
    return (SocketErrorCode)error;
  } else {
    return SocketErrorCode::Ok;
  }
}

SocketErrorCode InternalSocket::BlockedSendTo(const uint8* data, int32 length,
                                              const InetAddress& sendto_addr) {
  if (!EnsureUnicastEndpoint(sendto_addr)) {
    return SocketErrorCode::AccessError;
  }

#if FUN_DISABLE_IPV6
  sockaddr_in sa;
#else
  sockaddr_in6 sa;
#endif
  sendto_addr.ToNative(sa);

  return (SocketErrorCode)sendto(socket_, (const char*)data, length, 0,
                                 (const sockaddr*)&sa, sizeof(sa));
}

SocketErrorCode InternalSocket::BlockedSend(const uint8* data, int32 length) {
  return (SocketErrorCode)send(socket_, (const char*)data, length, 0);
}

SocketErrorCode InternalSocket::BlockedRecvFrom(uint8* data, int32 length,
                                                InetAddress& out_recvfrom) {
#if FUN_DISABLE_IPV6
  sockaddr_in sa;
  int32 sa_len = sizeof(sa);
#else
  sockaddr_in6 sa;
  int32 sa_len = sizeof(sa);
#endif

  // TODO 오류처리를 해야하나??
  const SocketErrorCode rc = (SocketErrorCode)recvfrom(
      socket_, (char*)data, length, 0, (sockaddr*)&sa, &sa_len);
  out_recvfrom = InetAddress(sa);

  return rc;
}

SocketErrorCode InternalSocket::IssueSendTo(const uint8* data, int32 length,
                                            const InetAddress& sendto_addr) {
  if (SendIssued()) {
    // 경고만 낼 뿐 무시 처리는 안한다. 이건 검사 목적일 뿐이고 정확하지
    // 않으니까.
    delegate_->OnSocketWarning(this, "WARNING: IssueSendTo is duplicated!");
  }

  if (length <= 0) {
    return SocketErrorCode::InvalidArgument;  // 0 이하의 수신 자체는
                                              // 무의미한데다 socket 오동작을
                                              // 유발하므로
  }

  send_buffer_.ResizeUninitialized(length);
  UnsafeMemory::Memcpy(send_buffer_.MutableData(), data, length);

  WSABUF buf;
  buf.buf = (char*)send_buffer_.MutableData();
  buf.len = length;

  //멀티캐스트 주소까지는 인정을 해주어야할듯함...
  if (!(sendto_addr.IsUnicast() || sendto_addr.IsMulticast())) {
    return SocketErrorCode::AccessError;
  }

  send_ovl_.issued = true;

#if FUN_DISABLE_IPV6
  sockaddr_in sa;
  sendto_addr.ToNative(sa);
#else
  sockaddr_in6 sa;
  sendto_addr.ToNative(sa);
#endif

  DWORD error;
  DWORD out_done = 0;
  DWORD flags = 0;
  const int32 rc = WSASendTo(socket_, &buf, 1, &out_done, flags, (SOCKADDR*)&sa,
                             sizeof(sa), &send_ovl_, nullptr);
  if (rc != 0 && (error = WSAGetLastError()) != WSA_IO_PENDING) {
    send_ovl_.issued = false;

    PostSocketWarning(error, __FUNCTION__);
    // NOTE: 10054, 10052 or WSAEMSGSIZE는 상대방에게 성공적으로 전달했으나 no
    // completion signal이므로 그냥 오류 처리해도 무방.
    return (SocketErrorCode)error;
  } else {
    return SocketErrorCode::Ok;
  }
}

void InternalSocket::InitOthers() {
  // CSocketInitializer::Get();

  ignore_not_socket_error_ = false;

  // RestoredCount = 0;

  associated_iocp_ = nullptr;
  {
    CScopedLock2 Lock(socket_closed_mutex_);
    socket_closed_or_closing_mutex_protected_ = false;
  }

  // bLeakPinnedInternal_TEST = false;

  verbose_ = true;

  io_pending_count_ = 0;

  completion_context_ = nullptr;

  accept_ex_length_ = 0;

#if FUN_DISABLE_IPV6
  InetAddress::None.ToNative(recvfrom_addr_);
#else
  InetAddress::None.ToNative(recvfrom_addr_);
#endif

  UnsafeMemory::Memset(&sendto_addr_, 0x00, sizeof(sendto_addr_));
  sendto_addr_len_ = 0;

  recv_flags_ = 0;

  ttl_to_restore_on_send_completion_ = -1;

  ttl_should_restore_on_send_completion_ = false;

  connect_ex_complete_ = false;
}

// async issue의 결과를 기다린다.
// 아직 아무것도 완료되지 않았으면 null을 리턴한다.
// 만약 완료 성공 또는 소켓 에러 등의 실패가 생기면 객체를 리턴하되 ErrorCode가
// 채워져 있다. 수신 완료시 데이터를 가져오는 방법: GetRecvBuffer()를 호출하되
// 파라메터로 가져올 데이터 크기를 넣어줌 된다.
bool InternalSocket::GetRecvOverlappedResult(
    bool wait_until_complete, OverlappedResult& out_overlapped_result) {
  // WSAGetOverlappedResult를 호출 전에 이걸 검증하는 것이 FASTER!
  if (!recv_ovl_.issued || !HasOverlappedIoCompleted(&recv_ovl_)) {
    return false;
  }

  /*
  // 타임아웃을 지정할 수 있도록 해볼까나?
  // 이때 이벤트를 기다리는게 좋을듯 싶은데..
  if (TimeoutMSec > 0) {
    if (WaitForSingleObject(recv_ovl_.hEvent, TimeoutMSec) != WAIT_OBJECT_0) {
      return false;
    }
  }
  */

  DWORD done = 0;
  DWORD flags = 0;
  const BOOL result = WSAGetOverlappedResult(socket_, &recv_ovl_, &done,
                                             wait_until_complete, &flags);
  if (!result && WSAGetLastError() == WSA_IO_INCOMPLETE) {
    return false;
  }

  // 이런 경우가 없어야 하는데 테스트 로비 클라에서 이런 경우가 있는듯
  // fun_check(!(result && WSAGetLastError() == WSA_IO_INCOMPLETE));

  out_overlapped_result.received_flags = flags;
  out_overlapped_result.completed_length = done;
  if (!result) {
    out_overlapped_result.socket_error = (SocketErrorCode)WSAGetLastError();
  }

  recv_ovl_.issued = false;

  // 수신받은 주소 저장.
  out_overlapped_result.received_from = InetAddress(recvfrom_addr_);

  return true;
}

bool InternalSocket::GetAcceptExOverlappedResult(
    bool wait_until_complete, OverlappedResult& out_overlapped_result) {
  if (!accept_ex_ovl_.issued || !HasOverlappedIoCompleted(&accept_ex_ovl_)) {
    return false;
  }

  DWORD done = 0;
  DWORD flags = 0;
  const BOOL result = WSAGetOverlappedResult(socket_, &accept_ex_ovl_, &Done,
                                             wait_until_complete, &flags);
  if (!result && WSAGetLastError() == WSA_IO_INCOMPLETE) {
    return false;
  }

  // fun_check(!(result && WSAGetLastError() == WSA_IO_INCOMPLETE)); // 이런
  // 경우가 없어야 하는데 테스트 로비 클라에서 이런 경우가 있는듯

  out_overlapped_result.received_flags = flags;
  out_overlapped_result.completed_length = done;
  if (!result) {
    out_overlapped_result.socket_error = (SocketErrorCode)WSAGetLastError();
  }

  // 이제 처리했으니 청소해버린다.
  accept_ex_ovl_.issued = false;

  // 접속한 주소 저장.
  out_overlapped_result.received_from = InetAddress(recvfrom_addr_);

  return true;
}

bool InternalSocket::GetSendOverlappedResult(
    bool wait_until_complete, OverlappedResult& out_overlapped_result) {
  // WSAGetOverlappedResult를 호출 전에 이걸 검증하는 것이 FASTER!
  if (!HasOverlappedIoCompleted(&send_ovl_) || !send_ovl_.issued) {
    return false;
  }

  DWORD done = 0;
  DWORD flags = 0;
  const BOOL result = WSAGetOverlappedResult(socket_, &send_ovl_, &done,
                                             wait_until_complete, &flags);
  if (!result) {
    const DWORD error = WSAGetLastError();
    if (error == WSA_IO_INCOMPLETE) {
      return false;
    }
  }

  // 실험해본 결과 IssueSend 자체를 안한 상태에서 이 조건이 온다.
  // 이때는 아직 안보내졌다고 뻥쳐야 한다.
  if (result && !done && !flags) {
    return false;
  }

  out_overlapped_result.received_flags = flags;
  out_overlapped_result.completed_length = done;

  if (!result) {
    out_overlapped_result.socket_error = (SocketErrorCode)WSAGetLastError();
  }

  send_ovl_.issued = false;

  return true;
}

void InternalSocket::Listen() {
  SetIPv6Only(false);

  // AcceptEx를 사용할 경우에는 아래를 해주어야한다함.
  // http://symlink.tistory.com/52
  {
    BOOL on = TRUE;
    setsockopt(socket_, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char*)&on,
               sizeof(on));
  }

  if (listen(socket_, SOMAXCONN) != 0) {
    PostSocketWarning(WSAGetLastError(), __FUNCTION__);
  }
}

InetAddress InternalSocket::GetSockName() {
  if (socket_ == INVALID_SOCKET) {
    return InetAddress::None;
  }

#if FUN_DISABLE_IPV6
  sockaddr_in sa;
#else
  sockaddr_in6 sa;
#endif
  fun_socklen_t sa_length = sizeof(sa);

  const int32 rc = getsockname(socket_, (sockaddr*)&sa, &sa_length);
  if (rc == 0) {
    // TODO 길이체크 해야하남???
    const InetAddress SockName = InetAddress(sa);
    return SockName;
  } else {
    // ThrowError();
    return InetAddress::None;
  }
}

InetAddress InternalSocket::GetPeerName() {
  if (socket_ == INVALID_SOCKET) {
    return InetAddress::None;
  }

#if FUN_DISABLE_IPV6
  sockaddr_in sa;
#else
  sockaddr_in6 sa;
#endif
  fun_socklen_t sa_length = sizeof(sa);
  const int32 rc = getpeername(socket_, (sockaddr*)&sa, &sa_length);
  if (rc == 0) {
    // TODO 길이체크 해야하남???
    return InetAddress(sa);
  } else {
    // ThrowError();
    return InetAddress::None;
  }
}

int32 InternalSocket::SetSendBufferSize(int32 size) {
  int32 value = size;
  const int32 rc = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
                              (const char*)&value, sizeof(value));
  if (rc != 0) {
    PostSocketWarning(WSAGetLastError(), __FUNCTION__);
  }

  return rc;
}

int32 InternalSocket::SetRecvBufferSize(int32 size) {
  int32 value = size;
  const int32 rc = setsockopt(socket_, SOL_SOCKET, SO_RCVBUF,
                              (const char*)&value, sizeof(value));
  if (rc != 0) {
    PostSocketWarning(WSAGetLastError(), __FUNCTION__);
  }

  return rc;
}

void InternalSocket::CloseSocketHandleOnly() {
  CScopedLock2 socket_closed_guard(socket_closed_mutex_, false);
  socket_closed_guard.UnsafeLock();  // 드물지만 파괴자에서 잘못된 CS를 접근하는
                                     // 경우 ShowUserMisuseError는 쐣

  if (!socket_closed_or_closing_mutex_protected_) {
    socket_closed_or_closing_mutex_protected_ = true;  // 이게 위로 오게 해야!

    NetUtil::AssertCloseSocketWillReturnImmediately(socket_);
    closesocket(socket_);  // socket handle 값은 그대로 둔다.

    // socket을 닫으면 Win32는 IOCP와의 연계도 없앤다. 따라서 이 성향을
    // 따라가도록 한다.
    if (associated_iocp_) {
      // fake IOCP에 연계중이고 issue가 된 상태이면 completion with error를
      // 흉내낸다.
      if (!associated_iocp_->RealIocpEnabled()) {
        if (RecvIssued()) {
          recv_ovl_.Internal =
              STATUS_ABANDONED_WAIT_0;  // 하여튼 STATUS_PENDING만 아니면 됨
          associated_iocp_->PostSocketCloseEvent_INTERNAL(
              this, CompletionType::Receive);
        }

        if (SendIssued()) {
          send_ovl_.Internal =
              STATUS_ABANDONED_WAIT_0;  // 하여튼 STATUS_PENDING만 아니면 됨
          associated_iocp_->PostSocketCloseEvent_INTERNAL(this,
                                                          CompletionType::Send);
        }
      }

      associated_iocp_->UnassociateSocket_FakeMode(this);
      associated_iocp_ = nullptr;
    }
  }
}

bool InternalSocket::GetVerboseFlag() { return verbose_; }

void InternalSocket::SetVerboseFlag(bool flag) { verbose_ = flag; }

InternalSocket::~InternalSocket() {
  // LOG(LogNetEngine, Error, "%s", __FUNCTION__);

  CloseSocketHandleOnly();
}

SocketErrorCode InternalSocket::Shutdown(ShutdownFlag how) {
  if (shutdown(socket_, (int)how) == 0) {
    return SocketErrorCode::Ok;
  } else {
    return (SocketErrorCode)WSAGetLastError();
  }
}

void InternalSocket::DirectBindAcceptEx() {
  GUID GuidAcceptEx = WSAID_ACCEPTEX;
  GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
  DWORD dwBytes;

  //----------------------------------------
  // Load the AcceptEx function into memory using WSAIoctl.
  // The WSAIoctl function is an extension of the ioctlsocket()
  // function that can use overlapped I/O. The function's 3rd
  // through 6th parameters are input and output buffers where
  // we pass the pointer to our AcceptEx function. This is used
  // so that we can call the AcceptEx function directly, rather
  // than refer to the Mswsock.lib library.
  WSAIoctl(socket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx,
           sizeof(GuidAcceptEx), (void*)&lpfnAcceptEx, sizeof(lpfnAcceptEx),
           &dwBytes, nullptr, nullptr);

  ////----------------------------------------
  //// Load the AcceptEx function into memory using WSAIoctl.
  //// The WSAIoctl function is an extension of the ioctlsocket()
  //// function that can use overlapped I/O. The function's 3rd
  //// through 6th parameters are input and output buffers where
  //// we pass the pointer to our AcceptEx function. This is used
  //// so that we can call the AcceptEx function directly, rather
  //// than refer to the Mswsock.lib library.
  // WSAIoctl(
  //    socket_,
  //    SIO_GET_EXTENSION_FUNCTION_POINTER,
  //    &GuidGetAcceptExSockAddrs,
  //    sizeof(GuidGetAcceptExSockAddrs),
  //    &lpfnGetAcceptExSockAddrs,
  //    sizeof(lpfnGetAcceptExSockAddrs),
  //    &dwBytes,
  //    nullptr,
  //    nullptr);
}

// http://symlink.tistory.com/52
SocketErrorCode InternalSocket::AcceptEx(InternalSocket* exisiting_socket) {
  if (AcceptExIssued()) {
    delegate_->OnSocketWarning(this, "WARNING: AcceptEx is duplicated!");
  }

  if (lpfnAcceptEx == nullptr) {
    DirectBindAcceptEx();
  }

  accept_ex_buffer_.ResizeUninitialized(AddrLengthInTdi * 2);

  accept_ex_ovl_.issued = true;

  const BOOL result =
      lpfnAcceptEx(socket_,                    // SOCKET       sListenSocket
                   exisiting_socket->socket_,  // SOCKET       sAcceptSocket
                   accept_ex_buffer_.MutableData(),  // PVOID lpOutputBuffer
                   0,                   // DWORD        dwReceiveDataLength
                   AddrLengthInTdi,     // DWORD        dwLocalAddressLength
                   AddrLengthInTdi,     // DWORD        dwRemoteAddressLength
                   &accept_ex_length_,  // LPDWORD      lpdwBytesReceived
                   &accept_ex_ovl_      // LPOVERLAPPED lpOverlapped
      );
  if (!result) {
    const DWORD error = WSAGetLastError();

    // AcceptEx가 비동기를 전재로한 함수이므로, pending이면 OK임.
    if (InternalSocket::IsPendingErrorCode((SocketErrorCode)error)) {
      return SocketErrorCode::Ok;
    }

    // NotSocket 오류는 어떤 경우에 나오는거지??
    // 1. 소켓 디스크립터가 초기화되지 않은경우.
    // 2. 닫은 소켓을 다시 사용하려 할 경우.

    if (ignore_not_socket_error_ &&
        (SocketErrorCode)error == SocketErrorCode::NotSocket) {
      return SocketErrorCode::Ok;
    }

    accept_ex_ovl_.issued = false;

    PostSocketWarning(error, __FUNCTION__);
    return (SocketErrorCode)error;
  }

  return SocketErrorCode::Ok;
}

#if _WIN32_WINNT >= 0x0501  // connectex 0x0501이상에서만 사용가능.
void InternalSocket::DirectBindConnectEx() {
  GUID GuidConnectEx = WSAID_CONNECTEX;
  DWORD dwBytes = 0;

  WSAIoctl(socket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx,
           sizeof(GuidConnectEx), (void*)&lpfnConnectEx, sizeof(lpfnConnectEx),
           &dwBytes, nullptr, nullptr);
}

SocketErrorCode InternalSocket::ConnectEx(const String& host, int32 port) {
  ////TODO InetAddress 클래스 자체에 resolving 기능이 추가되어 있으므로, 아래
  ///코드는 불필요함. /만약, InetAddress 주소가 잘못된 경우에는 예외가 던져지는
  ///형태인데 이걸 어떤 형태로 핸들링하는게 좋으려나??
  //
  // ByteArray HostAddrA = CT2A(host);
  //
  // InetAddress AddrPort;
  // AddrPort.BinaryAddress = inet_addr(HostAddrA);
  // if (AddrPort.BinaryAddress == INADDR_NONE) {
  //  if (LPHOSTENT Host = gethostbyname(HostAddrA))
  //  {
  //    AddrPort.BinaryAddress = ((LPIN_ADDR)Host->h_addr)->s_addr;
  //  } else {
  //    const DWORD error = WSAGetLastError();
  //    PostSocketWarning(error, __FUNCTION__);
  //    return (SocketErrorCode)error;
  //  }
  //}
  //
  // AddrPort.Port = port;//htons((u_short)port);
  //
  ////@maxidea: debug temp.
  ////_tprintf("InternalSocket::ConnectEx: ServerIP=%s\n",
  ///*AddrPort.ToString());
  //
  // return ConnectEx(AddrPort);

  try {
    InetAddress addr(host, port);
    return ConnectEx(addr);
  } catch (Exception&) {
    // TODO logging???
    //뭐라고 해야하나???
    // TODO
    return SocketErrorCode::ConnectionRefused;
  }
}

SocketErrorCode InternalSocket::ConnectEx(const InetAddress& addr) {
  if (ConnectExIssued()) {
    delegate_->OnSocketWarning(this, "WARNING: ConnectEX is duplicated!");
  }

  if (lpfnConnectEx == nullptr) {
    DirectBindConnectEx();
  }

  // 이미 바인드 되어있는 상태여야 한다.
  // ConnectEx는 자동바운드(포트=0)이어서는 안됨.
  if (!IsBoundSocket()) {
    delegate_->OnSocketWarning(
        this, "WARNING: ConnectEX socket Not Bind! Auto Bind!");

    Bind();
  }

  connect_ex_ovl_.issued = true;

  // associate를 먼저해야 하겠다.
  if (associated_iocp_ == nullptr) {
    // TODO OnSocketWarning이라는것 자체가 별 필요없어 보이는데...??
    delegate_->OnSocketWarning(this, "WARNING: ConnectEX Associate IOCP None!");
  }

#if FUN_DISABLE_IPV6
  sockaddr_in sa;
  addr.ToNative(sa);
#else
  sockaddr_in6 sa;
  addr.ToNative(sa);
#endif

  const BOOL result = lpfnConnectEx(socket_, (struct sockaddr*)&sa, sizeof(sa),
                                    nullptr, 0, nullptr, &connect_ex_ovl_);
  if (!result) {
    const DWORD error = WSAGetLastError();
    if (error != WSA_IO_PENDING) {
      return (SocketErrorCode)error;  //@maxidea: 정확히 WSAGetLastError()의
                                      //반환값과 ESocketError이 1:1 대응할까??
    }
  }

  return SocketErrorCode::Ok;
}

SocketErrorCode InternalSocket::ConnectExComplete() {
  connect_ex_complete_ = true;

  if (setsockopt(socket_, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) <
      0) {
    return (SocketErrorCode)GetLastError();
  }

  return SocketErrorCode::Ok;
}

//이함수는 폴로대체하는건 어떨런지?
bool InternalSocket::GetConnectExOverlappedResult(
    bool wait_until_complete, OverlappedResult& out_overlapped_result) {
  if (!connect_ex_ovl_.issued || !HasOverlappedIoCompleted(&connect_ex_ovl_)) {
    return false;
  }

  DWORD done = 0;
  DWORD flags = 0;
  const BOOL result = WSAGetOverlappedResult(socket_, &connect_ex_ovl_, &done,
                                             wait_until_complete, &Flags);
  if (!result && WSAGetLastError() == WSA_IO_INCOMPLETE) {
    return false;
  }

  // fun_check(!(b && WSAGetLastError()==WSA_IO_INCOMPLETE)); // 이런 경우가
  // 없어야 하는데 테스트 로비 클라에서 이런 경우가 있는듯

  out_overlapped_result.received_flags = flags;
  out_overlapped_result.completed_length = done;

  if (!result) {
    out_overlapped_result.socket_error = (SocketErrorCode)WSAGetLastError();
  }

  connect_ex_ovl_.issued = false;

  // recvfrom_addr_
  out_overlapped_result.received_from =
      InetAddress(recvfrom_addr_);  // 불필요할듯

  return true;
}
#endif  // _WIN32_WINNT >= 0x0501

bool InternalSocket::IsBoundSocket() {
  const InetAddress sock_name = GetSockName();
  return (sock_name.GetPort() >= 1 && sock_name.GetPort() <= 65534);
}

void InternalSocket::FinalizeAcceptEx(InternalSocket* tcp_listening_socket,
                                      InetAddress& local_addr,
                                      InetAddress& remote_addr) {
  if (setsockopt(socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                 (char*)&tcp_listening_socket_->socket_,
                 sizeof(tcp_listening_socket_->socket_)) == SOCKET_ERROR) {
    // sink error.
    WSAGetLastError();
  }

  // TODO IPv6로 받아주어야할듯 싶은데...
  // sockaddr_in LocalSockAddr, RemoteSockAddr;
  // int32 LocalSockAddrLen = sizeof(LocalSockAddr);
  // int32 RemoteSockAddrLen = sizeof(RemoteSockAddr);

  sockaddr* local_sock_addr = nullptr;
  int32 local_sock_addr_len = 0;  // InetAddress::MIN_ADDRESS_LENGTH;
  sockaddr* remote_sock_addr = nullptr;
  int32 remote_sock_addr_len = 0;  // InetAddress::MIN_ADDRESS_LENGTH;

  fun_check(tcp_listening_socket_->accept_ex_buffer_.Count() > 0);

  GetAcceptExSockaddrs(tcp_listening_socket_->accept_ex_buffer_
                           .MutableData(),  // PVOID       lpOutputBuffer
                       0,                   // DWORD       dwReceiveDataLength
                       AddrLengthInTdi,     // DWORD       dwLocalAddressLength
                       AddrLengthInTdi,     // DWORD       dwRemoteAddressLength
                       &local_sock_addr,    // LPSOCKADDR* LocalSockaddr
                       &local_sock_addr_len,  // LPINT       LocalSockaddrLength
                       &remote_sock_addr,     // LPSOCKADDR* RemoteSockaddr
                       &remote_sock_addr_len  // LPINT RemoteSockaddrLength
  );

  // FIXME 주소값이 왜 NULL인게냐!!

  // fun_check(LocalSockAddr  != nullptr);
  // fun_check(RemoteSockAddr != nullptr);

  // local_addr  = InetAddress(LocalSockAddr, LocalSockAddrLen);
  // remote_addr = InetAddress(RemoteSockAddr, RemoteSockAddrLen);

  //@todo 기껏 위에서 구해놓고, 이리하는 이유는 뭘까?? 아예 안하면 되지 않나??
  // 이걸로 하자. GetAcceptExSockaddrs가 잘 작동 안하지만, 속도 희생해서라도,
  // 어쩔 수 없다. GetAcceptExSockaddrs가 제대로 작동을 안하는건가??
  // -> 인자 타입이 잘못 넘어갔었음.. 값이 아닌 포인터로 넘겨야 하는데, 주소를
  // 받아오는곳에 값으로 넘겼었더라는...

  // NOTE 만약, 주소가 제대로 안나온다면 아래 두 라인을 다시 활성화 해야함..

  //가끔씩 주소가 제대로 인식이 안되는데,.. 어떤 상황인지??
  //접속을 받는 과정중에, 원격지에서 접속을 끊을 경우 주소를 얻지 못하는 경우도
  //있음. 그러므로, 여기에 assertion을 걸 필요는 없어보임.
  local_addr = GetSockName();
  remote_addr = GetPeerName();

  // fun_check(local_addr.IsUnicast());
  // fun_check(remote_addr.IsUnicast());
}

#ifdef USE_DisableUdpConnResetReport

// 본 기능은 더 이상 쓰지 않는다.
// 수신자 사이의 라우터가 ICMP port unreachable 발생시
//
// UDP socket으로 sendto를 했는데 상대 IP는 유효하나 port가 없으면 ICMP port
// unreachable이 도착하며 localhost에서 WSAECONNRESET이 도착한다. 여기까지는
// 그렇다 치자. WSAECONNRESET 도착 후 송수신이 되지 못하는 문제가 발견됐다! 이것
// 때문에 UDP SOCKET이 ICMP 거시기를 받으면 멀쩡한 타 PEER와의 통신까지 먹통이
// 되는 문제가 있다. 따라서 아예 ICMP 오류 리포팅 자체를 막아버리자.
void InternalSocket::DisableUdpConnResetReport() {
  DWORD bytes_returned = 0;
  BOOL new_behaviour = FALSE;

  // disable  new behavior using
  // IOCTL: SIO_UDP_CONNRESET
  const DWORD status = WSAIoctl(socket_, SIO_UDP_CONNRESET, &new_behaviour,
                                sizeof(new_behaviour), nullptr, 0,
                                &bytes_returned, nullptr, nullptr);
  if (status == SOCKET_ERROR) {
    DWORD err = WSAGetLastError();
    if (WSAEWOULDBLOCK == err) {
      //// nothing to do
      // return(FALSE);
    } else {
      // printf("WSAIoctl(SIO_UDP_CONNRESET) Error: %d\n", err);
      // return(FALSE);
    }
  }
}
#endif  // USE_DisableUdpConnResetReport

void InternalSocket::SetBlockingMode(bool flag) {
  u_long value = flag ? 0 : 1;
  ioctlsocket(socket_, FIONBIO, &value);
}

uint8* InternalSocket::GetRecvBufferPtr() { return recv_buffer_.MutableData(); }

SocketErrorCode InternalSocket::EnableBroadcastOption(bool flag) {
  // IPv6라면 동작하지 않을터....??

  u_long value = flag ? 1 : 0;
  const int32 rc = setsockopt(socket_, SOL_SOCKET, SO_BROADCAST,
                              (const char*)&value, sizeof(value));
  if (rc != 0) {
    const DWORD err = WSAGetLastError();
    PostSocketWarning(err, __FUNCTION__);
    return (SocketErrorCode)err;
  }

  broadcast_option_enabled_ = flag;

  return SocketErrorCode::Ok;
}

SocketErrorCode InternalSocket::SetTTL(int32 ttl) {
  int32 value;
  int32 value_len = sizeof(value);

  int32 rc;
  if ((rc = getsockopt(socket_, IPPROTO_IP, IP_TTL, (char*)&value,
                       &value_len)) != 0) {
    delegate_->OnSocketWarning(this, "This socket doesn't support ttl change!");
    return (SocketErrorCode)rc;
  } else {
    value = ttl;
    if ((rc = setsockopt(socket_, IPPROTO_IP, IP_TTL, (const char*)&value,
                         value_len)) != 0) {
      delegate_->OnSocketWarning(this, "Cannot change ttl!");
    }
    return (SocketErrorCode)rc;
  }
}

bool InternalSocket::IsUdpStopErrorCode(SocketErrorCode socket_error) {
  // 주의: SocketErrorCode::ConnectResetByRemote 는 ICMP connreset에서 오는건데
  // 이건 계속 UDP 통신을 할 수 있는 조건이므로 false이어야 함!
  return socket_error == SocketErrorCode::NotSocket ||
         socket_error == SocketErrorCode::OperationAborted;
}

bool InternalSocket::IsTcpStopErrorCode(SocketErrorCode socket_error) {
  return socket_error != SocketErrorCode::Ok;
}

bool InternalSocket::IsPendingErrorCode(SocketErrorCode socket_error) {
  return socket_error == SocketErrorCode::WouldBlock ||
         socket_error == SocketErrorCode::IoPending;
}

SocketErrorCode InternalSocket::AllowPacketFragmentation(bool allow) {
  int32 value = allow ? 0 : 1;

  if (setsockopt(socket_, IPPROTO_IP, IP_DONTFRAGMENT, (char*)&value,
                 sizeof(value)) != 0) {
    return (SocketErrorCode)WSAGetLastError();
  } else {
    return SocketErrorCode::Ok;
  }
}

bool InternalSocket::IsClosed() {
  // 드문 케이스 땜시 부득이
  if (!socket_closed_mutex_.IsValid()) {
    return true;
  } else {
    CScopedLock2 socket_closed_guard(socket_closed_mutex_);
    return socket_closed_or_closing_mutex_protected_;
  }
}

void InternalSocket::EnableNagleAlgorithm(bool enable) {
  u_long value = enable ? 0 : 1;
  const int32 rc = setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char*)&value,
                              sizeof(value));
  if (rc == SOCKET_ERROR) {
    // LOG(LogNetEngine, Warning, "%s: set tcp-nagle option failure: error=%d",
    // __FILE__, WSAGetLastError());
    PostSocketWarning(WSAGetLastError(), __FUNCTION__);
  }
}

// TODO 실질적으로 사용안되고 있음. BlockedSendTo 함수에서 사용하고는 있으나,
// 정작 이 함수 자체가 사용안되고 있음. 상황 봐서 제거하던지, 소켓 유틸쪽으로
//빼주던지 하자. UDP로 255.255.255.255.65535로 쏘면 블럭이 되는 현상때문에
// 그러한데... 흠.. 브로드 캐스팅 할 경우에는 239.255.255.250으로 해야하나??
//UPnP처럼??
bool InternalSocket::EnsureUnicastEndpoint(const InetAddress& sendto_addr) {
  // if (sendto_addr.BinaryAddress == 0xFFFFFFFF) { // OS에 의해 차단되지
  // 않으려면 필수
  //  delegate_->OnSocketWarning(this, "Sending to 255.255.255.255 is not
  //  permitted!"); return false;
  //} else if (sendto_addr.Port == 0xFFFF || sendto_addr.Port == 0) { // OS에
  //의해 차단되지 않으려면 필수
  //  delegate_->OnSocketWarning(this, "Sending to prohibited port is not
  //  permitted!"); return false;
  //} else if (sendto_addr.BinaryAddress == 0) { // OS에 의해 차단되지 않으려면
  //필수
  //  delegate_->OnSocketWarning(this, "Sending to 0.0.0.0 is not permitted!");
  //  return false;
  //}

  // NOTE: 255.255.255.255:65535로 쏘면 Win32는 막아버린다. 굳이 보내려면
  // 239.255.255.250으로 브로드캐스트하덩가.
  // TODO 일단 막고, 정리하면서 검토.
  if (!(sendto_addr.IsUnicast() || sendto_addr.IsMulticast())) {
    delegate_->OnSocketWarning(this,
                               String::Format("{0} endpoint is not permitted.",
                                              *sendto_addr.ToString()));
    return false;
  }

  return true;
}

int32 InternalSocket::GetSendBufferSize(int32& out_size) {
  int32 out_size_len = sizeof(out_size);
  const int32 rc = getsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (char*)&out_size,
                              &out_size_len);
  if (rc != 0) {
    PostSocketWarning(WSAGetLastError(), __FUNCTION__);
  }

  return rc;
}

int32 InternalSocket::GetRecvBufferSize(int32& out_size) {
  int32 out_size_len = sizeof(out_size);
  const int32 rc = getsockopt(socket_, SOL_SOCKET, SO_RCVBUF, (char*)&out_size,
                              &out_size_len);
  if (rc != 0) {
    PostSocketWarning(WSAGetLastError(), __FUNCTION__);
  }

  return rc;
}

SocketErrorCode InternalSocket::GetTtl(int32& out_ttl) {
  int32 out_ttl_len = sizeof(out_ttl);
  int32 rc;
  if ((rc = getsockopt(socket_, IPPROTO_IP, IP_TTL, (char*)&out_ttl,
                       &out_ttl_len)) != 0) {
    delegate_->OnSocketWarning(this, "This socket doesn't support ttl change!");
  }

  return (SocketErrorCode)rc;
}

void InternalSocket::RestoreTtlOnCompletion() {
  if (ttl_should_restore_on_send_completion_) {
    ttl_should_restore_on_send_completion_ = false;

    SetTTL(ttl_to_restore_on_send_completion_);
  }
}

void InternalSocket::OnIoCompletion(Array<IHostObject*>& send_issued_pool,
                                    ReceivedMessageList& msg_list,
                                    CompletionStatus& completion) {
  io_completion_delegate_->OnSocketIoCompletion(send_issued_pool, msg_list,
                                                completion);
}

void InternalSocket::SetIPv6Only(SOCKET socket, bool flag) {
#if !FUN_DISABLE_IPV6
  int32 value = flag ? 1 : 0;
  const int32 rc =
      setsockopt(socket_, IPPROTO_IPV6, IPV6_V6ONLY,
                 reinterpret_cast<const char*>(&value), sizeof(value));
  if (rc == -1) {
    // delegate_->OnSocketWarning(this, "This socket doesn't support IPV6_V6ONLY
    // change!");
  }
#endif
}

void InternalSocket::SetIPv6Only(bool flag) { SetIPv6Only(socket_, flag); }

//
// IHasOverlappedIo
//

IHasOverlappedIo::IHasOverlappedIo()
    : send_issued_(false), recv_issued_(false) {}

// TEMP 어느정도만 사용되다가 폐기될 예정임.
//
// InternalSocketSelectContext
//

InternalSocketSelectContext::InternalSocketSelectContext() {
  FD_ZERO(&read_socket_list_);
  FD_ZERO(&write_socket_list_);
  FD_ZERO(&error_socket_list_);
}

void InternalSocketSelectContext::AddWriteWaiter(InternalSocket& socket) {
  FD_SET(socket.socket_, &write_socket_list_);
}

void InternalSocketSelectContext::AddExceptionWaiter(InternalSocket& socket) {
  FD_SET(socket.socket_, &error_socket_list_);
}

void InternalSocketSelectContext::Wait(uint32 timeout_msec) {
  if (timeout_msec != INFINITE) {
    timeval tv;
    tv.tv_sec = timeout_msec / 1000;
    tv.tv_usec = (timeout_msec % 1000) * 1000;
    select(0, &read_socket_list_, &write_socket_list_, &error_socket_list_,
           &tv);
  } else {
    // Infinite
    select(0, &read_socket_list_, &write_socket_list_, &error_socket_list_,
           nullptr);
  }
}

bool InternalSocketSelectContext::GetConnectResult(InternalSocket& socket,
                                                   SocketErrorCode& out_code) {
  if (FD_ISSET(socket.socket_, &write_socket_list_) != 0) {
    out_code = SocketErrorCode::Ok;
    return true;
  }

  if (FD_ISSET(socket.socket_, &error_socket_list_) != 0) {
    int32 value = 0;
    int32 value_len = sizeof(value);
    getsockopt(socket.socket_, SOL_SOCKET, SO_ERROR, (char*)&value, &value_len);
    out_code = (SocketErrorCode)value;
    return true;
  }

  return false;
}

}  // namespace fun
}  // namespace fun
