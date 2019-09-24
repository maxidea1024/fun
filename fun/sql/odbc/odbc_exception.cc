#include "fun/sql/odbc/odbc_exception.h"
#include <typeinfo>

namespace fun {
namespace sql {
namespace odbc {

FUN_IMPLEMENT_EXCEPTION(OdbcException, fun::sql::SqlException, "Generic ODBC error")
FUN_IMPLEMENT_EXCEPTION(InsufficientStorageException, OdbcException, "Insufficient storage error")
FUN_IMPLEMENT_EXCEPTION(UnknownDataLengthException, OdbcException, "Unknown length of remaining data")
FUN_IMPLEMENT_EXCEPTION(DataTruncatedException, OdbcException, "Variable length character or binary data truncated")

} // namespace odbc
} // namespace sql
} // namespace fun
