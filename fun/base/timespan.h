#pragma once

#include "fun/base/base.h"
#include "fun/base/date_time_types.h"

namespace fun {

/**
 * A class that represents time spans up to microsecond resolution.
 */
class FUN_BASE_API Timespan {
 public:
  static const int64 MaxSeconds;
  static const int64 MinSeconds;
  static const int64 MaxMilliSeconds;
  static const int64 MinMilliSeconds;

  static const Timespan MinValue;
  static const Timespan MaxValue;
  static const Timespan Zero;

  static const Timespan Infinite;

  // Some constants
  static const int64 MILLISECONDS;
  static const int64 SECONDS;
  static const int64 MINUTES;
  static const int64 HOURS;
  static const int64 DAYS;

  /**
   * Creates a zero Timespan.
   */
  Timespan();

  /**
   * Creates a Timespan.
   */
  Timespan(int64 ticks);

  /**
   * Creates a Timespan.
   */
  Timespan(int32 hours, int32 minutes, int32 seconds);

  /**
   * Creates a Timespan.
   */
  Timespan(int32 days, int32 hours, int32 minutes, int32 seconds);

  /**
   * Creates a Timespan.
   */
  Timespan(int32 days, int32 hours, int32 minutes, int32 seconds,
           int32 milliseconds);

  /**
   * Creates a Timespan.
   */
  Timespan(int32 days, int32 hours, int32 minutes, int32 seconds,
           int32 milliseconds, int32 microsecond);

  bool IsValid() const;

  Timespan operator+(const Timespan& other) const;
  Timespan& operator+=(const Timespan& other);
  Timespan operator-() const;
  Timespan operator-(const Timespan& other) const;
  Timespan& operator-=(const Timespan& other);
  Timespan operator*(float scalar) const;
  Timespan& operator*=(float scalar);

  int32 Compare(const Timespan& other) const;
  friend bool operator==(const Timespan& lhs, const Timespan& rhs);
  friend bool operator!=(const Timespan& lhs, const Timespan& rhs);
  friend bool operator>(const Timespan& lhs, const Timespan& rhs);
  friend bool operator>=(const Timespan& lhs, const Timespan& rhs);
  friend bool operator<(const Timespan& lhs, const Timespan& rhs);
  friend bool operator<=(const Timespan& lhs, const Timespan& rhs);

  Timespan Duration() const;

  /**
   * Returns the number of days.
   */
  int32 Days() const;

  /**
   * Returns the number of hours (0 to 23).
   */
  int32 Hours() const;

  /**
   * Returns the number of minutes (0 to 59).
   */
  int32 Minutes() const;

  /**
   * Returns the number of seconds (0 to 59).
   */
  int32 Seconds() const;

  /**
   * Returns the number of milliseconds (0 to 999).
   */
  int32 Milliseconds() const;

  /**
   * Returns the fractions of a millisecond
   * in microseconds (0 to 999).
   */
  int32 Microseconds() const;

  /**
   * Returns the total number of hours.
   */
  double TotalHours() const;

  /**
   * Returns the total number of minutes.
   */
  double TotalMinutes() const;

  /**
   * Returns the total number of seconds.
   */
  double TotalSeconds() const;

  /**
   * Returns the total number of milliseconds.
   */
  double TotalMilliseconds() const;

  /**
   * Returns the total number of microseconds.
   */
  int64 TotalMicroseconds() const;

  /**
   */
  int64 ToTicks() const;

  /**
   */
  static Timespan FromTicks(int64 ticks);

  static Timespan FromDays(double days);
  static Timespan FromHours(double hours);
  static Timespan FromMinutes(double minutes);
  static Timespan FromSeconds(double seconds);
  static Timespan FromMilliseconds(double Milliseconds);
  static Timespan FromMicroseconds(double microseconds);

  String ToString() const;

  static bool TryParse(const String& str, Timespan& out_timespan);
  static Timespan Parse(const String& str);

  FUN_BASE_API friend uint32 HashOf(const Timespan& timespan);
  FUN_BASE_API friend Archive& operator&(Archive& ar, Timespan& timespan);

  bool TryAssign(int32 days, int32 hours, int32 minutes, int32 seconds,
                 int32 milliseconds, int32 microsecond);
  void Assign(int32 days, int32 hours, int32 minutes, int32 seconds,
              int32 milliseconds, int32 microsecond);

