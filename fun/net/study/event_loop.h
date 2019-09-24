#pragma once

namespace fun {
namespace net {


class EventLoop
{
  using TimeCallback = Function<void ()>;
  using Functor = Function<void ()>;

public:
  EventLoop();

  EventLoop(const EventLoop&) = delete;
  EventLoop& operator = (const EventLoop&) = delete;

  ~EventLoop();

  void Loop();

  void AssertInLoopThread()
  {
    if (!IsInLoopThread())
    {
      AssetNotInLoopThread();
    }
  }

  bool IsInLoopThread() const
  {
    return tid_ == this_thread::tid();
  }

  void Update(Channel* c);

  void Quit()
  {
    quit_ = true;

    WakeUp();
  }

  // Timer related
  void RunAt(const Timestamp& ts, const TimeCallback& cb);
  void RunAfter(double delay, const TimeCallback& cb);
  void RunEvery(double interval, const TimeCallback& cb);

  // Working queue
  void RunInLoop(Functor&);

private:
  typedef Array<Channel*> ChannelList;

  void DoPendingFunctors();
  void AbortNotInLoopThread();
  void WakeUp();
  void HandleRead();

  bool looping_;
  bool quit_;
  bool calling_pending_functors_;
  const pid_t tid_;

  UniquePtr<Poller> poller_;
  ChannelList active_channels_;

  TimerQeueue timers_;

  // for wakeup event-loop
  int wakeup_fd_;
  Channel wakeup_channel_;

  Mutex mutex_;
  Array<Functor> functors_;

  thread_local static EventLoop* loop_in_this_thread_;
};


} // namespace net
} // namespace fun
