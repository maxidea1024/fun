#include "fun/sql/postgresql/postgresql_exception.h"

namespace fun {
namespace sql {
namespace postgresql {

PostgreSqlException::PostgreSqlException(const String& message)
    : fun::sql::SqlException(String("[PostgreSQL]: ") + message) {}

PostgreSqlException::PostgreSqlException(const PostgreSqlException& e)
    : fun::sql::SqlException(e) {}

PostgreSqlException::~PostgreSqlException() throw() {}

//
// ConnectionException
//

ConnectionException::ConnectionException(const String& message)
    : PostgreSqlException(message) {}

//
// TransactionException
//

TransactionException::TransactionException(const String& message)
    : ConnectionException(message) {}

//
// StatementException
//

StatementException::StatementException(const String& message)
    : PostgreSqlException(message) {}

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
