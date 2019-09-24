#include "fun/net/net.h"

#include "ReportError.h"

namespace fun {
namespace net {

CCriticalSection2 NetUtil::CachedLocalAddressesCS;
Array<IpAddress> NetUtil::CachedLocalAddresses;

Array<IpAddress>& NetUtil::LocalAddresses() {
  CScopedLock2 Lock(CachedLocalAddressesCS);

  if (CachedLocalAddresses.IsEmpty()) {
    try {
      //주의 : Windows의 경우 아래의 함수가 호출되기전에 윈속이 초기화되어
      //있어야함.

      // TODO warning: 참조로 가져오면 문제가 발생함. 왜일까?? 나중에 분석을 좀
      // 해봐야할듯... const auto& Addresses = Dns::ThisHost().Addresses();

      const auto Addresses = Dns::ThisHost().Addresses();
      for (const auto& Address : Addresses) {
        CachedLocalAddresses.Add(Address);
      }
    } catch (Exception&) {
    }

    // 정 없으면, 로컬주소라도 하나 넣자.
    if (CachedLocalAddresses.IsEmpty()) {
      CachedLocalAddresses.Add(IpAddress("127.0.0.1"));
    }
  }

  return CachedLocalAddresses;
}

const IpAddress& NetUtil::LocalAddressAt(int32 Index) {
  // TODO 예외 처리를 하는게 좋으려나??
  return LocalAddresses()[Index];  // bounds checking no required at external.
}

void NetUtil::AssertCloseSocketWillReturnImmediately(SOCKET socket) {
  LINGER Value;
  int ValueLen = sizeof(Value);
  if (getsockopt(socket, SOL_SOCKET, SO_LINGER, (char*)&Value, &ValueLen) ==
      0) {
    if (Value.l_onoff && Value.l_linger) {
      ErrorReporter::Report(
          "FATAL: socket which has behavior of some waits in closesocket() has "
          "been detected.");
    }
  }
}

// TODO 클라이언트냐 서버냐에 따라서 달리 설정할 수 있도록 해보자.
void NetUtil::SetTcpDefaultBehavior(InternalSocket* socket) {
  socket_->SetRecvBufferSize(NetConfig::tcp_issue_recv_length);
  socket_->SetSendBufferSize(NetConfig::tcp_send_buffer_length);

  if (NetConfig::socket_tcp_keep_alive_option_enabled) {
    int32 Value = 1;
    setsockopt(socket_->socket_, SOL_SOCKET, SO_KEEPALIVE, (char*)&Value,
               sizeof(Value));
  }
}

// TODO 클라이언트냐 서버냐에 따라서 달리 설정할 수 있도록 해보자.
void NetUtil::SetUdpDefaultBehavior(InternalSocket* socket) {
  socket_->SetRecvBufferSize(NetConfig::udp_issue_recv_length);
  socket_->SetSendBufferSize(NetConfig::udp_send_buffer_length);
}

bool NetUtil::IsSameSubnet24(const IpAddress& A, const IpAddress& B) {
  uint32 IPv4PackedAddressA;
  uint32 IPv4PackedAddressB;

  if (!A.GetIPv4Address(IPv4PackedAddressA)) {
    return false;
  }

  if (!B.GetIPv4Address(IPv4PackedAddressB)) {
    return false;
  }

  // Y.Y.Y.?
  // 192.168.0.1
  // 192.168.0.200
  const uint8* RawA = (const uint8*)&IPv4PackedAddressA;
  const uint8* RawB = (const uint8*)&IPv4PackedAddressB;
  return RawA[3] == RawB[3] && RawA[2] == RawB[2] && RawA[1] == RawB[1];
}

}  // namespace net
}  // namespace fun
