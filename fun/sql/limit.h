#pragma once

#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * Limit stores information how many rows a query should return.
 */
class FUN_SQL_API Limit {
 public:
  typedef uint32 SizeT;

  enum Type {
    LIMIT_UNLIMITED = ~((SizeT)0)
  };

  /**
   * Creates the Limit.
   *
   * Value contains the upper row hint, if hard_limit is set to true, the limit acts as a hard
   * border, ie. every query must return exactly value rows, returning more than value objects will throw an exception!
   * LowerLimits always act as hard-limits!
   *
   * A value of LIMIT_UNLIMITED disables the limit.
   */
  Limit(SizeT value, bool hard_limit = false, bool is_lower_limit = false);

  /**
   * Destroys the Limit.
   */
  ~Limit();

  /**
   * Returns the value of the limit
   */
  SizeT Value() const;

  /**
   * Returns true if the limit is a hard limit.
   */
  bool IsHardLimit() const;

  /**
   * Returns true if the limit is a lower limit, otherwise it is an upperLimit
   */
  bool IsLowerLimit() const;

  /**
   * Equality operator.
   */
  bool operator == (const Limit& other) const;

  /**
   * Inequality operator.
   */
  bool operator != (const Limit& other) const;

 private:
  SizeT value_;
  bool hard_limit_;
  bool is_lower_limit_;
};


//
// inlines
//

inline uint32 Limit::Value() const {
  return value_;
}

inline bool Limit::IsHardLimit() const {
  return hard_limit_;
}

inline bool Limit::IsLowerLimit() const {
  return is_lower_limit_;
}

inline bool Limit::operator == (const Limit& other) const {
  return  other.value_ == value_ &&
          other.hard_limit_ == hard_limit_ &&
          other.is_lower_limit_ == is_lower_limit_;
}

inline bool Limit::operator != (const Limit& other) const {
  return  other.value_ != value_ ||
          other.hard_limit_ != hard_limit_ ||
          other.is_lower_limit_ != is_lower_limit_;
}

} // namespace sql
} // namespace fun
