#include "fun/sql/session_factory.h"
#include "fun/uri.h"
#include "fun/base/string.h"

namespace fun {
namespace sql {

SessionFactory::SessionFactory() {}

SessionFactory::~SessionFactory() {}

SessionFactory& SessionFactory::Instance() {
  static SessionFactory sf;
  return sf;
}

void SessionFactory::Add(Connector::Ptr connector) {
  fun::FastMutex::ScopedLock lock(mutex_);
  SessionInfo info(connector);
  std::pair<Connectors::iterator, bool> res =
    connectors_.insert(std::make_pair(connector->GetName(), info));
  if (!res.second) {
    res.first->second.cnt++;
  }
}

void SessionFactory::Remove(const String& key) {
  fun::FastMutex::ScopedLock lock(mutex_);
  Connectors::iterator it = connectors_.find(key);
  fun_check(connectors_.end() != it);

  --(it->second.cnt);
  if (it->second.cnt == 0) {
    connectors_.erase(it);
  }
}

Session SessionFactory::Create( const String& key,
                                const String& connection_string,
                                size_t timeout) {
  fun::SharedPtr<Connector> connector;
  {
    fun::FastMutex::ScopedLock lock(mutex_);
    Connectors::iterator it = connectors_.find(key);
    if (connectors_.end() == it) {
      throw fun::NotFoundException(key);
    }
    connector = it->second.connector;
  }
  return Session(connector->CreateSession(connection_string, timeout));
}

Session SessionFactory::Create(const String& uri, size_t timeout) {
  Uri u(uri);
  fun_check(!u.GetPath().IsEmpty());
  return Create(u.GetScheme(), u.GetPath().substr(1), timeout);
}

SessionFactory::SessionInfo::SessionInfo(Connector::Ptr connector)
  : cnt(1), connector(connector) {}

} // namespace sql
} // namespace fun
