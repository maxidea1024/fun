#pragma once

#include "fun/base/container/array.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/timestamp.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * Reactor, at most one per thread.
 */
class EventLoop : Noncopyable {
 public:
  typedef Function<void()> Functor;

  EventLoop();
  ~EventLoop();

  /**
   * Loops forever.
   *
   * Must be called in the same thread as creation of the object.
   */
  void Loop();

  // TODO 타임아웃을 주고, Polling 형태로 동작할 수 있도록 할수 있어야함.
  // int Pool(int timeout_msecs);

  void Quit();

  Timestamp PollReturnTime() const { return poll_return_time_; }

  int64 Iteration() const { return iteration_; }

  void RunInLoop(const Functor& f);
  void QueueInLoop(const Functor& f);

  void RunInLoop(const Functor&& f);
  void QueueInLoop(const Functor&& f);

  int32 GetEnqueuedCount() const;

  //
  // Timer
  //

  TimerId ScheduleAt(const Timestamp& time, const TimerCallback& callback);
  TimerId ScheduleAfter(double delay, const TimerCallback& callback);
  TimerId ScheduleEvery(double interval, const TimerCallback& callback);
  void CancelSchedule(TimerId timer_id);

  void WakeUp();
  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);
  bool HasChannel(Channel* channel);

  void AssertInLoopThread() {
    if (!IsInLoopThread()) {
      AbortNotInLoopThread();
    }
  }

  bool IsInLoopThread() const { return tid_ == Thread::CurrentTid(); }

  bool EventHandling() const { return events_handling_; }

  // TODO tag or context

  static EventLoop* GetEventLoopOfCurrentThread();

 private:
  void AbortNotInLoopThread();
  void HandleRead();  // for wakeup
  void ProcessPendingFunctors();

  void PrintActiveChannels() const;  // for debugging

  typedef Array<Channel*> ChannelList;

  bool looping_;
  bool quit_;
  bool events_handling_;
  bool calling_pending_functors_;
  int64 iteration_;
  pid_t tid_;
  Timestamp poll_return_time_;
  SharedPtr<Poller> poller_;
  SharedPtr<TimerQueue> timer_queue_;
  int wakeup_fd_;
  SharedPtr<Channel> wakeup_channel_;

  intptr_t tag_;

  ChannelList active_channels_;
  Channel* current_active_channel_;

  Mutex mutex_;
  Array<Functor> pending_functors_;
};

}  // namespace net
}  // namespace fun
