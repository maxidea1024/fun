#pragma once

#include "fun/base/condition.h"
#include "fun/base/function.h"
#include "fun/base/mutex.h"
#include "fun/base/thread.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * TODO
 */
class EventLoopThread : Noncopyable {
 public:
  typedef Function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const String& name = String());

  ~EventLoopThread();

  EventLoop* StartLoop();

 private:
  void ThreadFunc();

  EventLoop* loop_;
  bool exiting_;
  Thread thread_;
  Mutex mutex_;
  Condition cond_;
  ThreadInitCallback callback_;
};

}  // namespace net
}  // namespace fun
