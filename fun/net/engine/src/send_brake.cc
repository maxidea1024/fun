//TODO 실제로 사용되는지 여부 검토.
#include "fun/net/net.h"
#include "SendBrake.h"

namespace fun {
namespace net {

bool SendBrake::BrakeNeeded(double absolute_time, double max_send_speed) {
  // Send brake가 문제있다고 의심되면 brake needed는 항상 false를 리턴한다.
  if (!NetConfig::send_break_enabled) {
    return false;
  }

  if (last_clear_time_ == 0) {
    last_clear_time_ = absolute_time;
    return false; // 처음 하는 경우는 무조건 제동 말자
  }

  // 너무 짧게 설정되어 있으면 아래 조건문이 MTU 이하로 체크하는게 무의미하므로!
  fun_check(NetConfig::udp_packet_board_long_interval_sec >= 1);

  // 이렇게 해주지 않으면 long term이 막 끝난 직후의 송신량이 부정확하다는
  // 이유로 매 long term마다의 랙 유발
  if (total_bytes_ < NetConfig::min_send_speed) {
    return false;
  }

  const bool barking_needed = total_bytes_ > int32((absolute_time - last_clear_time_) * max_send_speed);
  return barking_needed;
}

// 총 보냈던 양을 누적한다.
void SendBrake::Accumulate(int32 byte_count) {
  total_bytes_ += byte_count;
}

void SendBrake::LongTick(double absolute_time) {
  const double long_interval = absolute_time - last_clear_time_;
  if (long_interval > 0) {
    last_clear_time_ = absolute_time;
    total_bytes_ = 0;
  }
}

SendBrake::SendBrake() {
  last_clear_time_ = 0;
  total_bytes_ = 0;
}

int32 SendBrake::GetMaxSendAmount(double absolute_time, double max_send_speed) {
  if (last_clear_time_ == 0) {
    last_clear_time_ = absolute_time;
  }

  // TCP 자체가 ack가 안오면 sender window slide가 더뎌지므로 자연 송신량 조절 기능이 되는 셈.
  // 따라서 굳이 송신량 조절기를 아직은 쓸 필요가 없다.
  return 1024*1024*100;

  // 이렇게 해주지 않으면 long term이 막 끝난 직후의 송신량이 부정확하다는 이유로 매 long term마다의 랙 유발
  //return MathBase::Max(NetConfig::min_send_speed, int32((absolute_time - last_clear_time_) * MathBase::Max(NetConfig::min_send_speed, max_send_speed)));
}

// defrag board의 경우 (addrport, packets) entry가 너무 자주 생성/소멸되는 경우 최근 수신속도 산출을 못했다.
// 그래서 이렇게 해서 일정시간 빈 엔트리라도 보유해야 한다.
bool RecentSpeedMeasurer::IsRemovingSafeForCalcSpeed(double absolute_time) {
  return (absolute_time - last_accum_time_) > (NetConfig::udp_packet_board_long_interval_sec * 3);
}

void RecentSpeedMeasurer::LongTick(double absolute_time) {
  if (last_long_interval_work_time_ == 0) {
    last_long_interval_work_time_ = absolute_time;
  }

  // 최근 수신속도 산출
  const double last_interval = absolute_time - last_long_interval_work_time_;
  if (last_interval == 0) {
    return;
  }

  if (last_interval > NetConfig::udp_packet_board_long_interval_sec * 0.2) {
    const double LastSpeed = double(last_interval_total_bytes_) / last_interval;
    recent_speed_ = MathBase::Lerp(recent_speed_, LastSpeed, 0.7);

    // Reset
    last_interval_total_bytes_ = 0;
    last_long_interval_work_time_ = absolute_time;
  } //else {
  //  fun_check(0);  // 너무 자주 호출되면 부정확한 결과가 나오므로
  //}
}

// 최대 송신가능속도를 산출한다.
void AllowedMaxSendSpeed::CalcNewValue(double recent_send_speed, double recent_receive_speed)
{
  // 수신속도가 0인 경우 '아직 못받았음'을 의미하므로 엄청 큰 값으로 재설정한다.
  if (recent_receive_speed == 0) {
    recent_receive_speed = VeryLargeValue;
  }

  // 송신 속도가 수신 속도보다 훨씬 크면 수신 속도만큼 최대송신가능속도를 제한. 그렇지 않으면 되레 올림
  // 0.5이어야 시간차로 생기는 '정작 수신이 받쳐주고 있는데 오판하는 사태'를 최소화
  if (recent_send_speed * 0.5 > recent_receive_speed) {
    value_ = recent_receive_speed * 0.8;
  } else {
    // 오버플로우 방지 감안해서 증가화
    value_ *= 2;
    value_ = MathBase::Min(value_, VeryLargeValue);
  }

  // 이렇게 해주지 않으면 long term이 막 끝난 직후의 송신량이 부정확하다는 이유로 매 long term마다의 랙 유발
  value_ = MathBase::Max(value_, (double)NetConfig::min_send_speed); //@todo MinSendSpeed가 uint32인게 맞는건가??

#ifdef UPDATE_TEST_STATS
  TestStats::test_allowed_max_send_speed = value_;
#endif
}


double TestStats::test_allowed_max_send_speed = 0;


AllowedMaxSendSpeed::AllowedMaxSendSpeed() {
  value_ = VeryLargeValue;
}

void RecentReceiveSpeedAtReceiverSide::LongTick(double absolute_time) {
  // 너무 오랫동안 '최근 수신속도'를 받지 못하는 상황이면 invalid로 바꾸도록 하자.
  if (last_clear_time_ == 0) {
    last_clear_time_ = absolute_time;
  }

  if ((absolute_time - last_clear_time_) > (NetConfig::udp_packet_board_long_interval_sec * 3)) {
    last_clear_time_ = absolute_time;
    value_ = 0;
  }
}

// INT_MAX로 바꿔도 안전한 값이어야 한다.
const double AllowedMaxSendSpeed::VeryLargeValue = (double)(int32_MAX - 1000000);

} // namespace net
} // namespace fun
