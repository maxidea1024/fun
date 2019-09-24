#pragma once

#include "fun/sql/limit.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * Range stores information how many rows a query should return.
 */
class FUN_SQL_API Range {
 public:
  /**
   * Creates the Range. lowValue must be smaller equal than upValue
   */
  Range(Limit::SizeT lowValue, Limit::SizeT upValue, bool hardLimit);

  /**
   * Destroys the Limit.
   */
  ~Range();

  /**
   * Returns the lower limit
   */
  const Limit& lower() const;

  /**
   * Returns the upper limit
   */
  const Limit& upper() const;

 private:
  Limit lower_;
  Limit upper_;
};

//
// inlines
//

inline const Limit& Range::lower() const { return lower_; }

inline const Limit& Range::upper() const { return upper_; }

namespace Keywords {

/**
 * Creates an upperLimit
 */
template <typename T>
Limit limit(T lim, bool hard = false) {
  return Limit(static_cast<Limit::SizeT>(lim), hard, false);
}

template <typename T>
Limit upperLimit(T lim, bool hard = false) {
  return limit(lim, hard);
}

template <typename T>
Limit lowerLimit(T lim) {
  return Limit(static_cast<Limit::SizeT>(lim), true, true);
}

template <typename T>
Range range(T low, T upp, bool hard = false) {
  return Range(static_cast<Limit::SizeT>(low), static_cast<Limit::SizeT>(upp),
               hard);
}

}  // namespace Keywords

}  // namespace sql
}  // namespace fun
