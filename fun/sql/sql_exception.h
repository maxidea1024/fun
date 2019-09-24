#pragma once

#include "fun/sql/sql.h"
#include "fun/base/exception.h"

namespace fun {
namespace sql {

FUN_DECLARE_EXCEPTION(FUN_SQL_API, SqlException, fun::IoException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, RowDataMissingException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, UnknownDataBaseException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, UnknownTypeException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, ExecutionException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, BindingException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, ExtractException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, LimitException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, NotSupportedException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, SessionUnavailableException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, SessionPoolExhaustedException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, SessionPoolExistsException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, NoSqlException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, LengthExceededException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, ConnectionFailedException, SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQL_API, NotConnectedException, SqlException)

} // namespace sql
} // namespace fun
