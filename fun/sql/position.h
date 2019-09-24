#pragma once

#include "fun/sql/limit.h"

namespace fun {
namespace sql {

/**
 * Utility class wrapping unsigned integer. Used to
 * indicate the recordset position in batch sql statements.
 */
class FUN_SQL_API Position {
 public:
  typedef uint32 Type;

  /** Creates the Position. */
  Position(Type value);

  /** Destroys the Position. */
  ~Position();

  /** Returns the position value. */
  Type Value() const;

 private:
  Position();

  Type value_;
};

//
// inlines
//

inline Position::Type Position::Value() const { return value_; }

namespace Keywords {

/**
 * Convenience function for creation of position.
 */
template <typename T>
inline Position from(const T& value) {
  return Position(value);
}

}  // namespace Keywords

}  // namespace sql
}  // namespace fun
