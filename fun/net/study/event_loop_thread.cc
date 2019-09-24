#include "fun/net/event_loop_thread.h"
#include "fun/net/event_loop.h"
#include <future>

namespace fun {
namespace net {


EventLoopThread::~EventLoopThread()
{
  if (thread_.Joinable())
  {
    ScopedLock<Mutex> guard(mutex_);
    if (loop_)
    {
      loop_->Quit();
    }

    thread_.Join();
  }
}


void EventLoopThread::Start()
{
  std::promise<void> promise;
  thread_ = std::thread([&]() {
      EventLoop loop;
      {
        ScopedLock<Mutex> guard(mutex_);
        loop_ = &loop;
      }
      promise.set_value();

      if (callback_)
      {
        callback_(&loop);
      }

      loop.Loop();
  });

  promise.get_future().wait();
}


EventLoop* EventLoopThread::GetEventLoop()
{
  ScopedLock<Mutex> guard(mutex_);
  return loop_;
}


} // namespace net
} // namespace fun
