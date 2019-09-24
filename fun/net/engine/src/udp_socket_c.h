#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class NetClientImpl;
class RemotePeer_C;

class UdpSocket_C
  : public ICompletionContext,
    public IHasOverlappedIo {
 public:
  // 만약 이 socket이 invalid->restore를 막 했으면 그때만 true이다.
  bool just_restored_;

  FUN_ALIGNED_VOLATILE double last_udp_recv_issued_time_;

  UniquePtr<UdpPacketDefragger> packet_defragger_;

  UdpSocket_C(NetClientImpl* owner, RemotePeer_C* owner_peer);
  ~UdpSocket_C();

  void LongTick(double absolute_time);

  bool CreateSocket(const InetAddress& udp_local_addr);

  bool RestoreSocket();

  void ConditionalIssueSend();

  void ConditionalIssueRecvFrom();

  void SendWhenReady( HostId final_dest_id,
                      FilterTag::Type filter_tag,
                      const InetAddress& send_to,
                      const SendFragRefs& data_to_send,
                      double added_time,
                      const UdpSendOption& send_opt);

  int32 GetUdpSendBufferPacketFilledCount(const InetAddress& dest_addr) {
    return packet_fragger_->GetTotalPacketCountOfAddr(dest_addr);
  }

  bool IsUdpSendBufferPacketEmpty(const InetAddress& dest_addr) {
    return packet_fragger_->IsUdpSendBufferPacketEmpty(dest_addr);
  }

  void ResetPacketFragState(RemotePeer_C* owner_peer);

  void CloseSocketHandleOnly();

  // IHasOverlappedIo interface
  void OnCloseSocketAndMakeOrphant() override;
  bool IsSocketClosed() override;

  // ICompletionContext interface
  LeanType GetLeanType() const override { return LeanType::UdpSocket_C; }

  InternalSocketPtr socket_;

  // 이게 queue가 아니고 map인 이유 - coalesce를 하기 위함
  UniquePtr<UdpPacketFragger> packet_fragger_;

  // 로컬 변수처럼 쓰임. 이 안의 msgbuffer가 계속 재사용되어야 메모리 재할당을 최소화하니까.
  // UdpPacketFragger보다 나중에 선언되어야 함! 이게 먼저 파괴되어야 하므로.
  UniquePtr<UdpPacketFraggerOutput> send_issued_frag_;

  // owner_peer_ == nullptr이지만 main_ != nullptr인 경우가 있으므로 이런 변수가 별도 필요.
  // owner_peer_ == nullptr이지만 main_ != nullptr인 경우란, 홀펀칭 재사용을 위해 재사용통에 들어가 있는 경우이다.
  NetClientImpl* main_;

  RemotePeer_C* owner_peer_;

  // LAN에서 인식되는 주소. 헤어핀 홀펀칭을 위함.
  InetAddress local_addr_;

  // 서버에서 인식된 이 소켓의 주소, 즉 외부 인터넷 주소. FunNet.RemotePeer_C.UdpAddrFromServer와 다름.
  InetAddress here_addr_at_server_;

  // 홀펀칭 재활용통에 들어간 시간. 0이면 재활용통에 들어가지 않았음을 의미.
  double recycle_binned_time_;

 private:
  bool RefreshLocalAddress();
};

typedef SharedPtr<UdpSocket_C> UdpSocketPtr_C;

} // namespace net
} // namespace fun
