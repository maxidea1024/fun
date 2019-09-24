#include "fun/sql/postgresql/connector.h"
#include "fun/sql/postgresql/sessionimpl.h"
#include "fun/sql/session_factory.h"

const PostgreSqlConnectorRegistrator funPostgreSqlConnectorRegistrator;

namespace fun {
namespace sql {
namespace postgresql {

String Connector::KEY = FUN_DATA_POSTGRESQL_CONNECTOR_NAME;

Connector::Connector() {}

Connector::~Connector() {}

const String& Connector::GetName() const {
  static const String n(FUN_DATA_POSTGRESQL_CONNECTOR_NAME);
  return n;
}

SessionImpl::Ptr Connector::CreateSession(const String& connection_string,
                                          size_t timeout) {
  return fun::RefCountedPtr<fun::sql::SessionImpl>(
      new SessionImpl(connection_string, timeout));
}

void Connector::RegisterConnector() {
  fun::sql::SessionFactory::Instance().Add(new Connector());
}

void Connector::UnregisterConnector() {
  fun::sql::SessionFactory::Instance().Remove(
      FUN_DATA_POSTGRESQL_CONNECTOR_NAME);
}

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
