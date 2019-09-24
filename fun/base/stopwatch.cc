#include "fun/base/stopwatch.h"

namespace fun {

Stopwatch::Stopwatch() : elapsed_(0), running_(false) {}

Stopwatch::~Stopwatch() {}

Clock::DiffType Stopwatch::Elapsed() const {
  if (running_) {
    Clock now(Clock::Now());
    return elapsed_ + (now - start_);
  } else {
    return elapsed_;
  }
}

void Stopwatch::Reset() {
  elapsed_ = 0;
  running_ = false;
}

void Stopwatch::Restart() {
  elapsed_ = 0;
  start_.Update();
  running_ = true;
}

} // namespace fun
