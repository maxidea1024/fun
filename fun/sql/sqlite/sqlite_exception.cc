#include "fun/sql/SQLite/SQLiteException.h"
#include <typeinfo>

namespace fun {
namespace sql {
namespace sqlite {

FUN_IMPLEMENT_EXCEPTION(SQLiteException, fun::sql::SqlException, "SQLite exception")
FUN_IMPLEMENT_EXCEPTION(InvalidSQLStatementException, SQLiteException, "Invalid sql statement")
FUN_IMPLEMENT_EXCEPTION(InternalDBErrorException, SQLiteException, "Internal DB error")
FUN_IMPLEMENT_EXCEPTION(DBAccessDeniedException, SQLiteException, "DB access denied")
FUN_IMPLEMENT_EXCEPTION(ExecutionAbortedException, SQLiteException, "Execution aborted")
FUN_IMPLEMENT_EXCEPTION(DBLockedException, SQLiteException, "DB locked")
FUN_IMPLEMENT_EXCEPTION(TableLockedException, SQLiteException, "Table locked")
FUN_IMPLEMENT_EXCEPTION(NoMemoryException, SQLiteException, "Out of Memory")
FUN_IMPLEMENT_EXCEPTION(ReadOnlyException, SQLiteException, "Read only")
FUN_IMPLEMENT_EXCEPTION(InterruptException, SQLiteException, "Interrupt")
FUN_IMPLEMENT_EXCEPTION(IOErrorException, SQLiteException, "I/O error")
FUN_IMPLEMENT_EXCEPTION(CorruptImageException, SQLiteException, "Corrupt image")
FUN_IMPLEMENT_EXCEPTION(TableNotFoundException, SQLiteException, "Table not found")
FUN_IMPLEMENT_EXCEPTION(DatabaseFullException, SQLiteException, "Database full")
FUN_IMPLEMENT_EXCEPTION(CantOpenDBFileException, SQLiteException, "Can't open DB file")
FUN_IMPLEMENT_EXCEPTION(LockProtocolException, SQLiteException, "Lock protocol")
FUN_IMPLEMENT_EXCEPTION(SchemaDiffersException, SQLiteException, "Schema differs")
FUN_IMPLEMENT_EXCEPTION(RowTooBigException, SQLiteException, "Row too big")
FUN_IMPLEMENT_EXCEPTION(ConstraintViolationException, SQLiteException, "Constraint violation")
FUN_IMPLEMENT_EXCEPTION(DataTypeMismatchException, SQLiteException, "Data type mismatch")
FUN_IMPLEMENT_EXCEPTION(ParameterCountMismatchException, SQLiteException, "Parameter count mismatch")
FUN_IMPLEMENT_EXCEPTION(InvalidLibraryUseException, SQLiteException, "Invalid library use")
FUN_IMPLEMENT_EXCEPTION(OSFeaturesMissingException, SQLiteException, "OS features missing")
FUN_IMPLEMENT_EXCEPTION(AuthorizationDeniedException, SQLiteException, "Authorization denied")
FUN_IMPLEMENT_EXCEPTION(TransactionException, SQLiteException, "Transaction exception")

} // namespace sqlite
} // namespace sql
} // namespace fun
