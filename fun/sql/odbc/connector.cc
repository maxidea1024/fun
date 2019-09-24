#include "fun/sql/odbc/connector.h"
#include "fun/sql/odbc/session_impl.h"
#include "fun/sql/session_factory.h"

const OdbcConnectorRegistrator funOdbcConnectorRegistrator;

namespace fun {
namespace sql {
namespace odbc {

const String Connector::KEY(FUN_DATA_ODBC_CONNECTOR_NAME);

Connector::Connector() {}

Connector::~Connector() {}

fun::RefCountedPtr<fun::sql::SessionImpl> Connector::CreateSession(
    const String& connection_string, size_t timeout) {
  return fun::RefCountedPtr<fun::sql::SessionImpl>(
      new SessionImpl(connection_string, timeout));
}

void Connector::RegisterConnector() {
  fun::sql::SessionFactory::Instance().Add(new Connector());
}

void Connector::UnregisterConnector() {
  fun::sql::SessionFactory::Instance().Remove(FUN_DATA_ODBC_CONNECTOR_NAME);
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
