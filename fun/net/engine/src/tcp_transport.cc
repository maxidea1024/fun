//TODO 코드정리
//@todo 첫번째 데이터(policy text) 수신시 splitter-test시 예외가 발생하는 문제점을 해소해야함.

#include "fun/net/net.h"
#include "TcpTransport.h"

namespace fun {
namespace net {

using lf = LiteFormat;

void TcpSendQueue::EnqueueCopy(const SendFragRefs& data_to_send, const TcpSendOption& send_opt) {
  auto packet = packet_pool_.NewOrRecycle();

  packet->unique_id = send_opt.unique_id;
  packet->packet = data_to_send.ToBytes();

  // 같은 UniqueId를 가지는 것을 찾아서 중복 처리한다. 단 0은 제외!
#ifdef ALLOW_UNIQUEID_FOR_TCP
  if (packet.unique_id != 0) {
    for (auto it = queue_.begin(); it != queue_.end(); ++it) {
      auto& context = *it;

      if (context.unique_id == packet.unique_id) {
        total_len_ -= context.packet.Len();
        context = packet;
        total_len_ += context.packet.Len();
        goto L1;
      }
    }
  }
#endif

  // 중복된 패킷이 아닐 경우에는, 바로 추가.
  queue_.Enqueue(packet);
  total_len_ += packet->packet.Len();

#ifdef ALLOW_UNIQUEID_FOR_TCP
L1:
#endif

  CheckConsistency();
}

// length만큼 보낼 데이터들을 fragmented send buffer(WSABUF)에 포인터 리스트로서 채운다.
void TcpSendQueue::FillSendBuf(FragmentedBuffer& output, int32 allowed_len) {
  //output 은 임시변수 형태로만 사용되므로, 구지 아래처럼 비워줄 필요가 없을터...
  //괜시리 capacity 재조정만 요구됨.
  //일단은 혹시 모르니 남겨두고... 차후에 멤버변수로 가지고 있지 않을 경우에는
  //제거하도록 하자.
  output.Clear(); // keep capacity(이게 유지하는게 바람직할까??)

  if (allowed_len <= 0) {
    return;
  }

  int32 acc_total = 0;

  if (partial_sent_packet_) {
    const uint8* ptr = (const uint8*)partial_sent_packet_->packet.ConstData() + partial_sent_len_;
    const int32 remain_len = partial_sent_packet_->packet.Len() - partial_sent_len_;
    output.Add(ptr, remain_len);
    acc_total += remain_len;
  }

  for (auto it = queue_.CreateIterator(); it && acc_total < allowed_len; ++it) {
    auto context = *it;
    const int32 packet_len = context->packet.Len();
    output.Add((const uint8*)context->packet.ConstData(), packet_len);
    acc_total += packet_len;
  }
}

TcpSendQueue::TcpSendQueue()
  : total_len_(0),
    partial_sent_len_(0),
    partial_sent_packet_(nullptr) {}

// `len`만큼 패킷 큐에서 제거한다.
// 
//   최우선: 보내다 만거, 차우선: 패킷 큐 상단
//   패킷 큐에서 제거 후 남은건 partial sent packet으로 옮긴다. 그리고 offset도 변경.
//   최종 처리 후 TotalLength로 변경됨.
void TcpSendQueue::DequeueNoCopy(int32 len) {
  if (len < 0) {
    throw InvalidArgumentException();
  }

  if (len > total_len_) {
    throw InvalidArgumentException();
  }

  if (len == 0) {
    return;
  }

  if (partial_sent_packet_) {
    // 새로 보낸 데이터 길이가 이전에 보내고 남은 것보다 작은 경우에는
    // 길이 카운터만 갱신하고 종료.
    // (아직까지도 이전에 보내고 남은걸 다 보내지 못한 상황.. 언젠간 다 보내겠지...)
    if ((partial_sent_len_ + len) < partial_sent_packet_->packet.Len()) {
      partial_sent_len_ += len;
      total_len_ -= len;
      return;
    }

    // 이전에 보내고 남은거 이상으로는 다보냈으니, 완료처리.
    // 물론, 조금더 보낼것이 있을수 있으므로, 여기서 바로 종료하지는 않음.
    const int32 Remainder = partial_sent_packet_->packet.Len() - partial_sent_len_;
    total_len_ -= Remainder;
    len -= Remainder;

    packet_pool_.ReturnToPool(partial_sent_packet_);
    partial_sent_packet_ = nullptr;
    partial_sent_len_ = 0;
  }

  // queue에서 length만큼 다 지우되 남은 것들은 partial sent packet으로 이송.
  while (len > 0 && !queue_.IsEmpty()) {
    auto head_packet = queue_.Dequeue();

    if (head_packet->packet.Len() <= len) { // 완전소진
      total_len_ -= head_packet->packet.Len();
      len -= head_packet->packet.Len();
      packet_pool_.ReturnToPool(head_packet);
    } else { // 해당 패킷중 일부만 보냈음.
      partial_sent_packet_ = head_packet;
      partial_sent_len_ = len; // 부분 패킷(마지막 보내던거) 전체를 보낸게 아니라 부분만 보낸것인데, 그 남은 길이임

      total_len_ -= len;
      len = 0;
      break;
    }
  }

  CheckConsistency();
}

void TcpSendQueue::LongTick(double absolute_time) {
  SendBrake.LongTick(absolute_time);
  send_speed.LongTick(absolute_time);
  RecentReceiveSpeedAtReceiverSide.LongTick(absolute_time);

  // 송신속도, 수신속도를 고려해서 최대송신가능속도를 결정
  AllowedMaxSendSpeed.CalcNewValue(send_speed.GetRecentSpeed(), RecentReceiveSpeedAtReceiverSide.GetValue());
}

TcpSendQueue::~TcpSendQueue() {
  for (auto& packet : queue_) {
    packet_pool_.ReturnToPool(packet);
  }

  if (partial_sent_packet_) {
    packet_pool_.ReturnToPool(partial_sent_packet_);
    partial_sent_packet_ = nullptr;
  }
}

void TcpSendQueue::CheckConsistency() {
#ifdef _DEBUG
  int32 len = 0;

  if (partial_sent_packet_) {
    fun_check(partial_sent_len_ < partial_sent_packet_->packet.Len());
    fun_check(partial_sent_len_ > 0);
    len += (partial_sent_packet_->packet.Len() - partial_sent_len_);
  }

  for (const auto& packet : queue_) {
    len += packet->packet.Len();
  }

  fun_check(len >= 0);
  fun_check(len == total_len_);
#endif
}

} // namespace net
} // namespace fun
