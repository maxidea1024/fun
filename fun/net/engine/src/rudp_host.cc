#include "fun/net/net.h"
#include "RUdpHost.h"

namespace fun {
namespace net {

// 수신된 스트림 객체 자체를 리턴한다.
// 성능 좋은 접근을 위함. 여기에서 처리된 부분을 뜯어내는건 유저의 몫이다.
StreamQueue* RUdpHost::GetReceivedStream() {
  return &receiver_.recv_stream_;
}

// 객체를 생성하자마자 상대와의 연결이 된 것으로 간주한다.
// 이미 초기 프레임 값이 서로 맞춰진 셈이니까.
RUdpHost::RUdpHost(IRUdpHostDelegate* delegate, FrameNumber first_frame_number)
  : sender_(this, first_frame_number),
    receiver_(this, first_frame_number),
    delegate_(fun_check_ptr(delegate)) {
}

void RUdpHost::GetStats(RUdpHostStats& out_stats) {
  //TODO 해주는게 좋을까??
  //out_stats.Reset();

  out_stats.received_frame_count = receiver_.receiver_window_.Count();
  out_stats.received_stream_count = receiver_.recv_stream_.GetLength();
  out_stats.total_received_stream_length = receiver_.total_received_stream_length_;
  out_stats.total_ack_frame_count = receiver_.total_ack_frame_count_;
  out_stats.recent_receive_speed = receiver_.recent_receive_speed;

  out_stats.expected_frame_number = receiver_.expected_frame_number;
  out_stats.last_received_data_frame_number = receiver_.last_received_data_frame_number_;

  out_stats.send_stream_count = sender_.send_stream_.GetLength();
  out_stats.first_send_frame_count = sender_.first_sender_window_.Count();
  out_stats.resend_frame_count = sender_.resend_window_.Count();
  out_stats.total_send_stream_length = sender_.total_send_stream_length_;
  out_stats.total_resend_count = sender_.total_resend_count_;
  out_stats.total_first_send_count = sender_.total_first_send_count_;
  out_stats.recent_send_frame_to_udp_speed = sender_.recent_send_frame_to_udp_speed_;
  out_stats.send_speed_limit = sender_.send_speed_limit_;
  out_stats.total_receive_data_count = receiver_.total_receive_data_frame_count_;

  if (!sender_.first_sender_window_.IsEmpty()) {
    out_stats.first_sender_window_last_frame = sender_.first_sender_window_.Back().frame_number;
  } else {
    out_stats.first_sender_window_last_frame = (FrameNumber)0;
  }

  if (!sender_.resend_window_.IsEmpty()) {
    out_stats.resend_window_last_frame = sender_.resend_window_.Back().frame_number;
  } else {
    out_stats.resend_window_last_frame = (FrameNumber)0;
  }

  out_stats.last_expected_frame_number_at_sender = sender_.last_expected_frame_number_at_sender_;
}

// 매 짧은 순간마다 이것을 호출해야 한다.
// 적정 주기는 1밀리초이다.
void RUdpHost::Tick(float elapsed_time) {
  // 수신 case부터 먼저 처리해야 ack frame을 먼저 보낼 수 있다.
  receiver_.Tick(elapsed_time);

  // 그리고 나서 송신 case 처리
  sender_.Tick(elapsed_time);
}

// 스트림에 송신할 데이터를 추가한다.
void RUdpHost::Send(const uint8* data, int32 length) {
  sender_.SendViaReliableUDP(data, length);
  sender_.ConditionalStreamToSenderWindow(false);
}

// UDP로부터 받은 프레임을 이 객체에 넣는다.
//
// 이것은 ack나 데이터 프레임을 구별해서 수신 윈도를 접근한다.
void RUdpHost::TakeReceivedFrame(RUdpFrame& frame) {
  receiver_.ProcessReceivedFrame(frame);
}

// 만약 다른 reliable send 경로를 통해 reliable UDP frame을
// 보낼 방도가 있다면 그쪽으로 이미 성공적으로 보내졌음을
//
// 가정해서 이번에 보낼 frame number를 양보하고 그 번호를 유저에게 건네준다.
// 예컨대 릴레이 서버를 구현할 때 이 기능이 쓰인다.
//
// (이 함수는 주의해서 사용해야 한다. 잘못 쓰면 reliable UDP 통신을 끊어버리는 원인이 된다.)
//
// 또한, 프레임번호는 연이처리가 되어야하는데 중간에 하나가 빠지게 되면,
// 수신윈도우안의 프레임들이 제때
// 처리되지 못하고 지속적으로 쌓이는 문제가 발생함.
FrameNumber RUdpHost::YieldFrameNumberForSendingLongFrameReliably() {
  return sender_.NextFrameNumber();
}

FrameNumber RUdpHost::GetRecvExpectFrameNumber() {
  return receiver_.expected_frame_number_;
}

} // namespace net
} // namespace fun
