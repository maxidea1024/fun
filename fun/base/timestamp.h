#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h" // HashOf
#include <ctime> // std::time_t

//TODO hash...

namespace fun {

class Timespan;

/**
 * A Timestamp stores a monotonic* time value
 * with (theoretical) microseconds resolution.
 * Timestamps can be compared with each other
 * and simple arithmetic is supported.
 *
 * [*] Note that Timestamp values are only monotonic as
 * long as the systems's clock is monotonic as well
 * (and not, e.g. set back due to time synchronization
 * or other reasons).
 *
 * Timestamps are UTC (Coordinated Universal Time)
 * based and thus independent of the timezone
 * in effect on the system.
 *
 * The internal reference time is the Unix epoch,
 * midnight, January 1, 1970.
 */
class FUN_BASE_API Timestamp {
 public:
  typedef int64 TimeVal;
  typedef int64 UtcTimeVal;
  typedef int64 TimeDiff;

  /**
   * Minimum timestamp value.
   */
  static const TimeVal TIMEVAL_MIN;
  /**
   * Maximum timestamp value.
   */
  static const TimeVal TIMEVAL_MAX;

  /**
   * Creates a timestamp with the current time.
   */
  Timestamp();

  /**
   * Creates a timestamp with the current time.
   */
  explicit Timestamp(ForceInit_TAG);

  /**
   * Creates a timestamp from the given time value
   * (microseconds since midnight, January 1, 1970).
   */
  Timestamp(TimeVal tv);

  /**
   * Copy constructor.
   */
  Timestamp(const Timestamp& rhs);

  /**
   * Destroys the timestamp
   */
  ~Timestamp();

  Timestamp& operator = (const Timestamp& other);
  Timestamp& operator = (TimeVal tv);

  void Swap(Timestamp& other);

  void Update();

  static Timestamp Now();

  bool operator == (const Timestamp& other) const;
  bool operator != (const Timestamp& other) const;
  bool operator >  (const Timestamp& other) const;
  bool operator >= (const Timestamp& other) const;
  bool operator <  (const Timestamp& other) const;
  bool operator <= (const Timestamp& other) const;

  Timestamp  operator +  (TimeDiff d) const;
  Timestamp  operator +  (const Timespan& span) const;
  Timestamp  operator -  (TimeDiff d) const;
  Timestamp  operator -  (const Timespan& span) const;
  TimeDiff   operator -  (const Timestamp& ts) const;
  Timestamp& operator += (TimeDiff d);
  Timestamp& operator += (const Timespan& span);
  Timestamp& operator -= (TimeDiff d);
  Timestamp& operator -= (const Timespan& span);

  /**
   * Returns the timestamp expressed in time_t.
   * time_t base time is midnight, January 1, 1970.
   * Resolution is one second.
   */
  std::time_t EpochTime() const;

  /**
   * Returns the timestamp expressed in UTC-based
   * time. UTC base time is midnight, October 15, 1582.
   * Resolution is 100 nanoseconds.
   */
  UtcTimeVal UtcTime() const;

  /**
   * Returns the timestamp expressed in microseconds
   * since the Unix epoch, midnight, January 1, 1970.
   */
  TimeVal EpochMicroseconds() const;

  /**
   * Returns the time elapsed since the time denoted by
   * the timestamp. Equivalent to Timestamp() - *this.
   */
  TimeDiff Elapsed() const;

  /**
   * Returns true if the given interval has passed
   * since the time denoted by the timestamp.
   */
  bool IsElapsed(TimeDiff interval) const;

  /**
   * Returns the raw time value.
   *
   * Same as EpochMicroseconds().
   */
  TimeVal Raw() const;

  /**
   * Creates a timestamp from a std::time_t.
   */
  static Timestamp FromEpochTime(std::time_t t);

  /**
   * Creates a timestamp from a UTC time value
   * (100 nanosecond intervals since midnight,
   * October 15, 1582).
   */
  static Timestamp FromUtcTime(UtcTimeVal val);

  /**
   * Returns the resolution in units per second.
   * Since the timestamp has microsecond resolution,
   * the returned value is always 1000000.
   */
  static TimeDiff Resolution();

#if defined(_WIN32)
  static Timestamp FromFileTimeNP(uint32 file_time_lo, uint32 file_time_hi);
  void ToFileTimeNP(uint32& file_time_lo, uint32& file_time_hi) const;
#endif

