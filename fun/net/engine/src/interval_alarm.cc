#include "fun/net/net.h"
#include "fun/net/interval_alarm.h"

namespace fun {
namespace net {

IntervalAlaram::IntervalAlaram(double interval_sec) {
  fun_check(interval_sec > 0.0);
  interval_ = interval_sec;

  Reset();
}

bool IntervalAlaram::TakeElapsedTime(double elapsed_time) {
  // Subtract cool-time
  cooltime_ -= elapsed_time;

  // Check whether time to do.
  time_to_do_ = cooltime_ < 0;

  // Reset
  if (cooltime_ < 0) {
    cooltime_ += interval_;
  }

  return time_to_do_;
}

void IntervalAlaram::SetInterval(double interval_sec) {
  fun_check(interval_sec > 0.0);
  if (interval_sec <= 0) {
    throw InvalidArgumentException();
  }

  //@todo 재미있는 요소가 있을듯? (분석해보면 좋을듯도..)
  // 신구 interval의 비율에 따라 현재 쿨타임 또한 조정한다.
  const double ratio = interval_sec / interval_;
  cooltime_ *= ratio;
  interval_ = interval_sec;
}

void IntervalAlaram::Reset() {
  cooltime_ = interval_ * 0.3;
  time_to_do_ = false;
}

} // namespace net
} // namespace fun
