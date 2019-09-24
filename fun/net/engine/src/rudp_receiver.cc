//수신 윈도우
// 일단 받은 프레임들을 보존하는 역활임.  일단 받고, 빼나갈 상황이 되면 빼나가므로 버퍼의 역활임.
// 그런 이유로, 수신 윈도우의 내용들은 빼나가기 전까지 보존되어야함.
//

#include "fun/net/net.h"
#include "RUdpReceiver.h"
#include "RUdpHost.h"

namespace fun {
namespace net {

RUdpReceiver::RUdpReceiver(RUdpHost* owner, FrameNumber first_frame_number)
  : recv_stream_(NetConfig::stream_grow_by),
    owner_(owner),
    total_received_stream_length_(0),
    total_ack_frame_count_(0),
    recent_receive_frame_count_(0),
    recent_receive_frame_count_start_time_(0),
    recent_receive_speed(RUdpConfig::received_speed_before_update),
    total_receive_data_frame_count_(0),
    expected_frame_number(first_frame_number),
    last_received_data_frame_number_(first_frame_number),
    last_send_gathered_acks_time_(0) {
}

// 모여진 ack들을 1개 이상의 ack 프레임으로 변환해서 송신자에게 보낸다.
void RUdpReceiver::SendGatheredAcks() {
  fun_check(RUdpConfig::max_ack_count_in_one_frame > 2);

  // 모여진 ack들을 상대에게 보내고 ack list를 청소한다.
  // 1개 프레임에서 보낼 수 있는 ack 갯수는 제한되므로 몇번에 나눠 보낸다.
  while (!acks_to_send_.IsEmpty()) {
    RUdpFrame frame;
    frame.type = RUdpFrameType::Ack;
    frame.frame_number = (FrameNumber)0;
    frame.acked_frame_numbers.Reset(new CompressedFrameNumbers()); // 계속 생성하는게 좀 걸리는데....

    int32 insertion_count = 0;//acks_to_send_.Count();

    // 보낼 ack list를 모두 정렬한다. 설령 다 넣지 못하더라도.
    //HeuristicQuickSort(acks_to_send_.MutableData(), acks_to_send_.Count(), 2000);
    acks_to_send_.Sort();

    // 보낼 ack list를 순서대로 넣는다.
    for (int32 i = 0; i < acks_to_send_.Count(); ++i) {
      const FrameNumber frame_id = acks_to_send_[i];
      frame.acked_frame_numbers->AddSortedNumber(frame_id);

      insertion_count++;

      // 프레임당 최대 ACK를 초과할 경우에는 다음번에 보냅니다.
      if (frame.acked_frame_numbers->Count() >= RUdpConfig::max_ack_count_in_one_frame) {
        break;
      }
    }

    frame.recent_receive_speed = recent_receive_speed;
    frame.expected_frame_number = expected_frame_number;

    owner_->sender.SendOneFrame(frame, false);
    owner_->sender.last_acK_send_time_elapsed_ = 0;

    //@todo 역순으로 저장하자. array shift cost를 절약하자.
    acks_to_send_.RemoveAt(0, insertion_count);
  }
}

// 통상 데이터 프레임을 받아 처리한다
void RUdpReceiver::ProcessDataFrame(RUdpFrame& frame) {
  ++total_receive_data_frame_count_;

  // ack list에 추가한다. 단, 타 reliable channel을 통해 송신 가능 상황이면(예컨대 tcp 릴레이 모드)
  // ack에 포함시키지 않는다.
  // 또한, expected frame number 과거 것들도 추가를 하지 않는다.
  // 추후 보낼 ack frame에서 expected frame number를 같이 보내기 때문에 안 보내도 ok.
  if (!owner_->delegate_->IsReliableChannel()) {
    if (FrameNumberUtil::Compare(frame.frame_number, expected_frame_number) >= 0) {
      acks_to_send_.Add(frame.frame_number);
      total_ack_frame_count_++;
    }
  }

  last_received_data_frame_number_ = frame.frame_number;

  // 적절한 Frame이면...
  if (!IsTooOldFrame(frame.frame_number)) {
    // fill received frame to reliable UDP recv buffer
    AddToReceivedFrames(frame);

    // flush accumulated queue items to user recv queue if any
    FlushAckAccumulatedToOutput();
  } //else {
  //  LOG(LogNetEngine,Warning, "TooOldFrame!!\n");
  //}
}

// 수신된 데이터 프레임의 프레임 번호 등을 감안, 수신윈도에 넣는다.
void RUdpReceiver::AddToReceivedFrames(RUdpFrame& received_frame) {
  // 순서에 맞추어서, ReceivedFrame에 채워 넣음.
  for (auto it = receiver_window_.CreateIterator(); it; ++it) {
    auto& frame = *it;

    // 이미 같은게 있으면, 바로 완료.
    if (frame.frame_number == received_frame.frame_number) {
      return;
    }

    // 바로 앞에 삽입함.
    if (FrameNumberUtil::Compare(received_frame.frame_number, frame.frame_number) < 0) {
      receiver_window_.InsertBefore(it, ReceiverFrame(received_frame));

      // 수신큐에 제대로 삽입된 것이므로 카운팅.
      recent_receive_frame_count_++;
      return;
    }
  }

  // 맨 끝에 삽입.
  receiver_window_.Append(ReceiverFrame(received_frame));

  // 수신큐에 제대로 삽입된 것이므로 카운팅.
  recent_receive_frame_count_++;
}

// 기대했던 프레임 번호와 순차적으로 연속된 프레임들을 모아서
// 수신 스트림으로 보내준다.
// 
// 즉, 수신 성공 처리를 한다.
// 
// 수신윈도우(receiver_window_)에 있는 프레임들을 RecvStream으로 Push함.
void RUdpReceiver::FlushAckAccumulatedToOutput() {
  // 유저에게 모아서 stream처럼 만들어 보낸다.
  // 그리고 receive queue에서 제거한다.
  while (!receiver_window_.IsEmpty()) {
    auto& receiver_frame = receiver_window_.Front();

    if (receiver_frame.frame_number == expected_frame_number) {
      const uint8* frame_data = (const uint8*)receiver_frame.data.ConstData();
      const int32 frame_len = receiver_frame.data.Len();
      recv_stream_.EnqueueCopy(frame_data, frame_len); // copy
      total_received_stream_length_ += frame_len;
      receiver_window_.RemoveFront();

      // 시퀀스 넘버 증가.
      // * overflow 방지처리를 위해서 헬퍼 함수를 사용함.
      expected_frame_number = FrameNumberUtil::NextFrameNumber(expected_frame_number);
    } else {
      break;
    }
  }
}

// ack 프레임을 처리한다. 즉 송신윈도에서 제거한다.
void RUdpReceiver::ProcessAckFrame(RUdpFrame& frame) {
  // Remove from send-frame list
  DecompressedFrameNumberArray ack_list;
  frame.acked_frame_numbers->Decompress(ack_list);

  if (!ack_list.IsEmpty()) {
    owner_->sender.last_received_ack_frame_number_ = ack_list.Last();
    owner_->sender.last_received_ack_time_ = owner_->delegate_->GetAbsoluteTime();
    owner_->sender.total_ack_frame_recv_count_++;
  }

  // ack를 보낸 측(즉 data frame의 수신자)에서 보내준 expected frame number 이하 것들은 미리 걸러내버린다.
  owner_->sender.RemoveSpecifiedAndItsPastsFromSenderWindow(frame.expected_frame_number);
  owner_->sender.last_expected_frame_number_at_sender_ = frame.expected_frame_number;

  // 받은 ack 내용들을 sender window에서 모두 제거한다.
  for (int32 i = 0; i < ack_list.Count(); ++i) {
    const FrameNumber acked_frame_id = ack_list[i];
    owner_->sender.RemoveFromSenderWindow(acked_frame_id);
  }

  // update remote's receive speed with 10% weight
  owner_->sender.remote_recv_speed_ = (int32)MathBase::Lerp((double)owner_->sender.remote_recv_speed_, (double)frame.recent_receive_speed, 0.9) + 1;
}

// 너무 오래된 프레임인가를 체크하되 프레임 번호가 비트와이즈로
// 한바퀴 돈 경우도 고려한다.
bool RUdpReceiver::IsTooOldFrame(FrameNumber frame_id) const {
  return FrameNumberUtil::Compare(frame_id, expected_frame_number) < 0;
}

void RUdpReceiver::Tick(float elapsed_time) {
  const double absolute_time = owner_->delegate_->GetAbsoluteTime();

  // 첫번째 호출일 경우, 시간 기록.
  if (recent_receive_frame_count_start_time_ == 0) {
    recent_receive_frame_count_start_time_ = absolute_time;
  }

  if ((absolute_time - recent_receive_frame_count_start_time_) > RUdpConfig::calc_recent_receive_interval) {
    const double T = 0.1 / (absolute_time - recent_receive_frame_count_start_time_);
    recent_receive_speed = (int32)MathBase::Lerp((double)recent_receive_speed, (double)recent_receive_frame_count_, T);
    recent_receive_frame_count_ = 0;
    recent_receive_frame_count_start_time_ = absolute_time;
  }

  // 매 Tick마다 ack를 모으면 너무 조금 모여서 frame 갯수만 늘어난다.
  // 따라서 어느 정도의 시간 간격을 두고 ack들을 모아야 한다.
  // 그렇다고 너무 길게 기다리면 resend의 확률이 커지므로 조심해야 한다.
  if ((absolute_time - last_send_gathered_acks_time_) > RUdpConfig::stream_to_sender_window_coalesce_interval * 0.2) {
    SendGatheredAcks();
    last_send_gathered_acks_time_ = absolute_time;
  }
}

// UDP에서 받은 프레임을 처리한다. ack이건 통상이건 연결 해제 요청이건간에.
void RUdpReceiver::ProcessReceivedFrame(RUdpFrame& frame) {
  switch (frame.type) {
    case RUdpFrameType::Ack:
      ProcessAckFrame(frame);
      break;
    case RUdpFrameType::Data:
      ProcessDataFrame(frame);
      break;
  }
}

} // namespace net
} // namespace fun
