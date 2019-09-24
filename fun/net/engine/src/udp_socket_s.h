#pragma once

#include "PacketFrag.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

class NetServerImpl;
class RemoteClient_S;

/**
 * 서버에서 보유한 UDP socket 1개
 * 이 객체를 CFallbackableUdpTransport_S에서 참조한다.
 */
class UdpSocket_S : public ICompletionContext,
                    public ListNode<UdpSocket_S>,
                    public IUdpPacketFraggerDelegate,
                    public IUdpPacketDefraggerDelegate,
                    public UseCount,
                    public IHostObject {
 public:
  CCriticalSection2 mutex_;
  CCriticalSection2 udp_pakcet_fragger_mutex_;

  // IUdpPacketDefraggerDelegate interface
  void EnqueuePacketDefragWarning(const InetAddress& sender,
                                  const char* text) override;
  int32 GetMessageMaxLength() override;
  HostId GetSrcHostIdByAddrAtDestSide_NOLOCK(const InetAddress& address);
  HostId GetLocalHostId() override;

  FUN_ALIGNED_VOLATILE bool send_issued_;
  FUN_ALIGNED_VOLATILE bool recv_issued_;

  NetServerImpl* owner_;
  InternalSocketPtr socket_;
  InetAddress cached_local_addr_;

  // why this is map instead of queue - to coalesce
  UniquePtr<UdpPacketFragger> packet_fragger_;

  // 로컬 변수처럼 쓰임. 이 안의 msgbuffer가 계속 재사용되어야 메모리 재할당을
  // 최소화하니까. UdpPacketFragger보다 나중에 선언되어야 함! 이게 먼저
  // 파괴되어야 하므로.
  UniquePtr<UdpPacketFraggerOutput> send_issued_frag_;

  UniquePtr<UdpPacketDefragger> packet_defragger_;

  UdpSocket_S(NetServerImpl* owner);
  ~UdpSocket_S();

  SocketErrorCode IssueSend(double absolute_time);

  CCriticalSection2& GetMutex();
  CCriticalSection2& GetSendMutex();

  void Decrease();

  // IHostObject interface
  void OnIssueSendFail(const char* where, SocketErrorCode socket_error);
  void ConditionalIssueSend();
  void ConditionalIssueRecvFrom();
  void SendWhenReady(HostId sender_id, const InetAddress& sender_addr,
                     HostId dest_id, const SendFragRefs& data_to_send,
                     const UdpSendOption& send_opt);
  void SendWhenReady(HostId host_id, FilterTag::Type filter_tag,
                     const InetAddress& send_to,
                     const SendFragRefs& data_to_send,
                     const UdpSendOption& send_opt);

  NamedInetAddress GetRemoteIdentifiableLocalAddr(RemoteClient_S* client);

  virtual LeanType GetLeanType() const { return LeanType::UdpSocket_S; }

  InetAddress GetCachedLocalAddr();

  double GetAbsoluteTime() override;
  void RequestReceiveSpeedAtReceiverSide_NoRelay(
      const InetAddress& dest_addr) override;
  int32 GetOverSendSuspectingThresholdInByte() override;

  void LongTick(double absolute_time);

  void AssertIsLockedByCurrentThread() {
    GetMutex().AssertIsLockedByCurrentThread();
  }

  void AssertIsNotLockedByCurrentThread() {
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

  bool IsLockedByCurrentThread() {
    return GetMutex().IsLockedByCurrentThread();
  }

  CCriticalSection2& GetFraggerMutex() { return udp_pakcet_fragger_mutex_; }

  void AssertIsFraggerLockedByCurrentThread() {
    GetFraggerMutex().AssertIsLockedByCurrentThread();
  }

  void AssertIsFraggerNotLockedByCurrentThread() {
    GetFraggerMutex().AssertIsNotLockedByCurrentThread();
  }

  bool IsFraggerLockedByCurrentThread() {
    return GetFraggerMutex().IsLockedByCurrentThread();
  }
};

typedef SharedPtr<UdpSocket_S> UdpSocketPtr_S;

}  // namespace net
}  // namespace fun
