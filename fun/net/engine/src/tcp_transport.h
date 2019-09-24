#pragma once

#include "fun/net/net.h"
#include "fun/base/object_pool.h"
#include "send_brake.h"

namespace fun {
namespace net {

class SendFragRefs;

class TcpPacketContext {
 public:
  // TODO 풀링되므로, 계속 생성되는 CByteString보다는 Array<uint8>로 변경해야함.
  ByteArray packet;
  uint64 unique_id;
  //bool no_coalesce;

 public:
  TcpPacketContext()
    : unique_id(0)
    //, no_coalesce(false)
  {
  }

  TcpPacketContext(const TcpSendOption& opt)
    : unique_id(opt.unique_id)
    //, no_coalesce(opt.no_coalesce)
  {
  }
};


// reliable send 자체는 우선순위가 모순이다. 따라서 아예 지원하지 않는다.
// 송신 속도 조절기도 포함됨
//
// NOTE 결과적으로 송싱속도 제어기는 사용하지 않는다는 얘기임. 그러므로, 제거해야한다는 얘기가 됨.
// SendBrake, AllowedMaxSendSpeed, RecentSpeedMeasurer 자체를 제거하는건 어떨런지?
// 아울러 TcpSendQueue 자체가 필요한가? coalescing을 위해서 잔존시켜야할까??
class TcpSendQueue {
 public:
  TcpSendQueue();
  ~TcpSendQueue();

 public:
  void EnqueueCopy(const SendFragRefs& data, const TcpSendOption& option);
  void FillSendBuf(FragmentedBuffer& output, int32 Length);
  inline int32 GetLength() const { return total_length_; }
  void DequeueNoCopy(int32 Length);
  void LongTick(double absolute_time);

 public:
  ObjectPool<TcpPacketContext> PacketPool;
  List<TcpPacketContext*> Queue;

  TcpSendQueue(const TcpSendQueue& rhs);
  TcpSendQueue& operator = (const TcpSendQueue& rhs);

  TcpPacketContext* partial_sent_packet_;
  int32 partial_sent_length_;
  int32 total_length_;

  void CheckConsistency();

 public:
  // TCP 자체가 ack에 의한 sender window slide 기능이 존재->송신량 자연 조절->따라서 불필요함.
  SendBrake send_brake_;
  AllowedMaxSendSpeed allowed_max_send_speed_;
  RecentSpeedMeasurer send_speed_;
  RecentReceiveSpeedAtReceiverSide recent_receive_speed_at_receiver_side_;
};

} // namespace net
} // namespace fun
