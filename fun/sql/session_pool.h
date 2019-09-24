#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/pooled_session_holder.h"
#include "fun/sql/pooled_session_impl.h"
#include "fun/sql/session.h"
#include "fun/hash_map.h"
#include "fun/base/any.h"
#include "fun/timer.h"
#include "fun/base/mutex.h"
#include <map>

namespace fun {
namespace sql {

/**
 * This class implements session pooling for FUN Data.
 *
 * Creating a connection to a database is often a time consuming
 * operation. Therefore it makes sense to reuse a session object
 * once it is no longer needed.
 *
 * A SessionPool manages a collection of SessionImpl objects
 * (decorated with a PooledSessionImpl).
 *
 * When a SessionImpl object is requested, the SessionPool first
 * looks in its set of already initialized SessionImpl for an
 * available object. If one is found, it is returned to the
 * client and marked as "in-use". If no SessionImpl is available,
 * the SessionPool attempts to create a new one for the client.
 * To avoid excessive creation of SessionImpl objects, a limit
 * can be set on the maximum number of objects.
 * Sessions found not to be connected to the database are purged
 * from the pool whenever one of the following events occurs:
 *
 *   - JanitorTimer event
 *   - Get() request
 *   - PutBack() request
 *
 * Not connected idle sessions can not exist.
 *
 * Usage example:
 *
 *     SessionPool pool("ODBC", "...");
 *     ...
 *     Session sess(pool.Get());
 *     ...
 */
class FUN_SQL_API SessionPool : public RefCountedObject {
 public:
  typedef fun::RefCountedPtr<SessionPool> Ptr;

  /**
   * Creates the SessionPool for sessions with the given connector
   * and connection_string.
   *
   * The pool allows for at most max_sessions sessions to be created.
   * If a session has been idle for more than idle_time seconds, and more than
   * min_sessions sessions are in the pool, the session is automatically destroyed.
   */
  SessionPool(const String& connector,
              const String& connection_string,
              int32 min_sessions = 1,
              int32 max_sessions = 32,
              int32 idle_time = 60);

  /**
   * Destroys the SessionPool.
   */
  ~SessionPool();

  /**
   * Returns a Session.
   *
   * If there are unused sessions available, one of the
   * unused sessions is recycled. Otherwise, a new session
   * is created.
   *
   * If the maximum number of sessions for this pool has
   * already been created, a SessionPoolExhaustedException
   * is thrown.
   */
  Session Get();

  /**
   * Returns a Session with requested property set.
   * The property can be different from the default pool
   * value, in which case it is reset back to the pool
   * value when the session is reclaimed by the pool.
   */
  template <typename T>
  Session Get(const String& name, const T& value) {
    Session s = Get();
    add_property_map_.insert(AddPropertyMap::value_type(s.GetImpl(),
      std::make_pair(name, s.GetProperty(name))));
    s.SetProperty(name, value);

    return s;
  }

  /**
   * Returns a Session with requested feature set.
   * The feature can be different from the default pool
   * value, in which case it is reset back to the pool
   * value when the session is reclaimed by the pool.
   */
  Session Get(const String& name, bool value);

  /**
   * Returns the maximum number of sessions the SessionPool will manage.
   */
  int32 Capacity() const;

  /**
   * Returns the number of sessions currently in use.
   */
  int32 UsedCount() const;

  /**
   * Returns the number of idle sessions.
   */
  int32 IdleCount() const;

  /**
   * Returns the number of not connected active sessions.
   */
  int32 DeadCount();

  /**
   * Returns the number of allocated sessions.
   */
  int32 AllocatedCount() const;

  /**
   * Returns the number of available (idle + remaining capacity) sessions.
   */
  int32 AvailableCount() const;

  /**
   * Returns the name for this pool.
   */
  String GetName() const;

  /**
   * Returns the name formatted from supplied arguments as "connector:///connection_string".
   */
  static String GetName(const String& connector, const String& connection_string);

  /**
   * Sets feature for all the sessions.
   */
  void SetFeature(const String& name, bool state);

  /**
   * Returns the requested feature.
   */
  bool GetFeature(const String& name);

  /**
   * Sets property for all sessions.
   */
  void SetProperty(const String& name, const fun::Any& value);

  /**
   * Returns the requested property.
   */
  fun::Any GetProperty(const String& name);

  /**
   * Shuts down the session pool.
   */
  void Shutdown();

  /**
   * Returns true if session pool is active (not shut down).
   */
  bool IsActive() const;

 protected:
  /**
   * Can be overridden by subclass to perform custom initialization
   * of a newly created database session.
   *
   * The default implementation does nothing.
   */
  virtual void CustomizeSession(Session& session);

  typedef fun::RefCountedPtr<PooledSessionHolder> PooledSessionHolderPtr;
  typedef fun::RefCountedPtr<PooledSessionImpl> PooledSessionImplPtr;
  typedef std::map<PooledSessionHolder*, PooledSessionHolderPtr> SessionList;
  typedef std::map<String, bool> FeatureMap;
  typedef std::map<String, fun::Any> PropertyMap;

  void PurgeDeadSessions();
  int32 DeadImpl(SessionList& sessions);
  void ApplySettings(SessionImpl::Ptr impl);
  void PutBack(PooledSessionHolderPtr holder);
  void OnJanitorTimer(fun::Timer&);

 private:
  typedef std::pair<String, fun::Any> PropertyPair;
  typedef std::pair<String, bool> FeaturePair;
  typedef std::map<SessionImpl::Ptr, PropertyPair> AddPropertyMap;
  typedef std::map<SessionImpl::Ptr, FeaturePair> AddFeatureMap;

  void CloseAll(SessionList& session_list);

  String connector_;
  String connection_string_;
  int32 min_sessions_;
  int32 max_sessions_;
  int32 idle_time_;
  int32 session_count_;
  SessionList idle_sessions_;
  SessionList active_sessions_;
  fun::Timer janitor_timer_;
  FeatureMap feature_map_;
  PropertyMap property_map_;
  std::atomic<bool> shutdown_;
  AddPropertyMap add_property_map_;
  AddFeatureMap add_feature_map_;
  mutable fun::Mutex mutex_;

  friend class PooledSessionImpl;

 public:
  SessionPool(const SessionPool&) = delete;
  SessionPool& operator=(const SessionPool&) = delete;
};


//
// inlines
//

inline String SessionPool::GetName( const String& connector,
                                    const String& connection_string) {
  return Session::GetUri(connector, connection_string);
}

inline String SessionPool::GetName() const {
  return GetName(connector_, connection_string_);
}

inline bool SessionPool::IsActive() const {
  return !shutdown_;
}

} // namespace sql
} // namespace fun
