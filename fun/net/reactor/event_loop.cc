#include "fun/net/reactor/event_loop.h"

namespace fun {
namespace net {

// TLS variable
thread_local static EventLoop* loop_in_this_thread_ = nullptr;

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      events_handling_(false),
      calling_pending_functors_(false),
      iteration_(0),
      tid_(Thread::CurrentTid()),
      poller_(Poller::Create(this)),
      timer_queue_(new TimerQueue(this)),
      wakeup_fd_(CreateEventfd()),
      wakeup_channel_(new Channel(this, wakeup_fd_)),
      current_active_channel_(nullptr) {
  if (loop_in_this_thread_) {
    // event loop 객체는 thread당 하나만 있어야함.
    // 예외를 던지던지 panic.
  }

  loop_in_this_thread_ = this;

  wakeup_channel_->SetReadCallback([this]() { HandleRead(); });
  wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop() {
  wakeup_channel_->DisableAll();
  wakeup_channel_->Remove();
  close(wakeup_fd_);
  loop_in_this_thread_ = nullptr;
}

void EventLoop::Loop() {
  fun_check(!looping_);
  AssertInLoopThread();

  looping_ = true;
  quit_ = false;

  while (!quit_) {
    active_channels_.Reset();  // clear but keep capacity

    poller_return_time_ = poller_->Poll(kPollTimeMs, &active_channels_);
    iteration_++;

    events_handling_ = true;
    for (auto active_channel : active_channels_) {
      current_active_channel_ = active_channel;
      current_active_channel_->HandleEvent(poller_return_time_);
    }
    current_active_channel_ = nullptr;
    events_handling_ = false;

    ProcessPendingFunctors();
  }

  looping_ = false;
}

// TODO 상황별 검토를 좀 해봐야할듯함...

#if 0

int EventLoop::Poll(int timeout_msecs) {
  fun_check(!looping_);
  AssertInLoopThread();

  looping_ = true;
  quit_ = false;

  while (!quit_) {
    active_channels_.Reset(); // clear but keep capacity

    poller_return_time_ = poller_->Poll(timeout_msecs, &active_channels_);
    ++iteration_;

    events_handling_ = true;
    for (auto active_channel : active_channels_) {
      current_active_channel_ = active_channel;
      current_active_channel_->HandleEvent(poller_return_time_);
      current_active_channel_ = nullptr;
    }
    events_handling_ = false;

    ProcessPendingFunctors();
  }

  return 0;
}

#endif

void EventLoop::Quit() {
  quit_ = true;

  if (!IsInLoopThread()) {
    WakeUp();
  }
}

void EventLoop::RunInLoop(const Functor& func) {
  if (IsInLoopThread()) {
    func();
  } else {
    QueueInLoop(func);
  }
}

void EventLoop::QueueInLoop(const Functor& func) {
  {
    ScopedLock<FastMutex> guard(mutex_);
    pending_functors_.Add(func);
  }

  if (!IsInLoopThread() || calling_pending_functors_) {
    // 자, 할일이 있으니 어서 일어나서 일해라...
    WakeUp();
  }
}

void EventLoop::RunInLoop(const Functor&& func) {
  if (IsInLoopThread()) {
    func();
  } else {
    QueueInLoop(MoveTemp(func));
  }
}

void EventLoop::QueueInLoop(const Functor&& func) {
  {
    ScopedLock<FastMutex> guard(mutex_);
    pending_functors_.Add(MoveTemp(func));
  }

  if (!IsInLoopThread() || calling_pending_functors_) {
    // 자, 할일이 있으니 어서 일어나서 일해라...
    WakeUp();
  }
}

int32 EventLoop::GetEnqueuedCount() const {
  ScopedLock<FastMutex> guard(mutex_);
  return pending_functors_.Count();
}

TimerId EventLoop::ExpireAt(const Timestamp& time,
                            const TimerCallback& callback) {
  return timer_queue_->AddTimer(callback, time, 0.0);
}

TimerId EventLoop::ExpireAfter(double delay, const TimerCallback& callback) {
  Timestamp time(AddTime(Timestamp::Now(), delay));
  return ExpireAt(time, callback);
}

TimerId EventLoop::ExpireRepeatedly(double interval,
                                    const TimerCallback& callback) {
  Timestamp time(AddTime(Timestamp::Now(), interval));
  return timer_queue_->AddTimer(callback, time, interval);
}

TimerId EventLoop::ExpireAt(const Timestamp& time,
                            const TimerCallback&& callback) {
  return timer_queue_->AddTimer(MoveTemp(callback), time, 0.0);
}

TimerId EventLoop::ExpireAfter(double delay, const TimerCallback&& callback) {
  Timestamp time(AddTime(Timestamp::Now(), delay));
  return ExpireAt(time, MoveTemp(callback));
}

TimerId EventLoop::ExpireRepeatedly(double interval,
                                    const TimerCallback&& callback) {
  Timestamp time(AddTime(Timestamp::Now(), interval));
  return timer_queue_->AddTimer(MoveTemp(callback), time, interval);
}

void EventLoop::CancelSchedule(TimerId timer_id) {
  timer_queue_->Cancel(timer_id);
}

// poller를 깨우는 용도.
// IO syscall이 과하게 발생하지 않을까 싶은데...??
void EventLoop::WakeUp() {
  uint64 one = 1;
  write(wakeup_fd_, &one, sizeof one);
}

void EventLoop::HandleRead() {
  uint64 one;
  read(wakeup_fd_, &one, sizeof one);
}

void EventLoop::UpdateChannel(Channel* channel) {
  fun_check_ptr(channel);
  fun_check(channel->owner_loop_ == this);
  AssertInLoopThread();

  poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel) {
  fun_check_ptr(channel);
  fun_check(channel->owner_loop_ == this);
  AssertInLoopThread();

  if (events_handling_) {
    // TODO
  }

  poller_->RemoveChannel(channel);
}

bool EventLoop::HasChannel(Channel* channel) {
  fun_check_ptr(channel);
  fun_check(channel->owner_loop_ == this);
  AssertInLoopThread();

  return poller_->HasChannel(channel);
}

EventLoop* EventLoop::GetEventLoopOfCurrentThread() {
  return loop_in_this_thread_;
}

void EventLoop::AbortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in tid = " << tid_
            << ", current thread id = " << Thread::CurrentTid();
}

void EventLoop::ProcessPendingFunctors() {
  Array<Functor>> working_set;

  // task들을 호출중임을 표시 설정.
  calling_pending_functors_ = true;

  {
    ScopedLock<FastMutex> guard(mutex_);
    working_set.Swap(pending_functors_);
  }

  for (int32 i = 0; i < working_set.Count(); ++i) {
    working_set[i]();
  }

  // task들을 호출중임을 표시 해제.
  calling_pending_functors_ = false;
}

void EventLoop::PrintActiveChannels() const {
  for (const auto& active_channel : active_channels_) {
    LOG_TRACE << "{" << active_channel->ReventsToString() << "} ";
  }
}

}  // namespace net
}  // namespace fun
