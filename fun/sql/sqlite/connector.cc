#include "fun/sql/sqlite/connector.h"
#include "fun/sql/session_factory.h"
#include "fun/sql/sqlite/session_impl.h"

#if defined(FUN_UNBUNDLED)
#include <sqlite3.h>
#else
#include "sqlite3.h"
#endif

const SQLiteConnectorRegistrator funSQLiteConnectorRegistrator;

namespace fun {
namespace sql {
namespace sqlite {

const String Connector::KEY(FUN_DATA_SQLITE_CONNECTOR_NAME);

Connector::Connector() {}

Connector::~Connector() {}

fun::RefCountedPtr<fun::sql::SessionImpl> Connector::CreateSession(
    const String& connection_string, size_t timeout) {
  return fun::RefCountedPtr<fun::sql::SessionImpl>(
      new fun::sql::sqlite::SessionImpl(connection_string, timeout));
}

void Connector::RegisterConnector() {
  fun::sql::SessionFactory::Instance().Add(new Connector());
}

void Connector::UnregisterConnector() {
  fun::sql::SessionFactory::Instance().Remove(FUN_DATA_SQLITE_CONNECTOR_NAME);
}

void Connector::EnableSharedCache(bool flag) {
  sqlite3_enable_shared_cache(flag ? 1 : 0);
}

void Connector::EnableSoftHeapLimit(int limit) {
  sqlite3_soft_heap_limit(limit);
}

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
