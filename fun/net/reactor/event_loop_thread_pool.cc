#include "fun/net/reactor/event_loop_thread_pool.h"
#include "fun/net/reactor/event_loop.h"
#include "fun/net/reactor/event_loop_thread.h"

namespace fun {
namespace net {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop,
                                         const String& name)
    : base_loop_(base_loop),
      name_(name),
      started_(false),
      thread_count_(0),
      next_(0) {
  fun_check_ptr(base_loop);
}

EventLoopThreadPool::~EventLoopThreadPool() { fun_check(!started_); }

void EventLoopThreadPool::SetThreadCount(int32 thread_count) {
  // 시작하기 전에 호출해야함.
  fun_check(!started_);
  fun_check(thread_count >= 0);

  thread_count_ = thread_count;
}

void EventLoopThreadPool::Start(const ThreadInitCallback& cb) {
  base_loop_->AssertInLoopThread();

  fun_check(!started_);

  started_ = true;

  for (int32 i = 0; i < thread_count_; ++i) {
    char buf[name_.Len() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

    EventLoopThread* thread = new EventLoopThread(cb, buf);
    threads_.Add(thread);
    loops_.Add(thread->StartLoop());
  }

  if (thread_count_ == 0 && cb) {
    cb(base_loop_);
  }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
  base_loop_->AssertInLoopThread();
  fun_check(started_);

  EventLoop* loop = base_loop_;

  if (!loops_.IsEmpty()) {
    // round-robin
    loop = loops_[next_];
    next_ = (next_ + 1) % loops_.Count();
  }

  return loop;
}

// TODO 특정 해쉬값을 기준으로 균등하게 분할할 수 있는 기능이 필요해보이는데...
//차라리 특정 함수를 노출해주는게 좋으려나?
// uuid를 기준으로 분할해주는게 좋을까??
EventLoop* EventLoopThreadPool::GetLoopForHash(size_t hash) {
  base_loop_->AssertInLoopThread();
  fun_check(started_);

  EventLoop* loop = base_loop_;

  if (!loops_.IsEmpty()) {
    loop = loops_[hash % loops_.Count()];
  }

  return loop;
}

Array<EventLoop*> EventLoopThreadPool::GetAllLoops() const {
  base_loop_->AssertInLoopThread();
  fun_check(started_);

  if (loops_.IsEmpty()) {
    Array<EventLoop*> list;
    list.Add(base_loop_);
    return list;
  }

  return loops_;
}

}  // namespace net
}  // namespace fun
