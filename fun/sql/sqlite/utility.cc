#include "fun/sql/sqlite/utility.h"
#include "fun/sql/sqlite/sqlite_exception.h"
#include "fun/base/number_formatter.h"
#include "fun/base/string.h"
#include "fun/base/any.h"
#include "fun/base/exception.h"

#if defined(FUN_UNBUNDLED)
#include <sqlite3.h>
#else
#include "sqlite3.h"
#endif

#ifndef SQLITE_OPEN_URI
#define SQLITE_OPEN_URI  0
#endif

namespace fun {
namespace sql {
namespace sqlite {

const int Utility::THREAD_MODE_SINGLE = SQLITE_CONFIG_SINGLETHREAD;
const int Utility::THREAD_MODE_MULTI = SQLITE_CONFIG_MULTITHREAD;
const int Utility::THREAD_MODE_SERIAL = SQLITE_CONFIG_SERIALIZED;
int Utility::thread_mode_ =
#if (SQLITE_THREADSAFE == 0)
  SQLITE_CONFIG_SINGLETHREAD;
#elif (SQLITE_THREADSAFE == 1)
  SQLITE_CONFIG_SERIALIZED;
#elif (SQLITE_THREADSAFE == 2)
  SQLITE_CONFIG_MULTITHREAD;
#endif

const int Utility::OPERATION_INSERT = SQLITE_INSERT;
const int Utility::OPERATION_DELETE = SQLITE_DELETE;
const int Utility::OPERATION_UPDATE = SQLITE_UPDATE;

const String Utility::SQLITE_DATE_FORMAT = "%Y-%m-%d";
const String Utility::SQLITE_TIME_FORMAT = "%H:%M:%S";
Utility::TypeMap Utility::types_;
fun::Mutex Utility::mutex_;

Utility::SQLiteMutex::SQLiteMutex(sqlite3* db) : mutex_((db) ? sqlite3_db_mutex(db) : 0) {
  if (mutex_) {
    sqlite3_mutex_enter(mutex_);
  }
}

Utility::SQLiteMutex::~SQLiteMutex() {
  if (mutex_) {
    sqlite3_mutex_leave(mutex_);
  }
}

Utility::Utility() {
  InitializeDefaultTypes();
}

void Utility::InitializeDefaultTypes() {
  if (types_.IsEmpty()) {
    types_.insert(TypeMap::value_type("", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("BOOL", MetaColumn::FDT_BOOL));
    types_.insert(TypeMap::value_type("BOOLEAN", MetaColumn::FDT_BOOL));
    types_.insert(TypeMap::value_type("BIT", MetaColumn::FDT_BOOL));
    types_.insert(TypeMap::value_type("UINT8", MetaColumn::FDT_UINT8));
    types_.insert(TypeMap::value_type("UTINY", MetaColumn::FDT_UINT8));
    types_.insert(TypeMap::value_type("UINTEGER8", MetaColumn::FDT_UINT8));
    types_.insert(TypeMap::value_type("INT8", MetaColumn::FDT_INT8));
    types_.insert(TypeMap::value_type("TINY", MetaColumn::FDT_INT8));
    types_.insert(TypeMap::value_type("INTEGER8", MetaColumn::FDT_INT8));
    types_.insert(TypeMap::value_type("UINT16", MetaColumn::FDT_UINT16));
    types_.insert(TypeMap::value_type("USHORT", MetaColumn::FDT_UINT16));
    types_.insert(TypeMap::value_type("UINTEGER16", MetaColumn::FDT_UINT16));
    types_.insert(TypeMap::value_type("INT16", MetaColumn::FDT_INT16));
    types_.insert(TypeMap::value_type("SHORT", MetaColumn::FDT_INT16));
    types_.insert(TypeMap::value_type("INTEGER16", MetaColumn::FDT_INT16));
    types_.insert(TypeMap::value_type("UINT", MetaColumn::FDT_UINT32));
    types_.insert(TypeMap::value_type("UINT32", MetaColumn::FDT_UINT32));
    types_.insert(TypeMap::value_type("UINTEGER32", MetaColumn::FDT_UINT32));
    types_.insert(TypeMap::value_type("INT", MetaColumn::FDT_INT32));
    types_.insert(TypeMap::value_type("INT32", MetaColumn::FDT_INT32));
    types_.insert(TypeMap::value_type("INTEGER", MetaColumn::FDT_INT64));
    types_.insert(TypeMap::value_type("INTEGER32", MetaColumn::FDT_INT32));
    types_.insert(TypeMap::value_type("UINT64", MetaColumn::FDT_UINT64));
    types_.insert(TypeMap::value_type("ULONG", MetaColumn::FDT_INT64));
    types_.insert(TypeMap::value_type("UINTEGER64", MetaColumn::FDT_UINT64));
    types_.insert(TypeMap::value_type("INT64", MetaColumn::FDT_INT64));
    types_.insert(TypeMap::value_type("LONG", MetaColumn::FDT_INT64));
    types_.insert(TypeMap::value_type("INTEGER64", MetaColumn::FDT_INT64));
    types_.insert(TypeMap::value_type("TINYINT", MetaColumn::FDT_INT8));
    types_.insert(TypeMap::value_type("SMALLINT", MetaColumn::FDT_INT16));
    types_.insert(TypeMap::value_type("BIGINT", MetaColumn::FDT_INT64));
    types_.insert(TypeMap::value_type("LONGINT", MetaColumn::FDT_INT64));
    types_.insert(TypeMap::value_type("COUNTER", MetaColumn::FDT_UINT64));
    types_.insert(TypeMap::value_type("AUTOINCREMENT", MetaColumn::FDT_UINT64));
    types_.insert(TypeMap::value_type("REAL", MetaColumn::FDT_DOUBLE));
    types_.insert(TypeMap::value_type("FLOA", MetaColumn::FDT_DOUBLE));
    types_.insert(TypeMap::value_type("FLOAT", MetaColumn::FDT_DOUBLE));
    types_.insert(TypeMap::value_type("DOUB", MetaColumn::FDT_DOUBLE));
    types_.insert(TypeMap::value_type("DOUBLE", MetaColumn::FDT_DOUBLE));
    types_.insert(TypeMap::value_type("DECIMAL", MetaColumn::FDT_DOUBLE));
    types_.insert(TypeMap::value_type("NUMERIC", MetaColumn::FDT_DOUBLE));
    types_.insert(TypeMap::value_type("CHAR", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("CLOB", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("TEXT", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("VARCHAR", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("NCHAR", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("NCLOB", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("NTEXT", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("NVARCHAR", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("LONGVARCHAR", MetaColumn::FDT_STRING));
    types_.insert(TypeMap::value_type("BLOB", MetaColumn::FDT_BLOB));
    types_.insert(TypeMap::value_type("DATE", MetaColumn::FDT_DATE));
    types_.insert(TypeMap::value_type("TIME", MetaColumn::FDT_TIME));
    types_.insert(TypeMap::value_type("DATETIME", MetaColumn::FDT_TIMESTAMP));
    types_.insert(TypeMap::value_type("TIMESTAMP", MetaColumn::FDT_TIMESTAMP));
  }
}

void Utility::AddColumnType(String sqlite_type, MetaColumn::ColumnDataType fun_type) {
  // Check for errors in the mapping
  if (MetaColumn::FDT_UNKNOWN == fun_type) {
    throw fun::sql::NotSupportedException("Cannot map to unknown fun type.");
  }

  // Initialize default types
  InitializeDefaultTypes();

  // Add type to internal map
  types_[sqlite_type] = fun_type.ToUpper();
}

String Utility::GetLastError(sqlite3* db) {
  String error;
  SQLiteMutex m(db);
  const char* sqlite_err = sqlite3_errmsg(db);
  if (sqlite_err) {
    error = sqlite_err;
  }
  return error;
}

MetaColumn::ColumnDataType Utility::GetColumnType(sqlite3_stmt* stmt, size_t pos) {
  fun_check_dbg(stmt);

  // Ensure statics are initialized
  {
    fun::Mutex::ScopedLock lock(mutex_);
    static Utility u;
  }

  const char* pc = sqlite3_column_decltype(stmt, (int)pos);
  String sqlite_type = pc ? pc : "";
  fun::toUpperInPlace(sqlite_type);
  sqlite_type = sqlite_type.substr(0, sqlite_type.find_first_of(" ("));

  TypeMap::const_iterator it = types_.find(fun::trimInPlace(sqlite_type));
  if (types_.end() == it) {
    return MetaColumn::FDT_BLOB;
  } else {
    return it->second;
  }
}

void Utility::ThrowException(sqlite3* db, int rc, const String& error_msg) {
  switch (rc) {
    case SQLITE_OK:
      break;
    case SQLITE_ERROR:
      throw InvalidSQLStatementException(GetLastError(db), error_msg);
    case SQLITE_INTERNAL:
      throw InternalDBErrorException(GetLastError(db), error_msg);
    case SQLITE_PERM:
      throw DBAccessDeniedException(GetLastError(db), error_msg);
    case SQLITE_ABORT:
      throw ExecutionAbortedException(GetLastError(db), error_msg);
    case SQLITE_BUSY:
    case SQLITE_BUSY_RECOVERY:
#if defined(SQLITE_BUSY_SNAPSHOT)
    case SQLITE_BUSY_SNAPSHOT:
#endif
      throw DBLockedException(GetLastError(db), error_msg);
    case SQLITE_LOCKED:
      throw TableLockedException(GetLastError(db), error_msg);
    case SQLITE_NOMEM:
      throw NoMemoryException(GetLastError(db), error_msg);
    case SQLITE_READONLY:
      throw ReadOnlyException(GetLastError(db), error_msg);
    case SQLITE_INTERRUPT:
      throw InterruptException(GetLastError(db), error_msg);
    case SQLITE_IOERR:
      throw IOErrorException(GetLastError(db), error_msg);
    case SQLITE_CORRUPT:
      throw CorruptImageException(GetLastError(db), error_msg);
    case SQLITE_NOTFOUND:
      throw TableNotFoundException(GetLastError(db), error_msg);
    case SQLITE_FULL:
      throw DatabaseFullException(GetLastError(db), error_msg);
    case SQLITE_CANTOPEN:
      throw CantOpenDBFileException(GetLastError(db), error_msg);
    case SQLITE_PROTOCOL:
      throw LockProtocolException(GetLastError(db), error_msg);
    case SQLITE_EMPTY:
      throw InternalDBErrorException(GetLastError(db), error_msg);
    case SQLITE_SCHEMA:
      throw SchemaDiffersException(GetLastError(db), error_msg);
    case SQLITE_TOOBIG:
      throw RowTooBigException(GetLastError(db), error_msg);
    case SQLITE_CONSTRAINT:
      throw ConstraintViolationException(GetLastError(db), error_msg);
    case SQLITE_MISMATCH:
      throw DataTypeMismatchException(GetLastError(db), error_msg);
    case SQLITE_MISUSE:
      throw InvalidLibraryUseException(GetLastError(db), error_msg);
    case SQLITE_NOLFS:
      throw OSFeaturesMissingException(GetLastError(db), error_msg);
    case SQLITE_AUTH:
      throw AuthorizationDeniedException(GetLastError(db), error_msg);
    case SQLITE_FORMAT:
      throw CorruptImageException(GetLastError(db), error_msg);
    case SQLITE_NOTADB:
      throw CorruptImageException(GetLastError(db), error_msg);
    case SQLITE_RANGE:
      throw InvalidSQLStatementException(GetLastError(db), error_msg);
    case SQLITE_ROW:
      break; // sqlite_step() has another row ready
    case SQLITE_DONE:
      break; // sqlite_step() has finished executing
    default:
      throw SQLiteException(fun::Format("Unknown error code: %d", rc), error_msg);
  }
}

bool Utility::FileToMemory(sqlite3* memory, const String& filename) {
  int rc;
  sqlite3* file;
  sqlite3_backup* backup;

  rc = sqlite3_open_v2(filename.c_str(), &file, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL);
  if(rc == SQLITE_OK ) {
    backup = sqlite3_backup_init(memory, "main", file, "main");
    if( backup ) {
      sqlite3_backup_step(backup, -1);
      sqlite3_backup_finish(backup);
    }
    rc = sqlite3_errcode(file);
  }

  sqlite3_close(file);
  return SQLITE_OK == rc;
}

bool Utility::MemoryToFile(const String& filename, sqlite3* memory) {
  int rc;
  sqlite3* file;
  sqlite3_backup* backup;

  rc = sqlite3_open_v2(filename.c_str(), &file, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, NULL);
  if(rc == SQLITE_OK ) {
    backup = sqlite3_backup_init(file, "main", memory, "main");
    if( backup ) {
      sqlite3_backup_step(backup, -1);
      sqlite3_backup_finish(backup);
    }
    rc = sqlite3_errcode(file);
  }

  sqlite3_close(file);
  return SQLITE_OK == rc;
}

bool Utility::IsThreadSafe() {
  return 0 != sqlite3_threadsafe();
}

int Utility::GetThreadMode() {
  return thread_mode_;
}

bool Utility::SetThreadMode(int mode) {
#if (SQLITE_THREADSAFE != 0)
  if (SQLITE_OK == sqlite3_shutdown())   {
    if (SQLITE_OK == sqlite3_config(mode)) {
      thread_mode_ = mode;
      if (SQLITE_OK == sqlite3_initialize()) {
        return true;
      }
    }
    sqlite3_initialize();
  }
#endif
  return false;
}

void* Utility::EventHookRegister(sqlite3* db, UpdateCallbackType callback_fn, void* param) {
  typedef void(*pF)(void*, int, const char*, const char*, sqlite3_int64);
  return sqlite3_update_hook(db, reinterpret_cast<pF>(callback_fn), param);
}

void* Utility::EventHookRegister(sqlite3* db, CommitCallbackType callback_fn, void* param) {
  return sqlite3_commit_hook(db, callback_fn, param);
}

void* Utility::EventHookRegister(sqlite3* db, RollbackCallbackType callback_fn, void* param) {
  return sqlite3_rollback_hook(db, callback_fn, param);
}

// NOTE: Utility::GetDbHandle() has been moved to SessionImpl.cpp,
// as a workaround for a failing AnyCast with Clang.
// See <https://github.com/funproject/fun/issues/578>
// for a discussion.

} // namespace sqlite
} // namespace sql
} // namespace fun
