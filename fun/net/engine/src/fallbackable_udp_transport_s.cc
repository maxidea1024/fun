#include "FallbackableUdpTransport_S.h"
#include "NetServer.h"
#include "PacketFrag.h"
#include "RemoteClient.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

void FallbackableUdpTransport_S::SetUdpAddrFromHere(const InetAddress& addr) {
  if (udp_addr_from_here_ != addr) {
    // Remove from old collection.
    owner_->owner_->udp_addr_to_remote_client_index_.Remove(
        udp_addr_from_here_);

    udp_addr_from_here_ = addr;

    // Insert into new collection.
    if (udp_addr_from_here_ != InetAddress::None) {
      owner_->owner_->udp_addr_to_remote_client_index_.Add(udp_addr_from_here_,
                                                           owner_);
    }
  }
}

FallbackableUdpTransport_S::FallbackableUdpTransport_S(RemoteClient_S* owner)
    : owner_(owner),
      real_udp_enabled_(false),
      client_udp_ready_waiting_(false),
      client_udp_socket_create_failed_(false),
      udp_addr_from_here_(InetAddress::None),
      udp_addr_internal_(InetAddress::None),
      holepunch_tag_(Uuid::None) {}

void FallbackableUdpTransport_S::SendWhenReady(HostId remote_id,
                                               const SendFragRefs& data,
                                               const UdpSendOption& send_opt) {
  // 이함수 호출 전에 main lock이 걸려있어야함.
  owner_->LockMain_AssertIsLockedByCurrentThread();

  if (real_udp_enabled_) {
    // Unreliable을 UDP로 보내는 경우 MTU size에 제한해서 split하지 않는다.
    // 어차피 UDP 자체가 큰 메시지를 split하니까.
    // (단, 이때 fragmentation여부는 소켓옵션에서 설정할 수 있음.)
    const auto filter_tag = FilterTag::Make(HostId_Server, owner_->GetHostId());
    udp_socket_->SendWhenReady(remote_id, filter_tag, GetUdpAddrFromHere(),
                               data, send_opt);
  } else {  // Fallback mode
    // 서버와의 홀펀칭이 안되었을 경우에는 대안으로 TCP를 통해서 보내도록 한다.
    CScopedLock2 tcp_send_queue_guard(
        owner_->to_client_tcp_->GetSendQueueMutex());
    GetFallbackUdpTransport().SendWhenReady(data, send_opt);
  }
}

TcpTransport_S& FallbackableUdpTransport_S::GetFallbackUdpTransport() {
  return *(owner_->to_client_tcp_);
}

// UDP 소켓이 재사용될 경우 앞서 사용했던 packet frag 버퍼링이
// 새 UDP peer에게 갈 경우 문제가 있을 수 있다.
// 따라서 그것들을 모두 초기화해버린다.
void FallbackableUdpTransport_S::ResetPacketFragState() {
  if (udp_socket_ && udp_addr_from_here_.IsUnicast()) {
    udp_socket_->AssertIsLockedByCurrentThread();

    {
      udp_socket->AssertIsFraggerLockedByCurrentThread();

      // 주의:
      // 여기서 Fragger lock을 걸게되면 dispose에서 순서가 꼬이게 됨.
      // (미리 걸고들어오는걸로 변경)
      // CScopedLock2 fragger_guard(udp_socket_->GetFraggerMutex());
      udp_socket_->packet_fragger_->Remove(udp_addr_from_here_);
    }

    if (udp_socket_->packet_defragger_) {
      udp_socket_->packet_defragger_->Remove(udp_addr_from_here_);
    }
  }
}

FallbackableUdpTransport_S::~FallbackableUdpTransport_S() {}

}  // namespace net
}  // namespace fun
