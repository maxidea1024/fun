#pragma once

namespace fun {
namespace net {

class NetClientImpl;
class TcpTransport_C;
class UdpSocket_C;
class SendFragRefs;

/**
 * UDP로 보내기가 가능할 경우에는 UDP로 우선 보내고, 그렇지 않을 경우에는 대안으로
 * TCP로 보내기가 가능한 transport 객체임.
 */
class FallbackableUdpTransport_C {
 public:
  FallbackableUdpTransport_C(NetClientImpl* owner);

  void SendWhenReady( HostId remote_id,
                      const SendFragRefs& data,
                      const UdpSendOptions& options);

  bool IsRealUdpEnabled() const { return real_udp_enabled_; }
  void SetRealUdpEnabled(bool enable);

 private:
  /** 이 객체를 소유한 NetClient 객체입니다. */
  NetClientImpl* owner_;

  TcpTransport_C* GetFallbackUdpTransport();

 public:
  /** 홀펀칭시 구분하기 위한 태그입니다. */
  Uuid holepunch_tag_;
  /** 클라에서 UDP 소켓을 생성하고(JIT UDP) 서버에 UDP 소켓 생성을 요청해서 응답을 대기중일 때만 true이다. */
  FUN_ALIGNED_VOLATILE bool server_udp_ready_waiting_;
  /** 서버와의 UDP 통신이 가능한지 여부를 나타냅니다. */
  FUN_ALIGNED_VOLATILE bool real_udp_enabled_;
  /** 서버와의 UDP 통신이 가능해진 시점의 시각입니다. */
  FUN_ALIGNED_VOLATILE double real_udp_enabled_time_;
  /** 서버 주소입니다. */
  InetAddress server_addr_;
  /** 서버 홀펀칭 시도 쿨타임. 초기에는 '안함' 이 지정됨 */
  FUN_ALIGNED_VOLATILE double holepunch_cooltime_;
  /** 서버 홀펀칭 시도 횟수. 홀펀칭이 안 될 놈이 영원히 하는 것을 방지용. */
  FUN_ALIGNED_VOLATILE int32 holepunch_attempt_count_;
  /** UDP -> TCP 후퇴한 횟수 */
  FUN_ALIGNED_VOLATILE int32 tcp_fallback_count_;

  // 서버로부터 가장 마지막에 UDP로 패킷을 받은 시간
  // TCP fallback을 안한 경우에만 유효한 값이다.
  FUN_ALIGNED_VOLATILE double last_server_udp_packet_recv_time_;
  FUN_ALIGNED_VOLATILE int32  last_server_udp_packet_recv_count_;
  FUN_ALIGNED_VOLATILE double last_udp_packet_recv_interval_;

#ifdef UDP_PACKET_RECEIVE_LOG
  Array<double> last_server_udp_packet_recv_queue_;
#endif
};

typedef SharedPtr<FallbackableUdpTransport_C> FallbackableUdpTransportPtr_C;

} // namespace net
} // namespace fun
