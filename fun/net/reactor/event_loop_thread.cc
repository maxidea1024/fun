#include "fun/net/reactor/event_loop_thread.h"
#include "fun/net/reactor/event_loop.h"
#include "fun/base/scoped_lock.h"

namespace fun {
namespace net {

EventLoopThread::EventLoopThread( const ThreadInitCallback& cb,
                                  const String& name)
  : loop_(nullptr),
    exiting_(false),
    thread_(ThreadFunc, name),
    mutex_(),
    cond_(mutex_),
    callback_(cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;

  if (loop_) {
    loop_->Quit();
    thread_.Join();
  }
}

EventLoop* EventLoopThread::StartLoop() {
  fun_check(!thread_.Started());
  thread_.Start();

  {
    ScopedLock<Mutex> guard(mutex_);
    while (loop_ == nullptr) {
      cond_.Wait();
    }
  }

  return loop_;
}

void EventLoopThread::ThreadFunc() {
  EventLoop loop;

  if (callback_) {
    callback(&loop);
  }

  {
    ScopedLock guard(mutex_);
    loop_ = &loop;
    cond_.Notify();
  }

  loop.Loop();
  loop_ = nullptr;
}

} // namespace net
} // namespace fun
