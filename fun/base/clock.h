#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"

namespace fun {

/**
 * A Clock stores a monotonic* clock value
 * with (theoretical) microseconds resolution.
 * Clocks can be compared with each other
 * and simple arithmetic is supported.
 *
 * [*] Note that Clock values are only monotonic if
 * the operating system provides a monotonic clock.
 * The IsMonotonic() function can be used to check whether
 * the system's clock is monotonic.
 *
 * Monotonic Clock is available on Windows, Linux, OS X
 * and on POSIX platforms supporting clock_gettime() with CLOCK_MONOTONIC.
 *
 * Clock values are relative to a system-dependent epoch time
 * (usually the system's startup time) and have no relation
 * to the time of day.
 */
class FUN_BASE_API Clock {
 public:
  using ValueType = int64;
  using DiffType = int64;

  static const ValueType MIN_CLOCK_VALUE;
  static const ValueType MAX_CLOCK_VALUE;

  /**
   * Creates a Clock with 0.
   */
  Clock();

  /**
   * Creates a Clock with the current system clock value.
   */
  explicit Clock(ForceInit_TAG);

  /**
   * Creates a Clock from the given clock value.
   */
  Clock(ValueType value);

  /**
   * Copy constructor.
   */
  Clock(const Clock& rhs);

  /**
   * Destroys the Clock.
   */
  ~Clock();

  Clock& operator = (const Clock& rhs);
  Clock& operator = (ValueType value);

  void Swap(Clock& other);

  static Clock Now();

  /**
   * Update the Clock with the current system clock.
   */
  void Update();

  bool operator == (const Clock& other) const;
  bool operator != (const Clock& other) const;
  bool operator >  (const Clock& other) const;
  bool operator >= (const Clock& other) const;
  bool operator <  (const Clock& other) const;
  bool operator <= (const Clock& other) const;

  Clock operator + (DiffType span) const;
  Clock operator - (DiffType span) const;
  DiffType operator - (const Clock& other) const;
  Clock& operator += (DiffType span);
  Clock& operator -= (DiffType span);

  /**
   * Returns the clock value expressed in microseconds
   * since the system-specific epoch time (usually system startup).
   */
  ValueType Microseconds() const;

  /**
   * Returns the clock value expressed in microseconds
   * since the system-specific epoch time (usually system startup).
   *
   * Same as Microseconds().
   */
  ValueType Raw() const;

  /**
   * Returns the time elapsed since the time denoted by
   * the Clock instance. Equivalent to Clock() - *this.
   */
  DiffType Elapsed() const;

  /**
   * Returns true if the given interval has passed
   * since the time denoted by the Clock instance.
   */
  bool IsElapsed(DiffType interval) const;

  //TODO 실제로 고정된 값을 반환하니까 상수로 선언해주어도 무방할듯...
  /**
   * Returns the resolution in units per second.
   * Since the Clock class has microsecond resolution,
   * the returned value is always 1000000.
   */
  static DiffType Resolution();

  /**
   * Returns the system's clock accuracy in microseconds.
   */
  static DiffType Accuracy();

  /**
   * Returns true if the system's clock is monotonic.
   */
  static bool IsMonotonic();

 private:
  ValueType value_;
};


//
// inlines
//

FUN_ALWAYS_INLINE Clock::Clock() : value_(0) {}

FUN_ALWAYS_INLINE Clock::Clock(ForceInit_TAG) {
  Update();
}

FUN_ALWAYS_INLINE Clock::Clock(ValueType value)
  : value_(value) {}

FUN_ALWAYS_INLINE Clock::Clock(const Clock& rhs)
  : value_(rhs.value_) {}

FUN_ALWAYS_INLINE Clock::~Clock() {
}

FUN_ALWAYS_INLINE Clock& Clock::operator = (const Clock& rhs) {
  value_ = rhs.value_;
  return *this;
}

FUN_ALWAYS_INLINE Clock& Clock::operator = (ValueType value) {
  value_ = value;
  return *this;
}

FUN_ALWAYS_INLINE void Clock::Swap(Clock& other) {
  fun::Swap(value_, other.value_);
}

FUN_ALWAYS_INLINE Clock Clock::Now() {
  return Clock(ForceInit);
}

FUN_ALWAYS_INLINE bool Clock::operator == (const Clock& other) const {
  return value_ == other.value_;
}

FUN_ALWAYS_INLINE bool Clock::operator != (const Clock& other) const {
  return value_ != other.value_;
}

FUN_ALWAYS_INLINE bool Clock::operator >  (const Clock& other) const {
  return value_ > other.value_;
}

FUN_ALWAYS_INLINE bool Clock::operator >= (const Clock& other) const {
  return value_ >= other.value_;
}

FUN_ALWAYS_INLINE bool Clock::operator <  (const Clock& other) const {
  return value_ < other.value_;
}

FUN_ALWAYS_INLINE bool Clock::operator <= (const Clock& other) const {
  return value_ <= other.value_;
}

FUN_ALWAYS_INLINE Clock Clock::operator + (Clock::DiffType span) const {
  return Clock(value_ + span);
}

FUN_ALWAYS_INLINE Clock Clock::operator - (Clock::DiffType span) const {
  return Clock(value_ - span);
}

FUN_ALWAYS_INLINE Clock::DiffType Clock::operator - (const Clock& other) const {
  return value_ - other.value_;
}

FUN_ALWAYS_INLINE Clock& Clock::operator += (Clock::DiffType span) {
  value_ += span;
  return *this;
}

FUN_ALWAYS_INLINE Clock& Clock::operator -= (Clock::DiffType span) {
  value_ -= span;
  return *this;
}

FUN_ALWAYS_INLINE Clock::ValueType Clock::Microseconds() const {
  return value_;
}

FUN_ALWAYS_INLINE Clock::DiffType Clock::Elapsed() const {
  Clock now(ForceInit);
  return now - *this;
}

FUN_ALWAYS_INLINE bool Clock::IsElapsed(Clock::DiffType interval) const {
  Clock now(ForceInit);
  DiffType span = now - *this;
  return span >= interval;
}

FUN_ALWAYS_INLINE Clock::DiffType Clock::Resolution() {
  return 1000000; // 10^6
}

FUN_ALWAYS_INLINE Clock::ValueType Clock::Raw() const {
  return value_;
}

} // namespace fun
