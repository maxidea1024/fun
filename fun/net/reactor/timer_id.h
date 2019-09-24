#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {
namespace reactor {

class Timer;

/**
An opaque identifier, for canceling timer.
*/
class TimerId {
 public:
  TimerId() : timer_(nullptr), sequence_(0) {}

  TimerId(Timer* timer, int64 sequence)
    : timer_(timer), sequence_(sequence) {}

  TimerId(const TimerId&) = default;
  TimerId& operator = (const TimerId&) = default;
  ~TimerId() = default;

 private:
  friend class TimerQueue;

  Timer* timer_;
  int64 sequence_;
};

} // namespace reactor
} // namespace net
} // namespace fun
