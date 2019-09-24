#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/session_impl.h"
#include "fun/ref_counted_ptr.h"
#include "fun/timestamp.h"
#include "fun/base/mutex.h"

namespace fun {
namespace sql {

class SessionPool;

/**
 * This class is used by SessionPool to manage SessionImpl objects.
 */
class FUN_SQL_API PooledSessionHolder : public fun::RefCountedObject {
 public:
  typedef RefCountedPtr<PooledSessionHolder> Ptr;

  /**
   * Creates the PooledSessionHolder.
   */
  PooledSessionHolder(SessionPool& owner, SessionImpl::Ptr session_impl);

  /**
   * Destroys the PooledSessionHolder.
   */
  ~PooledSessionHolder();

  /**
   * Returns a pointer to the SessionImpl.
   */
  SessionImpl::Ptr GetSession();

  /**
   * Returns a reference to the SessionHolder's owner.
   */
  SessionPool& GetOwner();

  /**
   * Updates the last access timestamp.
   */
  void Access();

  /**
   * Returns the number of seconds the session has not been used.
   */
  int32 Idle() const;

 private:
  SessionPool& onwer_;
  SessionImpl::Ptr impl_;
  fun::Timestamp last_used_;
  mutable fun::FastMutex mutex_;
};


//
// inlines
//

inline SessionImpl::Ptr PooledSessionHolder::GetSession() {
  return impl_;
}

inline SessionPool& PooledSessionHolder::GetOwner() {
  return onwer_;
}

inline void PooledSessionHolder::Access() {
  fun::FastMutex::ScopedLock l(mutex_);

  last_used_.Update();
}

inline int32 PooledSessionHolder::Idle() const {
  fun::FastMutex::ScopedLock l(mutex_);

  return (int32)(last_used_.Elapsed() / fun::Timestamp::Resolution());
}

} // namespace sql
} // namespace fun
