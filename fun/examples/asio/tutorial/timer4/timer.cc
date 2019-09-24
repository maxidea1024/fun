#include "fun/net/event_loop.h"

#include <iostream>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

class Printer : Noncopyable {
 public:
  Printer(fun::net::EventLoop* loop)
    : loop_(loop)
    , count_(0) {
    // Note: loop.ScheduleEvery() is better for this use case.
    loop_->ScheduleAfter(1, boost::bind(&Printer::Print, this));
  }

  ~Printer() {
    std::cout << "Final count is " << count_ << "\n";
  }

  void Print() {
    if (count_ < 5) {
      std::cout << count_ << "\n";
      ++count_;

      loop_->ScheduleAfter(1, boost::bind(&Printer::Print, this));
    }
    else {
      loop_->Quit();
    }
  }

 private:
  fun::net::EventLoop* loop_;
  int count_;
};

int main() {
  fun::net::EventLoop loop;
  Printer printer(&loop);
  loop.Loop();
}
