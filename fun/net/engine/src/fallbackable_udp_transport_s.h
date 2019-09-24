#pragma once

#include "UdpSocket_S.h"

namespace fun {
namespace net {

class TcpTransport_S;
class UdpSocket_S;
class RemoteClient_S;

class FallbackableUdpTransport_S {
 public:
  /** 클라와의 UDP 통신이 가능한가 여부 */
  bool real_udp_enabled_;
  /** 소켓사용을 요청했는가 */
  bool client_udp_ready_waiting_;
  /** 소켓 생성 실패를 한적이 있는가? */
  bool client_udp_socket_create_failed_;

  /**
   * 클라의 서버 입장에서 인식된 주소, 클라 로컬에서 인식된 주소
   * 주의: udp_addr_from_here_를 바꿀 경우 AddOrUpdateUdpAddrToRemoteClientIndex를
   *
   * 반드시 호출해야 한다! 인덱스된 값이니까.
   */
  InetAddress udp_addr_from_here_;
  InetAddress udp_addr_internal_;

  /** 홀펀칭시 구분하기 위한 태그입니다. */
  Uuid holepunch_tag_;
  /** 이 객체를 소유하고 있는 RemoteClient입니다. */
  RemoteClient_S* owner_;

  /** UDP 소켓들 중 이 객체에 배정된 소켓 */
  UdpSocketPtr_S udp_socket_;

  FallbackableUdpTransport_S(RemoteClient_S* owner);
  ~FallbackableUdpTransport_S();

  const InetAddress& GetUdpAddrFromHere() const { return udp_addr_from_here_; }

  void SetUdpAddrFromHere(const InetAddress& addr);

  void SendWhenReady( HostId remote_id,
                      const SendFragRefs& data,
                      const UdpSendOption& send_opt);

  void ResetPacketFragState();

 private:
  TcpTransport_S& GetFallbackUdpTransport();
};

} // namespace net
} // namespace fun
