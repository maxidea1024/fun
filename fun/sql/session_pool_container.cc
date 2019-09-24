//TODO SessionPoolContainer -> SessionPoolSet 으로 변경해주는게 좋을듯..?

#include "fun/sql/session_pool_container.h"
#include "fun/sql/session_factory.h"
#include "fun/sql/sql_exception.h"
#include "fun/uri.h"
#include "fun/base/string.h"
#include "fun/base/exception.h"
#include <algorithm>

using fun::FastMutex;

namespace fun {
namespace sql {

SessionPoolContainer::SessionPoolContainer() {}

SessionPoolContainer::~SessionPoolContainer() {}

void SessionPoolContainer::add(SessionPool::Ptr pool) {
  fun_check_ptr(pool.Get());

  FastMutex::ScopedLock lock(mutex_);
  if (session_pools_.find(pool->name()) != session_pools_.end()) {
    throw SessionPoolExistsException("Session pool already exists: " + pool->name());
  }

  session_pools_.insert(SessionPoolMap::value_type(pool->name(), pool));
}

Session SessionPoolContainer::Add(const String& session_key,
                                  const String& connection_string,
                                  int32 min_sessions,
                                  int32 max_sessions,
                                  int32 idle_time) {
  String name = SessionPool::GetName(session_key, connection_string);

  FastMutex::ScopedLock lock(mutex_);
  SessionPoolMap::iterator it = session_pools_.find(name);

  // pool already exists, silently return a session from it
  if (it != session_pools_.end()) {
    return it->second->Get();
  }

  SessionPool::Ptr session =
    new SessionPool(session_key, connection_string, min_sessions, max_sessions, idle_time);

  std::pair<SessionPoolMap::iterator, bool> ins =
    session_pools_.insert(SessionPoolMap::value_type(name, session));

  return ins.first->second->Get();
}

bool SessionPoolContainer::IsActive(const String& session_key,
                                    const String& connection_string) const {
  String name = connection_string.IsEmpty() ?
    session_key : SessionPool::GetName(session_key, connection_string);

  SessionPoolMap::const_iterator it = session_pools_.find(name);
  if (it != session_pools_.end() && it->second->IsActive()) {
    return true;
  }

  return false;
}

Session SessionPoolContainer::Get(const String& name) {
  return GetPool(name).Get();
}

SessionPool& SessionPoolContainer::GetPool(const String& name) {
  URI uri(name);
  String path = uri.GetPath();
  fun_check(!path.IsEmpty());
  String n = Session::GetUri(uri.GetScheme(), path.substr(1));

  FastMutex::ScopedLock lock(mutex_);
  SessionPoolMap::iterator it = session_pools_.find(n);
  if (session_pools_.end() == it) {
    throw NotFoundException(n);
  }
  return *it->second;
}

void SessionPoolContainer::Shutdown() {
  SessionPoolMap::iterator it = session_pools_.begin();
  SessionPoolMap::iterator end = session_pools_.end();
  for (; it != end; ++it) {
    it->second->Shutdown();
  }
}

} // namespace sql
} // namespace fun
