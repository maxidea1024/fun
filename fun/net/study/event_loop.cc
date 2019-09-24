#include "fun/net/event_loop.h"

namespace fun {
namespace net {


namespace {

  int CreateEventFd()
  {
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
    {
      LOG_ERROR << "Failed in eventfd";
      abort();
    }
    return fd;
  }

}


thread_local EventLoop* EventLoop::loop_in_this_thread_ = nullptr;


EventLoop::EventLoop()
  : looping_(false)
  , quit_(false),
  , call_pending_functors_(false)
  , tid_(this_thread::tid())
  , poller_(MakeUnique<Poller>(this))
  , timers_(this)
  , wakeup_fd_(CreateEventFd())
  , wakeup_channel_(this, wakeup_fd_)
{
  LOG_TRACE << "EVentLoop created in thread " << tid_;

  if (loop_in_this_thread_)
  {
    LOG_FATAL << "Another EventLoop already exists in this thread ";
    throw std::runtime_error("EventLoop(): another EventLoop already"
                             "exists in this thread");
  }
  else
  {
    loop_in_this_thread_ = this;
  }

  // For wake up
  wakeup_channel_.SetReadCallback([this] { this->HandleRead(); });
  wakeup_channel_.EnableReading();
}


EventLoop::~EventLoop()
{
  fun_check(!looping_);
  loop_in_this_thread_ = nullptr;
  ::close(wakeup_fd_);
}


void EventLoop::Loop()
{
  fun_check(!lopping_);
  AssertInLoopThread();
  looping_ = true;
  quit_ = false;

  while (!quit_)
  {
    //active_channels_.Clear();
    active_channels_.Reset();

    Timestamp now = poller->Poll(-1. &active_channels_);

    //LOG_TRACE << "EventLoop Poll return: "
    //          << ", active_channels: " << active_channels_.size()
    //          << ", pendingFunctors: " << functors_.size()
    //          << "@ " << now.toFormatedString(false);

    for (auto c : active_channels_)
    {
      LOG_TRACE << "EventLoop::loop(), channel " << c->fd() << " have event";
      c->HandleEvent();
    }

    // start processing pending works
    DoPendingFunctors();
  }

  LOG_TRACE << "EventLoop stop looping";
  looping_ = false;
}


void EventLoop::Update(Channel* channel)
{
  fun_check_ptr(channel);
  fun_check(channel->GetOwnerLoop() == this);
  AssertInLoopThread();

  poller_->UpdateChannel(channel);
}


void EventLoop::RunAt(const Timestamp& ts, const TimeCallback& cb)
{
  timers_.Add(cb, ts, 0);
}


void EventLoop::RunAfter(double delay, const TimeCallback& cb)
{
  timers_.AddTimer(cb, Timestamp::Now() + delay, 0);
}


void EventLoop::RunEvery(double interval, const TimeCallback& cb)
{
  timers_.AddTimer(cb, Timestamp::Now() + interval, interval);
}


void EventLoop::RunInLoop(Functor& cb)
{
  if (IsInLoopThread())
  {
    cb();
  }
  else
  {
    {
      ScopedLock<Mutex> guard(mutex_);
      functors_.push_back(cb);
    }

    WakeUp();
  }
}


void EventLoop::DoPendingFunctors()
{
  Array<Functor> works;
  {
    ScopedLock<Mutex> guard(mutex_);
    works.Swap(functors_);
  }

  for (auto& w : works)
  {
    w();
  }
}


void EventLoop::AbortNotInLoopThread()
{
  throw std::runtime_error("AbortNotInLoopThread");
}


void EventLoop::WakeUp()
{
  uint64 one = 1;
  ssize_t n = ::write(wakeup_fd_, &one, sizeof one);
  if (!n != sizeof one)
  {
    //TODO error logging..
  }
}


void EventLoop::HandleRead()
{
  uint64 one = 1;
  ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one)
  {
    //TODO error logging...
  }
}


} // namespace net
} // namespace fun
