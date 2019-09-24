#include "fun/net/timer_queue.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace fun {
namespace net {

namespace {

int CreateTimerFd() {
  int timer_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timer_fd < 0) {
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timer_fd;
}

struct timespec HowMuchTimeFromNow(const Timestamp& when) {
  int64_t microseconds =
      when.microSecondsSinceEpoch() - Timestamp::Now().microSecondsSinceEpoch();
  if (microseconds < 100) {
    microseconds = 100;
  }

  struct timespec ts;
  ts.tv_sec =
      static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

void ReadTimerFd(int timer_fd, const Timestamp& now) {
  uint64_t howmany;
  ssize_t n = ::read(timer_fd, &howmany, sizeof howmany);
  LOG_TRACE << "TimerQueue::HandleRead() " << howmany << " at "
            << now.toString();
  if (n != sizeof howmany) {
    LOG_ERROR << "TimerQueue::HandleRead() reads " << n
              << " bytes instead of 8";
  }
}

void ResetTimerFd(int timer_fd, const Timestamp& expiration) {
  // wake up loop by timerfd_settime()
  struct itimerspec new_value;
  struct itimerspec old_value;
  bzero(&new_value, sizeof new_value);
  bzero(&old_value, sizeof old_value);
  new_value.it_value = HowMuchTimeFromNow(expiration);
  int rc = ::timerfd_settime(timer_fd, 0, &new_value, &old_value);
  if (rc) {
    LOG_SYSERR << "timerfd_settime()";
  }
}

}  // namespace

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timer_fd_(CreateTimerFd()),
      timer_fd_channel_(loop, timer_fd_),
      timers_(),
      calling_expired_timers_(false) {
  timer_fd_channel_.SetReadCallback([this]() { HandleRead(); });
  timer_fd_channel_.EnableReading();
}

TimerQueue::~TimerQueue() {
  timer_fd_channel_.DisableAll();
  timer_fd_channel_.Remove();
  ::close(timer_fd_);

  // do not remove channel, since we're in EventLoop::dtor();
  // for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it) {
  //  delete it->second;
  //}

  // TODO pooling을 해주는게 좋을까...
  for (auto& pair : timers_) {
    delete pair.second;
  }
}

TimerId TimerQueue::AddTimer(const TimerCallback& cb, const Timestamp& when,
                             double interval) {
  Timer* timer = new Timer(cb, when, interval);
  loop_->RunInLoop([this, timer]() { AddTimerInLoop(timer); });
  return TimerId(timer, timer->GetSequence());
}

TimerId TimerQueue::AddTimer(TimerCallback&& cb, const Timestamp& when,
                             double interval) {
  Timer* timer = new Timer(MoveTemp(cb), when, interval);
  loop_->RunInLoop([this, timer]() { AddTimerInLoop(timer); });
  return TimerId(timer, timer->GetSequence());
}

void TimerQueue::Cancel(TimerId timer_id) {
  loop_->RunInLoop([this, timer_id]() { CancelInLoop(timer_id); });
}

void TimerQueue::AddTimerInLoop(Timer* timer) {
  loop_->AssertInLoopThread();

  const bool earliest_changed = Insert(timer);

  // 가장 일찍 expired되는 시간이 변경된 경우에는 timer_fd를 재설정해주어야함.
  if (earliest_changed) {
    ResetTimerFd(timer_fd_, timer->GetExpiration());
  }
}

void TimerQueue::CancelInLoop(TimerId timer_id) {
  loop_->AssertInLoopThread();

  fun_check(timers_.Count() == active_timers_.Count());

  ActiveTimer timer(timer_id.timer_, timer_id.sequence_);
  ActiveTimerSet::iterator it = active_timers_.find(timer);
  if (it != active_timers_.end()) {
    size_t n = timers_.erase(Entry(it->first->GetExpiration(), it->first));
    fun_check(n == 1);
    (void)n;
    delete it->first;  // FIXME: no delete please
    active_timers_.erase(it);
  } else if (calling_expired_timers_) {
    cancelling_timers_.Insert(timer);
  }

  fun_check(timers_.Count() == active_timers_.Count());
}

// timer_fd가 expired되었을 경우에 호출됨.
void TimerQueue::HandleRead() {
  loop_->AssertInLoopThread();

  Timestamp now(Timestamp::Now());
  ReadTimerFd(timer_fd_, now);

  // TODO 매번 목록을 만들지말고, 따로 가지고 있는게 좋을듯...
  std::vector<Entry> expireds = GetExpireds(now);

  calling_expired_timers_ = true;
  cancelling_timers_.clear();
  // safe to callback outside critical section
  // for (std::vector<Entry>::iterator it = expireds.begin(); it !=
  // expireds.end(); ++it) {
  //  it->second->Run();
  //}
  for (auto& expired : expireds) {
    expired.second->Run();
  }
  calling_expired_timers_ = false;

  // 반복되어서 실행해야할 타이머들은 다시 설정해줌.
  Reset(expireds, now);
}

std::vector<TimerQueue::Entry> TimerQueue::GetExpireds(const Timestamp& now) {
  fun_check(timers_.Count() == active_timers_.Count());

  std::vector<Entry> expireds;
  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  TimerList::iterator end = timers_.lower_bound(sentry);
  fun_check(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, back_inserter(expireds));
  timers_.erase(timers_.begin(), end);

  for (std::vector<Entry>::iterator it = expireds.begin(); it != expireds.end();
       ++it) {
    ActiveTimer timer(it->second, it->second->GetSequence());
    size_t n = active_timers_.erase(timer);
    fun_check(n == 1);
    (void)n;
  }

  fun_check(timers_.Count() == active_timers_.Count());

  return expireds;
}

void TimerQueue::Reset(const std::vector<Entry>& expireds,
                       const Timestamp& now) {
  Timestamp next_expire;

  for (std::vector<Entry>::const_iterator it = expireds.begin();
       it != expireds.end(); ++it) {
    ActiveTimer timer(it->second, it->second->GetSequence());

    if (it->second->ShouldRepeat() &&
        cancelling_timers_.find(timer) ==
            cancelling_timers_
                .end()) {  // 실행중일때 cancel한 경우에는 재시작하면 안됨.
      it->second->Restart(now);
      Insert(it->second);
    } else {
      // FIXME move to a free list
      delete it->second;  // FIXME: no delete please
    }
  }

  if (!timers_.IsEmpty()) {
    next_expire = timers_.begin()->second->GetExpiration();
  }

  if (next_expire.IsValid()) {
    ResetTimerFd(timer_fd_, next_expire);
  }
}

bool TimerQueue::Insert(Timer* timer) {
  loop_->AssertInLoopThread();

  fun_check(timers_.Count() == active_timers_.Count());

  bool earliest_changed = false;

  Timestamp when = timer->GetExpiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliest_changed = true;
  }

  {
    std::pair<TimerList::iterator, bool> result =
        timers_.Insert(Entry(when, timer));
    // fun_check(result.second); (void)result;
  }

  {
    std::pair<ActiveTimerSet::iterator, bool> result =
        active_timers_.Insert(ActiveTimer(timer, timer->GetSequence()));
    // fun_check(result.second); (void)result;
  }

  fun_check(timers_.Count() == active_timers_.Count());

  return earliest_changed;
}

}  // namespace net
}  // namespace fun
