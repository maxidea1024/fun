#pragma once

#include "RUdpHost.h"

namespace fun {
namespace net {

// forward declarations
class RemotePeer_C;
class SendFragRefs;
// class ReceivedMessageList;
class ReceivedMessage;
class RUdpHost;
class IRUdpHostDelegate;

/**
 * P2P reliable 통신을 위한 객체
 */
class RemotePeerRUdp : public IRUdpHostDelegate {
 public:
  RemotePeerRUdp(RemotePeer_C* owner);

  void ResetEngine(FrameNumber frame_number);

  void SendWhenReady(const SendFragRefs& data_to_send);
  FrameNumber NextFrameNumberForAnotherReliablySendingFrame();
  bool EnqueueReceivedFrameAndGetFlushedMessages(
      RUdpFrame& frame, ReceivedMessageList& out_result, ResultCode& out_error);
  bool EnqueueReceivedFrameAndGetFlushedMessages(
      MessageIn& msg, ReceivedMessageList& out_result, ResultCode& out_error);

  // 주기적으로 호출되어야하는 함수입니다.
  void Heartbeat();
  // 프레임 한개를 UDP transport를 통해서 보냅니다.
  void SendOneFrameToUdpTransport(RUdpFrame& frame);
  int32 GetUdpSendBufferPacketFilledCount();
  bool IsUdpSendBufferPacketEmpty();
  HostId TEST_GetHostId();
  FrameNumber GetRecvExpectFrameNumber();
  bool IsReliableChannel();

  // IRUdpHostDelegate interface
  double GetAbsoluteTime() override;
  double GetRecentPing() override;

  // ReliableUDP를 처리하는 Host 객체입니다.
  SharedPtr<RUdpHost> host_;
  // 이 객체를 소유하고 있는 RemotePeer 객체입니다.
  RemotePeer_C* owner_;
  // P2P reliable 통신이 실패하면, 이 플래그가 true로 셋팅되며, true로 셋팅된
  // 이후로는 서버를 경유해서 relaying하게 됩니다.
  bool panic_;
};

}  // namespace net
}  // namespace fun
