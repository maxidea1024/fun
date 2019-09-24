#include "fun/base/mutex.h"
#include "fun/net/event_loop.h"
#include "fun/net/event_loop_thread.h"

#include <stdio.h>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

//
// Minimize locking
//

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
    // cout is not thread safe
    //std::cout << "Final count is " << count_ << "\n";
    printf("Final count is %d\n", count_);
  }

  void Print1() {
    bool should_quit = false;
    int count = 0;
    {
      fun::ScopedLock guard(mutex_);
      if (count_ < 10) {
        count = count_;
        ++count_;
      }
      else {
        should_quit = true;
      }
    }

    // out of lock
    if (should_quit) {
      // printf("loop1_->Quit()\n");
      loop1_->Quit();
    }
    else {
      // cout is not thread safe
      //std::cout << "Timer 1: " << count << "\n";
      printf("Timer 1: %d\n", count);
      loop1_->ScheduleAfter(1, boost::bind(&Printer::Print1, this));
    }
  }

  void Print2() {
    bool should_quit = false;
    int count = 0;
    {
      fun::ScopedLock guard(mutex_);
      if (count_ < 10) {
        count = count_;
        ++count_;
      }
      else {
        should_quit = true;
      }
    }

    // out of lock
    if (should_quit) {
      // printf("loop2_->Quit()\n");
      loop2_->Quit();
    }
    else {
      // cout is not thread safe
      //std::cout << "Timer 2: " << count << "\n";
      printf("Timer 2: %d\n", count);
      loop2_->ScheduleAfter(1, boost::bind(&Printer::Print2, this));
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
