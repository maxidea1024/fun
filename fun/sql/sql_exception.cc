#include "fun/sql/sql_exception.h"
#include <typeinfo>

namespace fun {
namespace sql {

FUN_IMPLEMENT_EXCEPTION(SqlException, fun::IoException, "Database Exception")
FUN_IMPLEMENT_EXCEPTION(RowDataMissingException, SqlException,
                        "Data for row missing")
FUN_IMPLEMENT_EXCEPTION(UnknownDataBaseException, SqlException,
                        "Type of data base unknown")
FUN_IMPLEMENT_EXCEPTION(UnknownTypeException, SqlException,
                        "Type of data unknown")
FUN_IMPLEMENT_EXCEPTION(ExecutionException, SqlException, "Execution error")
FUN_IMPLEMENT_EXCEPTION(BindingException, SqlException, "Binding error")
FUN_IMPLEMENT_EXCEPTION(ExtractException, SqlException, "Extraction error")
FUN_IMPLEMENT_EXCEPTION(LimitException, SqlException, "Limit error")
FUN_IMPLEMENT_EXCEPTION(NotSupportedException, SqlException,
                        "Feature or property not supported")
FUN_IMPLEMENT_EXCEPTION(SessionUnavailableException, SqlException,
                        "Session is unavailable")
FUN_IMPLEMENT_EXCEPTION(SessionPoolExhaustedException, SqlException,
                        "No more sessions available from the session pool")
FUN_IMPLEMENT_EXCEPTION(SessionPoolExistsException, SqlException,
                        "Session already exists in the pool")
FUN_IMPLEMENT_EXCEPTION(NoSqlException, SqlException, "No data found")
FUN_IMPLEMENT_EXCEPTION(LengthExceededException, SqlException, "Data too long")
FUN_IMPLEMENT_EXCEPTION(ConnectionFailedException, SqlException,
                        "Connection attempt failed")
FUN_IMPLEMENT_EXCEPTION(NotConnectedException, SqlException,
                        "Not connected to data source")

}  // namespace sql
}  // namespace fun
