#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * 송수신량을 측정하는 기능은 없다. 다만 송수신량이 어느정도 이상 되면 보낼 양을 제한하도록,
 * 송신 속도를 제어하는 기능을 갖고 있다.
 * 그것도 long term 이하 단위로도!
 */
class SendBrake {
 public:
  SendBrake();

  void LongTick(double absolute_time);
  void Accumulate(int32 ByteLength);
  bool BrakeNeeded(double absolute_time, double max_send_speed);
  int32 GetMaxSendAmount(double absolute_time, double max_send_speed);

 private:
  int32 total_bytes_;
  double last_clear_time_;
};

/**
 * 송신측에서 사용됨
 */
class RecentSpeedMeasurer {
 public:
  RecentSpeedMeasurer()
    : last_interval_total_bytes_(0),
      last_long_interval_work_time_(0),
      recent_speed_(0),
      last_accum_time_(0) {}

  bool IsRemovingSafeForCalcSpeed(double absolute_time);

  void TouchFirstTime(double absolute_time) {
    last_accum_time_ = absolute_time;
  }

  void Accumulate(uint32 byte_count, double absolute_time) {
    last_interval_total_bytes_ += byte_count;
    last_accum_time_ = absolute_time;
  }

  void LongTick(double absolute_time);

  double GetRecentSpeed() const {
    return recent_speed_;
  }

 private:
  /** 최근 송수신속도 (바이트 단위) */
  double recent_speed_;
  /** 최근 송수신속도 산출용 */
  double last_interval_total_bytes_;
  /** 최근 송수신속도 산출용 */
  double last_long_interval_work_time_;
  /** 최근 송수신속도 산출용 */
  double last_accum_time_;
};

class AllowedMaxSendSpeed {
 public:
  AllowedMaxSendSpeed();

  double GetValue() const { return value_; }
  void CalcNewValue(double recent_send_speed, double recent_receive_speed);

 private:
  double value_;

  static const double VeryLargeValue;
};

/**
 * 수신측에서 사용됨
 */
class RecentReceiveSpeedAtReceiverSide {
 public:
  RecentReceiveSpeedAtReceiverSide()
    : value_(0), last_clear_time_(0) {}

  double GetValue() const { return value_; }
  void SetValue(double value) { value_ = value; }
  void LongTick(double absolute_time);

 private:
  /** 0이면 '수신량을 노티받은 적 없음' 의미 */
  double value_;
  double last_clear_time_;
};

} // namespace net
} // namespace fun
