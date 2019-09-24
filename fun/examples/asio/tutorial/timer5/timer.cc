#include "fun/base/mutex.h"
#include "fun/net/event_loop.h"
#include "fun/net/event_loop_thread.h"

#include <iostream>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

class Printer : Noncopyable {
 public:
  Printer(fun::net::EventLoop* loop1, fun::net::EventLoop* loop2)
    : loop1_(loop1)
    , loop2_(loop2)
    , count_(0) {
    loop1_->ScheduleAfter(1, boost::bind(&Printer::Print1, this));
    loop2_->ScheduleAfter(1, boost::bind(&Printer::Print2, this));
  }

  ~Printer() {
    std::cout << "Final count is " << count_ << "\n";
  }

  void Print1() {
    fun::ScopedLock guard(mutex_);
    if (count_ < 10) {
      std::cout << "Timer 1: " << count_ << "\n";
      ++count_;

      loop1_->ScheduleAfter(1, boost::bind(&Printer::Print1, this));
    }
    else {
      loop1_->Quit();
    }
  }

  void Print2() {
    fun::ScopedLock guard(mutex_);
    if (count_ < 10) {
      std::cout << "Timer 2: " << count_ << "\n";
      ++count_;

      loop2_->ScheduleAfter(1, boost::bind(&Printer::Print2, this));
    }
    else {
      loop2_->Quit();
    }
  }

 private:
  fun::MutexLock mutex_;
  fun::net::EventLoop* loop1_;
  fun::net::EventLoop* loop2_;
  int count_;
};


int main() {
  fun::SharedPtr<Printer> printer;  // make sure printer lives longer than loops, to avoid
                                    // race condition of calling Print2() on destructed object.
  fun::net::EventLoop loop;
  fun::net::EventLoopThread loop_thread;
  fun::net::EventLoop* loop_in_another_thread = loop_thread.StartLoop();
  printer.Reset(new Printer(&loop, loop_in_another_thread));
  loop.Loop();
}
