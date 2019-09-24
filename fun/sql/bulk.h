#pragma once

#include "fun/Void.h"
#include "fun/sql/limit.h"

namespace fun {
namespace sql {

class FUN_SQL_API Bulk {
 public:
  /**
   * Creates the Bulk.
   */
  Bulk(const Limit& limit);

  /**
   * Creates the Bulk.
   */
  Bulk(uint32 value);

  /**
   * Destroys the bulk.
   */
  ~Bulk();

  /**
   * Returns the limit associated with this bulk object.
   */
  const Limit& limit() const;

  /**
   * Returns the value of the limit associated with
   * this bulk object.
   */
  uint32 size() const;

 private:
  Bulk();

  Limit limit_;
};


//
// inlines
//

inline const Limit& Bulk::limit() const {
  return limit_;
}

inline uint32 Bulk::size() const {
  return limit_.value();
}


namespace Keywords {

/**
 * Convenience function for creation of bulk.
 */
inline Bulk bulk(const Limit& limit = Limit(static_cast<Limit::SizeT>(Limit::LIMIT_UNLIMITED), false, false)) {
  return Bulk(limit);
}


/**
 * Dummy bulk function. Used for bulk binding creation
 * (see BulkBinding) and bulk extraction signalling to Statement.
 */
inline void bulk(Void) {}

} // namespace Keywords

typedef void (*BulkFnType)(Void);

} // namespace sql
} // namespace fun
