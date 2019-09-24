#pragma once

namespace fun {
namespace net {

class IntervalAlaram {
public:
  IntervalAlaram(double interval_sec);

  bool TakeElapsedTime(double elapsed_time);
  bool IsTimeToDo() const { return time_to_do_; }
  void SetInterval(double interval_sec);
  void Reset();

private:
  double cooltime_;
  double interval_;
  bool time_to_do_;
};

} // namespace net
} // namespace fun
