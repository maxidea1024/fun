#pragma once

namespace fun {
namespace net {


class EventLoopThread
{
  using InitialCallback = Function<void (EventLoop*)>;
  
public:
  EventLoopThread(const InitialCallback& cb = InitialCallback())
    : loop_(nullptr)
    , callback_(cb)
  {
  }

  EventLoopThread(const EventLoopThread&) = delete;
  EventLoopThread& operator = (const EventLoopThread&) = delete;
  
  ~EventLoopThread();
  
  void Start();
  
  EventLoop* GetEventLoop();
  
private:
  Mutex mutex_;
  EventLoop* loop_;
  InitialCallback callback_;
  Thread thread_;
};


} // namespace net
} // namespace fun
