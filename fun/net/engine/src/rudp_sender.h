#pragma once

#include "RUdpConfig.h"
#include "RUdpFrame.h"
#include "StreamQueue.h"

namespace fun {
namespace net {

class RUdpHost;

/**
 * reliable UDP 송신기
 * window size나 ack, frame 등의 용어는 일반적인 data link layer
 * 및 TCP 프로토콜의 용어에서와 같다.
 *
 * 단독으로는 쓰이지 못하고, RUdpReceiver와 쌍을 맺어야 한다.
 * RUdpReceiver를 통해 ack를 받으니까.
 *
 * 주요 메서드: Tick, SendViaReliableUDP
 */
class RUdpSender {
 private:
  /** 소유자 */
  RUdpHost* owner_;

  /**이번에 송신 frame에 할당할 ID */
  FrameNumber current_frame_number_;

 public:
  /** 수신자가 최근에 받는 수신 속도 */
  int32 remote_recv_speed_;

  /** 마지막 ack를 송신한 후 지연된 시간 */
  double last_acK_send_time_elapsed_;

  /** 마지막으로 data frame을 송신한 후 지연된 시간 */
  double last_data_send_time_elapsed_;

  /** 마지막으로 stream -> sender window 이동 처리를 수행한 시간 */
  double last_do_stream_to_sender_window_time_;

  /** 총 ack 프레임을 받은 횟수 */
  int32 total_ack_frame_recv_count_;

  /**
   * 조정된 송신 최대 속도. 즉 window size와 관련된다.
   * 너무 크게 잡히면 송신 과부하로 drop되는 패킷이 많아지고 반대로 너무 적으면 비효율.
   */
  int32 send_speed_limit_;

  /**
   * 송신될 데이터가 처음 쌓이는 곳
   * TCP send buffer stream과 같은 역할을 한다.
   */
  StreamQueue send_stream_;

  // 송신 대기중인 frames, 즉 송신 윈도
  typedef List<SenderFrame> SenderWindow;
  SenderWindow first_sender_window_; //초송신큐
  SenderWindow resend_window_; //재송신 큐

  // 통계
  int32 total_send_stream_length_;
  int32 total_resend_count_;
  int32 total_first_send_count_;

  int32 recent_send_frame_to_udp_count_;
  double recent_send_frame_to_udp_start_time_;
  int32 recent_send_frame_to_udp_speed_;

  // 시작 이래 가장 오랫동안 resend가 시도된 시간. 이 모듈에 의해 계속 업데이트된다.
  // 타임아웃을 찾기 위한 용도로 사용 가능.
  double max_resend_elapsed_time_;

  FrameNumber last_expected_frame_number_at_sender_;
  FrameNumber last_received_ack_frame_number_;
  double last_received_ack_time_;

 public:
  RUdpSender(RUdpHost* owner, FrameNumber first_frame_number);

  void SendViaReliableUDP(const uint8* data, int32 length);

  /**
   * Frame을 쪼개서 보내는 흉내를 낸다.
   */
  void Tick(float elapsed_time);

 private:
  /**
   * 최근 송신 속도를 계산한다
   */
  void CalcRecentSendSpeed();

  /**
   * 송신 프레임 윈도우를 체크해서 재송신을 하거나 초송신을 한다.
   */
  void ConditionalFirstSenderWindowToUdpSender();

  void ConditionalResendWindowToUdpSender(double elapsed_time);

 public:
  void ConditionalStreamToSenderWindow(bool move_now);

  FrameNumber NextFrameNumber();

  /**
   * Send frame queue에서 한개를 제거한다.
   */
  bool RemoveFromSenderWindow(FrameNumber frame_id);

  void RemoveSpecifiedAndItsPastsFromSenderWindow(FrameNumber frame_id);

  void SendOneFrame(RUdpFrame& frame, bool resend);
};

} // namespace net
} // namespace fun
