#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {
namespace reactor {

/**
Internal class for timer event.
*/
class Timer : Noncopyable {
 public:
  Timer(const TimerCallback& cb, const Timestamp& when, double interval)
    : callback_(cb),
      expiration_(when),
      interval_(interval),
      sequence_(s_created_count_.IncrementAndGet()) {}

  Timer(TimerCallback&& cb, const Timestamp& when, double interval)
    : callback_(MoveTemp(cb)),
      expiration_(when),
      interval_(interval),
      sequence_(s_created_count_.IncrementAndGet()) {}

  void Run() const {
    //TODO 이벤트 트래킹을 해주는게 좋을듯...??
    callback_();
  }

  Timestamp GetExpiration() const {
    return expiration_;
  }

  bool ShouldRepeat() const {
    return interval_ > 0;
  }

  int64 GetSequence() const {
    return sequence_;
  }

  void Restart(const Timestamp& now) {
    fun_check(ShouldRepeat());
    expiration_ = AddTime(now, interval_);
  }

  static int64 GetCreatedCount() {
    return s_created_count_.get();
  }

 private:
  const TimerCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const int64 sequence_;

  static AtomicInt64 s_created_count_;
};

} // namespace reactor
} // namespace net
} // namespace fun
