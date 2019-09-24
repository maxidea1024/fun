#include "fallbackable_udp_transport_c.h"
#include "fun/net/net.h"
#include "net_client.h"
#include "udp_socket_c.h"

namespace fun {
namespace net {

FallbackableUdpTransport_C::FallbackableUdpTransport_C(NetClientImpl* owner) {
  owner_ = owner;

  server_addr_ = InetAddress::None;

  real_udp_enabled_time_ = 0;
  server_udp_ready_waiting_ = false;

  // 서버에서 개시명령이 오기전까지는 시도하지 않아야 하므로,
  // 쿨타임을 무한대로 설정해둠.
  // (무한값으로 설정할 경우에는 서버홀펀칭을 시도하지 않음.)
  holepunch_cooltime_ = NetConfig::INFINITE_COOLTIME;
  holepunch_attempt_count_ = 0;

  tcp_fallback_count_ = 0;
  real_udp_enabled_ = false;
  real_udp_enabled_time_ = 0;
  last_server_udp_packet_recv_count_ = 0;
  last_udp_packet_recv_interval_ = -1;
  last_server_udp_packet_recv_time_ = owner_->GetAbsoluteTime();
}

void FallbackableUdpTransport_C::SendWhenReady(HostId remote_id,
                                               const SendFragRefs& data,
                                               const UdpSendOptions& send_opt) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  const bool direct_udp_sendable =
      IsRealUdpEnabled() && !owner_->Get_ToServerUdpSocket()->IsSocketClosed();

  if (direct_udp_sendable) {
    // Server에게 보내는 udp send count를 한다.
    // FunNet 내부 RPC 메시지는 카운팅에서제외한다.
    if (!send_opt.engine_only_specific) {
      owner_->ToServerUdpSendCount++;
    }

    // UDP 송신 큐에 넣는다.
    // unreliable을 UDP로 보내는 경우 MTU size에 제한해서 split하는
    // 과정을 UDP socket 객체 내 frag board를 통해 시행한다.
    // PMTU discovery fail over를 위해.
    const auto filter_tag =
        FilterTag::Make(owner_->GetLocalHostId(), HostId_Server);
    owner_->Get_ToServerUdpSocket()->SendWhenReady(
        remote_id, filter_tag, server_addr_, data, owner_->GetAbsoluteTime(),
        send_opt);
  } else {  // Fallback mode
    GetFallbackUdpTransport()->SendWhenReady(data, send_opt);
  }
}

TcpTransport_C* FallbackableUdpTransport_C::GetFallbackUdpTransport() {
  return owner_->Get_ToServerTcp();
}

void FallbackableUdpTransport_C::SetRealUdpEnabled(bool enabled) {
  if (real_udp_enabled_ == enabled) {
    return;
  }

  real_udp_enabled_ = enabled;

  last_server_udp_packet_recv_time_ = owner_->GetAbsoluteTime();
  last_server_udp_packet_recv_count_ = 0;
  last_udp_packet_recv_interval_ = -1;

#ifdef UDP_PACKET_RECEIVE_LOG
  last_server_udp_packet_recv_queue_.Clear();
#endif

  if (enabled) {
    real_udp_enabled_time = owner_->GetAbsoluteTime();
  }
}

}  // namespace net
}  // namespace fun
