#pragma once

#include "fun/sql/sql_exception.h"
#include "fun/sql/sqlite/sqlite.h"

namespace fun {
namespace sql {
namespace sqlite {

FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, SQLiteException, fun::sql::SqlException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, InvalidSQLStatementException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, InternalDBErrorException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, DBAccessDeniedException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, ExecutionAbortedException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, DBLockedException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, TableLockedException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, NoMemoryException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, ReadOnlyException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, InterruptException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, IOErrorException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, CorruptImageException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, TableNotFoundException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, DatabaseFullException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, CantOpenDBFileException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, LockProtocolException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, SchemaDiffersException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, RowTooBigException, SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, ConstraintViolationException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, DataTypeMismatchException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, ParameterCountMismatchException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, InvalidLibraryUseException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, OSFeaturesMissingException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, AuthorizationDeniedException,
                      SQLiteException)
FUN_DECLARE_EXCEPTION(FUN_SQLITE_API, TransactionException, SQLiteException)

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
