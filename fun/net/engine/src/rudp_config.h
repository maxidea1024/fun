// TODO 상수와 설정 가능한 값을 분리하도록 하자.

#pragma once

namespace fun {
namespace net {

class RUdpConfig {
 public:
  //사용안됨.
  // static uint8 first_stream_value;

  //사용안됨.
  // static int32 max_random_stream_length;

  /** 한번에 보낼 수 있는 단위. 즉, 한 프레임의 길이입니다. MTU 보다 커서는
   * 안됩니다. */
  static int32 frame_length;

  /** 첫번째 재전송시까지의 대기시간입니다. */
  static double first_resend_cooltime;

  // static double enlarge_resend_cooltime_ratio;

  /** 최대 재전송 대기시간입니다. */
  static double max_resend_cooltime;

  //사용안됨.
  // static double min_lag;

  //사용안됨.
  // static double max_lag;

  //사용안됨.
  // static double simulated_udp_reliability_ratio;

  //사용안됨.
  // static int32 too_old_frame_number_threshold;

  /** 한 프레임당 최대 응답(Acknowledge) 수입니다. */
  static int32 max_ack_count_in_one_frame;

  //사용안됨.
  // static double tick_interval;

  //사용안됨.
  // static double show_ui_interval;

  static int32 received_speed_before_update;

  static double calc_recent_receive_interval;

  static int32 max_send_speed_in_frame_count;

  static double brake_max_send_speed_threshold;

  static double stream_to_sender_window_coalesce_interval;

  /** 응답 프레임 전송시 높은 우선순위를 사용할지 여부입니다. */
  static bool high_priority_ack_frame;

  /** 데이터 프레임 전송시 높은 우선순위를 사용할지 여부입니다. */
  static bool high_priority_data_frame;

  static double resend_limit_ratio;

  /** 1개의 호스트가 초당 보낼 수 있는 총 프레임의 최소 갯수입니다. */
  static int32 min_resend_limit_count;

  /** 1개의 호스트가 초당 보낼 수 있는 총 프레임의 최대 갯수입니다. */
  static int32 max_resend_limit_count;
};

}  // namespace net
}  // namespace fun
