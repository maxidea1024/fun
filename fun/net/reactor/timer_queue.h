#pragma once

#include "fun/base/container/array.h"
#include "fun/base/container/pair.h"
#include "fun/base/container/set.h"
#include "fun/base/timestamp.h"
#include "fun/net/net.h"

namespace fun {
namespace net {
namespace reactor {

class Timer;

/**
 * A best efforts timer queue.
 * No guarantee that the callback will be on time.
 */
class TimerQueue : Noncopyable {
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  /**
   * Schedules the callback to be run at given time,
   * repeats if @c interval > 0.0.
   *
   * Must be thread safe. Usually be called from other threads.
   */
  TimerId AddTimer(const TimerCallback& cb, const Timestamp& when,
                   double interval);

  TimerId AddTimer(TimerCallback&& cb, const Timestamp& when, double interval);

  void Cancel(TimerId timer_id);

 private:
  // 순서가 중요하므로, ordered_set을 사용해야함!!
  // 이부분을 수정할 방법이 없으려나...

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void AddTimerInLoop(Timer* timer);
  void CancelInLoop(TimerId timer_id);
  // called when timerfd alarms
  void HandleRead();
  // move out all expired timers
  std::vector<Entry> GetExpireds(const Timestamp& now);
  void Reset(const std::vector<Entry>& expireds, const Timestamp& now);

  bool Insert(Timer* timer);

  EventLoop* loop_;
  const int timer_fd_;
  Channel timer_fd_channel_;
  // Timer list sorted by expiration
  TimerList timers_;

  // for Cancel()
  ActiveTimerSet active_timers_;
  bool calling_expired_timers_;  // atomic
  ActiveTimerSet cancelling_timers_;
};

}  // namespace reactor
}  // namespace net
}  // namespace fun
