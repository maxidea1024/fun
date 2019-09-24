#pragma once

#include "fun/base/base.h"
#include "fun/base/timespan.h"
#include "fun/base/timestamp.h"

namespace fun {

/**
 * ExpirationDecorator adds an expiration method to values so that they can be
 * used with the UniqueExpireCache.
 */
template <typename ArgsType>
class ExpirationDecorator {
 public:
  ExpirationDecorator() : value_(), expires_at_() {}

  /**
   * Creates an element that will expire in diff milliseconds
   */
  ExpirationDecorator(const ArgsType& p, const Timespan::TimeDiff& msecs)
      : value_(p), expires_at_() {
    expires_at_ += (msecs * 1000);
  }

  /**
   * Creates an element that will expire after the given timeSpan
   */
  ExpirationDecorator(const ArgsType& p, const Timespan& timespan)
      : value_(p), expires_at_() {
    expires_at_ += timespan.TotalMicroseconds();
  }

  /**
   * Creates an element that will expire at the given time point
   */
  ExpirationDecorator(const ArgsType& p, const Timestamp& timestamp)
      : value_(p), expires_at_(timestamp) {}

  ~ExpirationDecorator() {}

  const Timestamp& GetExpiration() const { return expires_at_; }

  const ArgsType& Value() const { return value_; }

  ArgsType& Value() { return value_; }

 private:
  ArgsType value_;
  Timestamp expires_at_;
};

}  // namespace fun
