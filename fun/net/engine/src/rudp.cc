#include "fun/net/net.h"
#include "NetClient.h"
#include "RUdp.h"
#include "RUdpFrame.h"
#include "RUdpConfig.h"
#include "StreamQueue.h"
#include "MessageStream.h"
#include "RUdpHost.h"
#include "RpcCallOptionImpl.h"
#include "RUdpHelper.h"

namespace fun {
namespace net {

using lf = LiteFormat;

FrameNumber RemotePeerRUdp::NextFrameNumberForAnotherReliablySendingFrame() {
  return host_->YieldFrameNumberForSendingLongFrameReliably();
}

bool RemotePeerRUdp::EnqueueReceivedFrameAndGetFlushedMessages( MessageIn& msg,
                                                                ReceivedMessageList& output,
                                                                ResultCode& out_error) {
  RUdpFrame frame;
  FUN_DO_CHECKED(lf::Read(msg, frame.type));

  switch (frame.type) {
    case RUdpFrameType::Ack:
      frame.acked_frame_numbers_.Reset(new CompressedFrameNumbers());

      FUN_DO_CHECKED(frame.acked_frame_numbers_->Read(msg));
      FUN_DO_CHECKED(lf::Reads(msg,  frame.expected_frame_number, frame.recent_receive_speed));

      //if (!msg.AtEnd()) { // 이것도 넣어야 할까나?
      //  return false;
      //}
      break;

    case RUdpFrameType::Data: {
      FUN_DO_CHECKED(lf::Read(msg, frame.frame_number));
      ByteArray frame_data;
      FUN_DO_CHECKED(lf::Read(msg, frame_data));
      //frame.data = MessageIn(frame_data);
      frame.data = frame_data;
      break;
    }

    //case ERUdpFrame::Disconnect:
    //  FUN_DO_CHECKED(lf::Read(msg, frame.frame_id));
    //  FUN_DO_CHECKED(lf::Read(msg, frame.data));
    //  break;

    default:
      fun_check(false);
      break;
  }

  return EnqueueReceivedFrameAndGetFlushedMessages(frame, output, out_error);
}

bool RemotePeerRUdp::EnqueueReceivedFrameAndGetFlushedMessages( RUdpFrame& frame,
                                                                ReceivedMessageList& output,
                                                                ResultCode& out_error) {
  output.Clear(); // just in case

  host_->TakeReceivedFrame(frame);

  auto recv_stream = host_->GetReceivedStream();

  const int32 added_count = MessageStream::ExtractMessagesAndFlushStream(
                *recv_stream,
                output,
                owner_->host_id_,
                owner_->owner_->settings_.message_max_length,
                out_error);
  if (added_count < 0) {
    // 해당 peer와 더 이상 통신하기 어려운 마당이다.
    panic_ = true;
  }

  return true;
}

void RemotePeerRUdp::Heartbeat() {
  host_->Tick((float)owner_->owner_->GetElapsedTime());
}

void RemotePeerRUdp::SendOneFrameToUdpTransport(RUdpFrame& frame) {
  MessagePriority priority = GUnreliableSend_INTERNAL.priority;

  switch (frame.type) {
    case RUdpFrameType::Data:
      if (RUdpConfig::high_priority_data_frame) {
        priority = MessagePriority::Ring1;
      }
      break;

    case RUdpFrameType::Ack:
      if (RUdpConfig::high_priority_ack_frame) { // ACK는 더 높은 우선으로 쏴야 한다.
        priority = MessagePriority::Ring1;
      }
      break;
  }

  // 가능하면 직빵 UDP로 송신한다.
  if (owner_->IsDirectP2P()) {
    SendFragRefs data_to_send;

    MessageOut header;
    RUdpHelper::BuildSendDataFromFrame(frame, data_to_send, header);

    owner_->to_peer_udp_.SendWhenReady(data_to_send, UdpSendOption(priority, EngineOnlyFeature));
  } else {
    //TODO 전송시 깨지는 부분이 생기는듯함...
    //LOG(LogNetEngine,Warning,"Send ReliableUDP frame via relaying: frame=%d, len=%d", (int32)frame.frame_number_, frame.data_.Len());

    // [직빵으로 P2P로 보내지 못하는 경우]
    // TCP를 통해 서버로 릴레이한다.
    // ACK는 아예 보낼 필요도 없다. 괜히 쓸데없는 부하만 준다.
    // (참고: 이 직후 Resend는 더 이상 하지 않는다. 자세한 것은 RUdpSender::DoFramesForSend 참고)

    // CLIENT -> (LingerDataFrame1) -> SERVER -> (LingerDataFrame2) -> PEER

    switch (frame.type) {
      case RUdpFrameType::Data: {
          MessageOut header;
          lf::Write(header, MessageType::LingerDataFrame1);
          lf::Write(header, owner_->host_id_);
          lf::Write(header, frame.frame_number_);
          lf::Write(header, OptimalCounter32(frame.data_.Len()));

          SendFragRefs data_to_send;
          data_to_send.Add(header);
          data_to_send.Add(frame.data_);

          owner_->owner_->Send_ToServer_Directly_Copy(
              owner_->GetHostId(),
              MessageReliability::Reliable,
              data_to_send,
              UdpSendOption(GUnreliableSend_INTERNAL));
        }
        break;

      case RUdpFrameType::Ack:
        // Nothing to do...
        break;

      default:
        fun_check(false); // 의도 안한 프레임 타입
        break;
    }
  }
}

FrameNumber RemotePeerRUdp::GetRecvExpectFrameNumber() {
  return host_->GetRecvExpectFrameNumber();
}

bool RemotePeerRUdp::IsReliableChannel() {
  return owner_->IsRelayedP2P();
}

void RemotePeerRUdp::SendWhenReady(const SendFragRefs& data_to_send) {
  // Add message to send queue for each remote host
  SendFragRefs final_send_data;
  MessageOut header;
  MessageStream::AddStreamHeader(data_to_send, final_send_data, header);

  const ByteArray bytes = final_send_data.ToBytes(); // copy
  host_->Send((const uint8*)bytes.ConstData(), bytes.Len());
}

double RemotePeerRUdp::GetAbsoluteTime() {
  return owner_->owner_->GetAbsoluteTime();
}

RemotePeerRUdp::RemotePeerRUdp(RemotePeer_C* owner)
  : owner_(owner), panic_(false) {
}

void RemotePeerRUdp::ResetEngine(FrameNumber frame_number) {
  host_.Reset(new RUdpHost(this, frame_number));
}

int32 RemotePeerRUdp::GetUdpSendBufferPacketFilledCount() {
  if (owner_->IsDirectP2P()) {
    return owner_->to_peer_udp_.GetUdpSendBufferPacketFilledCount();
  } else {
    // 릴레이 모드에서는 TCP로 바로 전송하는 것이기 때문에 어차피 랙이 존재.
    // 따라서 비어있지 않다는 식으로 보내도 OK.
    return 1;
  }
}

bool RemotePeerRUdp::IsUdpSendBufferPacketEmpty() {
  if (owner_->IsDirectP2P()) {
    return owner_->to_peer_udp_.IsUdpSendBufferPacketEmpty();
  } else {
    // 릴레이 모드에서는 TCP로 바로 전송하는 것이기 때문에 어차피 랙이 존재.
    // 따라서 비어있지 않다는 식으로 보내도 OK.
    return false;
  }
}

HostId RemotePeerRUdp::TEST_GetHostId() {
  return owner_->host_id_;
}

double RemotePeerRUdp::GetRecentPing() {
  return MathBase::Max(owner_->recent_ping_, 0.0);
}

} // namespace net
} // namespace fun
