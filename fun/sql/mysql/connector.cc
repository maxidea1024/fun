#include "fun/sql/mysql/connector.h"
#include <mysql.h>
#include "fun/base/exception.h"
#include "fun/sql/mysql/session_impl.h"
#include "fun/sql/session_factory.h"

const MySqlConnectorRegistrator funMySqlConnectorRegistrator;

namespace fun {
namespace sql {
namespace mysql {

String Connector::KEY(FUN_DATA_MYSQL_CONNECTOR_NAME);
fun::FastMutex Connector::mutex_;

Connector::Connector() {}

Connector::~Connector() {}

const String& Connector::GetName() const {
  static const String n(FUN_DATA_MYSQL_CONNECTOR_NAME);
  return n;
}

fun::RefCountedPtr<fun::sql::SessionImpl> Connector::CreateSession(
    const String& connection_string, size_t timeout) {
  static bool init_done = false;
  {
    fun::FastMutex::ScopedLock l(mutex_);
    if (!init_done) {
      if (mysql_library_init(0, 0, 0) != 0) {
        throw Exception("mysql_library_init error");
      }

      init_done = true;
    }
  }

  return fun::RefCountedPtr<fun::sql::SessionImpl>(
      new fun::sql::MySql::SessionImpl(connection_string, timeout));
}

void Connector::RegisterConnector() {
  fun::sql::SessionFactory::Instance().Add(new Connector());
}

void Connector::UnregisterConnector() {
  fun::sql::SessionFactory::Instance().Remove(FUN_DATA_MYSQL_CONNECTOR_NAME);
  mysql_library_end();
}

}  // namespace mysql
}  // namespace sql
}  // namespace fun
