#include "fun/net/event_loop.h"

using namespace fun;
using namespace fun::net;

int main() {
  EventLoop loop;
  loop.Loop();
}
