#include "fun/net/event_loop.h"

#include <iostream>

void Print() { std::cout << "Hello, world!\n"; }

int main() {
  fun::net::EventLoop loop;
  loop.ScheduleAfter(5, Print);
  loop.Loop();
}
