#include "fun/net/event_loop.h"

#include <boost/bind.hpp>
#include <iostream>

void print(fun::net::EventLoop* loop, int* count) {
  if (*count < 5) {
    std::cout << *count << "\n";
    ++(*count);

    loop->ScheduleAfter(1, boost::bind(print, loop, count));
  } else {
    loop->Quit();
  }
}

int main() {
  fun::net::EventLoop loop;
  int count = 0;
  // Note: loop.ScheduleEvery() is better for this use case.
  loop.ScheduleAfter(1, boost::bind(print, &loop, &count));
  loop.Loop();
  std::cout << "Final count is " << count << "\n";
}