  FUN_ALWAYS_INLINE friend void Swap(Timestamp& x, Timestamp& y) {
    x.Swap(y);
  }

  FUN_ALWAYS_INLINE friend uint32 HashOf(const Timestamp& v) {
    return HashOf(v.value_);
  }

 private:
  TimeVal value_;
};


//
// inlines
//

FUN_ALWAYS_INLINE Timestamp::Timestamp() : value_(0) {}

FUN_ALWAYS_INLINE Timestamp::Timestamp(ForceInit_TAG) {
  Update();
}

FUN_ALWAYS_INLINE Timestamp Timestamp::Now() {
  return Timestamp(ForceInit);
}

FUN_ALWAYS_INLINE Timestamp::Timestamp(TimeVal tv) : value_(tv) {}

FUN_ALWAYS_INLINE Timestamp::Timestamp(const Timestamp& rhs)
  : value_(rhs.value_) {}

FUN_ALWAYS_INLINE Timestamp::~Timestamp() {}

FUN_ALWAYS_INLINE Timestamp& Timestamp::operator = (const Timestamp& rhs) {
  value_ = rhs.value_;
  return *this;
}

FUN_ALWAYS_INLINE Timestamp& Timestamp::operator = (TimeVal tv) {
  value_ = tv;
  return *this;
}

FUN_ALWAYS_INLINE void Timestamp::Swap(Timestamp& other) {
  fun::Swap(value_, other.value_);
}

FUN_ALWAYS_INLINE bool Timestamp::operator == (const Timestamp& ts) const {
  return value_ == ts.value_;
}

FUN_ALWAYS_INLINE bool Timestamp::operator != (const Timestamp& ts) const {
  return value_ != ts.value_;
}

FUN_ALWAYS_INLINE bool Timestamp::operator >  (const Timestamp& ts) const {
  return value_ > ts.value_;
}
FUN_ALWAYS_INLINE bool Timestamp::operator >= (const Timestamp& ts) const {
  return value_ >= ts.value_;
}

FUN_ALWAYS_INLINE bool Timestamp::operator <  (const Timestamp& ts) const {
  return value_ < ts.value_;
}

FUN_ALWAYS_INLINE bool Timestamp::operator <= (const Timestamp& ts) const {
  return value_ <= ts.value_;
}

FUN_ALWAYS_INLINE Timestamp Timestamp::operator + (Timestamp::TimeDiff d) const {
  return Timestamp(value_ + d);
}

FUN_ALWAYS_INLINE Timestamp Timestamp::operator - (Timestamp::TimeDiff d) const {
  return Timestamp(value_ - d);
}

FUN_ALWAYS_INLINE Timestamp::TimeDiff Timestamp::operator - (const Timestamp& ts) const {
  return value_ - ts.value_;
}

FUN_ALWAYS_INLINE Timestamp& Timestamp::operator += (Timestamp::TimeDiff d) {
  value_ += d;
  return *this;
}

FUN_ALWAYS_INLINE Timestamp& Timestamp::operator -= (Timestamp::TimeDiff d) {
  value_ -= d;
  return *this;
}

FUN_ALWAYS_INLINE std::time_t Timestamp::EpochTime() const {
  return std::time_t(value_ / Resolution());
}

FUN_ALWAYS_INLINE Timestamp::UtcTimeVal Timestamp::UtcTime() const {
  return value_*10 + (TimeDiff(0x01b21dd2) << 32) + 0x13814000;
}

FUN_ALWAYS_INLINE Timestamp::TimeVal Timestamp::EpochMicroseconds() const {
  return value_;
}

FUN_ALWAYS_INLINE Timestamp::TimeDiff Timestamp::Elapsed() const {
  Timestamp now(ForceInit);
  return now - *this;
}

FUN_ALWAYS_INLINE bool Timestamp::IsElapsed(Timestamp::TimeDiff interval) const {
  Timestamp now(ForceInit);
  TimeDiff span = now - *this;
  return span >= interval;
}

FUN_ALWAYS_INLINE Timestamp::TimeDiff Timestamp::Resolution() {
  return 1000000; // 10^6
}

FUN_ALWAYS_INLINE Timestamp::TimeVal Timestamp::Raw() const {
  return value_;
}

} // namespace fun