 private:
  int64 ticks_;
};

//
// inlines
//

FUN_ALWAYS_INLINE Timespan::Timespan() : ticks_(0) {}

FUN_ALWAYS_INLINE Timespan::Timespan(int32 hours, int32 minutes,
                                     int32 seconds) {
  Assign(0, hours, minutes, seconds, 0, 0);
}

FUN_ALWAYS_INLINE Timespan::Timespan(int32 days, int32 hours, int32 minutes,
                                     int32 seconds) {
  Assign(days, hours, minutes, seconds, 0, 0);
}

FUN_ALWAYS_INLINE Timespan::Timespan(int32 days, int32 hours, int32 minutes,
                                     int32 seconds, int32 milliseconds) {
  Assign(days, hours, minutes, seconds, milliseconds, 0);
}

FUN_ALWAYS_INLINE Timespan::Timespan(int32 days, int32 hours, int32 minutes,
                                     int32 seconds, int32 milliseconds,
                                     int32 microsecond) {
  Assign(days, hours, minutes, seconds, milliseconds, microsecond);
}

FUN_ALWAYS_INLINE bool Timespan::IsValid() const {
  return ticks_ >= MinValue.ticks_ && ticks_ <= MaxValue.ticks_;
}

FUN_ALWAYS_INLINE Timespan Timespan::operator+(const Timespan& other) const {
  return Timespan(ticks_ + other.ticks_);
}

FUN_ALWAYS_INLINE Timespan& Timespan::operator+=(const Timespan& other) {
  ticks_ += other.ticks_;
  return *this;
}

FUN_ALWAYS_INLINE Timespan Timespan::operator-() const {
  return Timespan(-ticks_);
}

FUN_ALWAYS_INLINE Timespan Timespan::operator-(const Timespan& other) const {
  return Timespan(ticks_ - other.ticks_);
}

FUN_ALWAYS_INLINE Timespan& Timespan::operator-=(const Timespan& other) {
  ticks_ -= other.ticks_;
  return *this;
}

FUN_ALWAYS_INLINE Timespan Timespan::operator*(float scalar) const {
  return Timespan((int64)(ticks_ * scalar));
}

FUN_ALWAYS_INLINE Timespan& Timespan::operator*=(float scalar) {
  ticks_ = int64(ticks_ * scalar);
  return *this;
}

// premultiply
FUN_ALWAYS_INLINE Timespan operator*(float scalar, const Timespan& timespan) {
  return timespan.operator*(scalar);
}

FUN_ALWAYS_INLINE int32 Timespan::Compare(const Timespan& other) const {
  return ticks_ == other.ticks_ ? 0 : (ticks_ < other.ticks_ ? -1 : +1);
}

FUN_ALWAYS_INLINE bool operator==(const Timespan& lhs, const Timespan& rhs) {
  return lhs.Compare(rhs) == 0;
}

FUN_ALWAYS_INLINE bool operator!=(const Timespan& lhs, const Timespan& rhs) {
  return lhs.Compare(rhs) != 0;
}

FUN_ALWAYS_INLINE bool operator>(const Timespan& lhs, const Timespan& rhs) {
  return lhs.Compare(rhs) > 0;
}

FUN_ALWAYS_INLINE bool operator>=(const Timespan& lhs, const Timespan& rhs) {
  return lhs.Compare(rhs) >= 0;
}

FUN_ALWAYS_INLINE bool operator<(const Timespan& lhs, const Timespan& rhs) {
  return lhs.Compare(rhs) < 0;
}

FUN_ALWAYS_INLINE bool operator<=(const Timespan& lhs, const Timespan& rhs) {
  return lhs.Compare(rhs) <= 0;
}

FUN_ALWAYS_INLINE Timespan Timespan::Duration() const {
  return Timespan(ticks_ >= 0 ? ticks_ : -ticks_);
}

FUN_ALWAYS_INLINE int32 Timespan::Days() const {
  return int32(ticks_ / DateTimeConstants::TICKS_PER_DAY);
}

FUN_ALWAYS_INLINE int32 Timespan::Hours() const {
  return int32((ticks_ / DateTimeConstants::TICKS_PER_HOUR) % 24);
}

FUN_ALWAYS_INLINE int32 Timespan::Minutes() const {
  return int32((ticks_ / DateTimeConstants::TICKS_PER_MINUTE) % 60);
}

FUN_ALWAYS_INLINE int32 Timespan::Seconds() const {
  return int32((ticks_ / DateTimeConstants::TICKS_PER_SECOND) % 60);
}

FUN_ALWAYS_INLINE int32 Timespan::Milliseconds() const {
  return int32((ticks_ / DateTimeConstants::TICKS_PER_MILLISECOND) % 1000);
}

FUN_ALWAYS_INLINE int32 Timespan::Microseconds() const {
  return int32(((ticks_ % DateTimeConstants::TICKS_PER_MICROSECOND) / 1000) %
               1000);
}

FUN_ALWAYS_INLINE double Timespan::TotalHours() const {
  return (double)ticks_ / DateTimeConstants::TICKS_PER_HOUR;
}

FUN_ALWAYS_INLINE double Timespan::TotalMinutes() const {
  return (double)ticks_ / DateTimeConstants::TICKS_PER_MINUTE;
}

FUN_ALWAYS_INLINE double Timespan::TotalSeconds() const {
  return (double)ticks_ / DateTimeConstants::TICKS_PER_SECOND;
}

FUN_ALWAYS_INLINE double Timespan::TotalMilliseconds() const {
  return (double)ticks_ / DateTimeConstants::TICKS_PER_MILLISECOND;
}

FUN_ALWAYS_INLINE int64 Timespan::TotalMicroseconds() const {
  return ticks_ / DateTimeConstants::TICKS_PER_MICROSECOND;
}

FUN_ALWAYS_INLINE int64 Timespan::ToTicks() const { return ticks_; }

FUN_ALWAYS_INLINE Timespan Timespan::FromTicks(int64 ticks) {
  return Timespan(ticks);
}

// UDL은 제거하도록 하자. 썩 맘에 들지 않는다..
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4455)
#endif

FUN_ALWAYS_INLINE Timespan operator"" h(unsigned long long value) {
  return Timespan::FromHours((double)value);
}
FUN_ALWAYS_INLINE Timespan operator"" m(unsigned long long value) {
  return Timespan::FromMinutes((double)value);
}
FUN_ALWAYS_INLINE Timespan operator"" s(unsigned long long value) {
  return Timespan::FromSeconds((double)value);
}
FUN_ALWAYS_INLINE Timespan operator"" ms(unsigned long long value) {
  return Timespan::FromMilliseconds((double)value);
}
FUN_ALWAYS_INLINE Timespan operator"" us(unsigned long long value) {
  return Timespan::FromMicroseconds((double)value);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace fun
