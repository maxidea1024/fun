#pragma once

#include "fun/base/base.h"
#include "fun/base/clock.h"

namespace fun {

/**
 * A simple facility to measure time intervals with microsecond resolution.
 */
class FUN_BASE_API Stopwatch {
 public:
  Stopwatch();
  ~Stopwatch();

  /**
   * Starts (or restarts) the stopwatch.
   */
  void Start();

  /**
   * Stops or pauses the stopwatch.
   */
  void Stop();

  /**
   * Resets the stopwatch.
   */
  void Reset();

  /**
   * Resets and starts the stopwatch.
   */
  void Restart();

  /**
   * Returns the elapsed time in microseconds
   * since the stopwatch started.
   */
  Clock::DiffType Elapsed() const;

  /**
   * Returns the number of seconds elapsed
   * since the stopwatch started.
   */
  double ElapsedSeconds() const;

  /**
   * Returns the Resolution of the stopwatch.
   */
  static Clock::ValueType Resolution();

 private:
  Stopwatch(const Stopwatch&);
  Stopwatch& operator=(const Stopwatch&);

  Clock start_;
  Clock::DiffType elapsed_;
  bool running_;
};

//
// inlines
//

FUN_ALWAYS_INLINE void Stopwatch::Start() {
  if (!running_) {
    start_.Update();
    running_ = true;
  }
}

FUN_ALWAYS_INLINE void Stopwatch::Stop() {
  if (running_) {
    Clock now(Clock::Now());
    elapsed_ += now - start_;
    running_ = false;
  }
}

FUN_ALWAYS_INLINE double Stopwatch::ElapsedSeconds() const {
  return (double)Elapsed() / (double)Resolution();
}

FUN_ALWAYS_INLINE Clock::ValueType Stopwatch::Resolution() {
  return Clock::Resolution();
}

}  // namespace fun
