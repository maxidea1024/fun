#pragma once

#include "fun/base/mutex.h"
#include "fun/base/string.h"
#include "fun/sql/session.h"
#include "fun/sql/session_pool.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * This class implements container of session pools.
 */
class FUN_SQL_API SessionPoolContainer {
 public:
  /**
   * Creates the SessionPoolContainer for sessions with the given session
   * parameters.
   */
  SessionPoolContainer();

  /**
   * Destroys the SessionPoolContainer.
   */
  ~SessionPoolContainer();

  /**
   * Adds existing session pool to the container.
   * Throws SessionPoolExistsException if pool already exists.
   */
  void Add(SessionPool::Ptr pool);

  /**
   * Adds a new session pool to the container and returns a Session from
   * newly created pool. If pool already exists, request to add is silently
   * ignored and session is returned from the existing pool.
   */
  Session Add(const String& session_key, const String& connection_string,
              int32 min_sessions = 1, int32 max_sessions = 32,
              int32 idle_time = 60);

  /**
   * Returns true if the requested name exists, false otherwise.
   */
  bool Has(const String& name) const;

  /**
   * Returns true if the session is active (i.e. not shut down).
   * If connection_string is empty string, session_key must be a
   * fully qualified session name as registered with the pool
   * container.
   */
  bool IsActive(const String& session_key,
                const String& connection_string = "") const;

  /**
   * Returns the requested Session.
   * Throws NotFoundException if session is not found.
   */
  Session Get(const String& name);

  /**
   * Returns a SessionPool reference.
   * Throws NotFoundException if session is not found.
   */
  SessionPool& GetPool(const String& name);

  /**
   * Removes a SessionPool.
   */
  void Remove(const String& name);

  /**
   * Returns the number of session pols in the container.
   */
  int32 Count() const;

  /**
   * Shuts down all the held pools.
   */
  void Shutdown();

 private:
  typedef std::map<String, SessionPool::Ptr, fun::CILess> SessionPoolMap;

  SessionPoolMap session_pools_;
  fun::FastMutex mutex_;

 public:
  SessionPoolContainer(const SessionPoolContainer&) = delete;
  SessionPoolContainer& operator=(const SessionPoolContainer&) = delete;
};

//
// inlines
//

inline bool SessionPoolContainer::Has(const String& name) const {
  return session_pools_.find(name) != session_pools_.end();
}

inline void SessionPoolContainer::Remove(const String& name) {
  session_pools_.erase(name);
}

inline int32 SessionPoolContainer::Count() const {
  return static_cast<int>(session_pools_.size());
}

}  // namespace sql
}  // namespace fun
