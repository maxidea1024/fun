#pragma once

#include "fun/sql/sqlite/sqlite.h"
#include "fun/sql/meta_column.h"
#include "fun/sql/session.h"
#include "fun/base/mutex.h"
#include "fun/types.h"
#include <map>

extern "C" {
  typedef struct sqlite3 sqlite3;
  typedef struct sqlite3_stmt sqlite3_stmt;
  typedef struct sqlite3_mutex* mutex_;
}

namespace fun {
namespace sql {
namespace sqlite {

/**
 * Various utility functions for SQLite.
 */
class FUN_SQLITE_API Utility {
 public:
  static const String SQLITE_DATE_FORMAT;
  static const String SQLITE_TIME_FORMAT;
  typedef std::map<String, MetaColumn::ColumnDataType> TypeMap;

  static const int THREAD_MODE_SINGLE;
  static const int THREAD_MODE_MULTI;
  static const int THREAD_MODE_SERIAL;

  static const int OPERATION_INSERT;
  static const int OPERATION_DELETE;
  static const int OPERATION_UPDATE;

  /**
   * Adds or replaces the mapping for SQLite column type sqlite_type
   * to a fun type fun_type.
   *
   * sqlite_type is a case-insensitive description of the column type with
   * any value fun_type value but MetaColumn::FDT_UNKNOWN.
   * A fun::sql::NotSupportedException is thrown if fun_type is invalid.
   */
  static void AddColumnType(String sqlite_type, MetaColumn::ColumnDataType fun_type);

  /**
   * Returns native DB handle.
   */
  static sqlite3* GetDbHandle(const Session& session);

  /**
   * Retreives the last error code from sqlite and converts it to a string.
   */
  static String GetLastError(sqlite3* db);

  /**
   * Retrieves the last error code from sqlite and converts it to a string.
   */
  static String GetLastError(const Session& session);

  /**
   * Throws for an error code the appropriate exception
   */
  static void ThrowException(sqlite3* db, int rc, const String& error_msg = String());

  /**
   * Returns column data type.
   */
  static MetaColumn::ColumnDataType GetColumnType(sqlite3_stmt* stmt, size_t pos);

  /**
   * Loads the contents of a database file on disk into an opened
   * database in memory.
   *
   * Returns true if successful.
   */
  static bool FileToMemory(sqlite3* memory, const String& filename);

  /**
   * Loads the contents of a database file on disk into an opened
   * database in memory.
   *
   * Returns true if successful.
   */
  static bool FileToMemory(const Session& session, const String& filename);

  /**
   * Saves the contents of an opened database in memory to the
   * database on disk.
   *
   * Returns true if successful.
   */
  static bool MemoryToFile(const String& filename, sqlite3* memory);

  /**
   * Saves the contents of an opened database in memory to the
   * database on disk.
   *
   * Returns true if successful.
   */
  static bool MemoryToFile(const String& filename, const Session& session);

  /**
   * Returns true if SQLite was compiled in multi-thread or serialized mode.
   * See http://www.sqlite.org/c3ref/threadsafe.html for details.
   *
   * Returns true if successful
   */
  static bool IsThreadSafe();

  /**
   * Sets the threading mode to single, multi or serialized.
   * See http://www.sqlite.org/threadsafe.html for details.
   *
   * Returns true if successful
   */
  static bool SetThreadMode(int mode);

  /**
   * Returns the thread mode.
   */
  static int GetThreadMode();

  /**
   * Update callback function type.
   */
  typedef void(*UpdateCallbackType)(void*, int, const char*, const char*, int64);

  /**
   * Commit callback function type.
   */
  typedef int(*CommitCallbackType)(void*);

  /**
   * Rollback callback function type.
   */
  typedef void(*RollbackCallbackType)(void*);

  /**
   * Registers the callback for (1)(insert, delete, update), (2)(commit) or
   * or (3)(rollback) events. Only one function per group can be registered
   * at a time. Registration is not thread-safe. Storage pointed to by param
   * must remain valid as long as registration is active. Registering with
   * callback_fn set to zero disables notifications.
   *
   * See http://www.sqlite.org/c3ref/update_hook.html and
   * http://www.sqlite.org/c3ref/commit_hook.html for details.
   */
  template <typename T, typename CBT>
  static bool RegisterUpdateHandler(sqlite3* db, CBT callback_fn, T* param) {
    typedef std::pair<CBT, T*> CBPair;
    typedef std::multimap<sqlite3*, CBPair> CBMap;
    typedef typename CBMap::iterator CBMapIt;
    typedef std::pair<CBMapIt, CBMapIt> CBMapItPair;

    static CBMap retMap;
    T* pRet = reinterpret_cast<T*>(EventHookRegister(db, callback_fn, param));

    if (pRet == 0) {
      if (retMap.find(db) == retMap.end()) {
        retMap.insert(std::make_pair(db, CBPair(callback_fn, param)));
        return true;
      }
    } else {
      CBMapItPair retMapRange = retMap.equal_range(db);
      for (CBMapIt it = retMapRange.first; it != retMapRange.second; ++it) {
        fun_check(it->second.first != 0);
        if ((callback_fn == 0) && (*pRet == *it->second.second)) {
          retMap.erase(it);
          return true;
        }

        if ((callback_fn == it->second.first) && (*pRet == *it->second.second)) {
          it->second.second = param;
          return true;
        }
      }
    }

    return false;
  }

  /**
   * Registers the callback by calling RegisterUpdateHandler(sqlite3*, CBT, T*).
   */
  template <typename T, typename CBT>
  static bool RegisterUpdateHandler(const Session& session, CBT callback_fn, T* param) {
    return RegisterUpdateHandler(GetDbHandle(session), callback_fn, param);
  }

  class SQLiteMutex {
   public:
    SQLiteMutex(sqlite3* db);
    ~SQLiteMutex();

   private:
    SQLiteMutex();
    sqlite3_mutex* mutex_;
  };

 private:
  /**
   * Maps SQLite column declared types to fun::sql types through
   * static TypeMap member.
   *
   * Note: SQLite is type-agnostic and it is the end-user responsibility
   * to ensure that column declared data type corresponds to the type of
   * data actually held in the database.
   *
   * Column types are case-insensitive.
   */
  Utility();

  Utility(const Utility&);
  Utility& operator = (const Utility&);

  static void InitializeDefaultTypes();

  static void* EventHookRegister(sqlite3* db, UpdateCallbackType callback_fn, void* param);
  static void* EventHookRegister(sqlite3* db, CommitCallbackType callback_fn, void* param);
  static void* EventHookRegister(sqlite3* db, RollbackCallbackType callback_fn, void* param);

  static TypeMap types_;
  static fun::Mutex mutex_;
  static int thread_mode_;
};


//
// inlines
//

inline String Utility::GetLastError(const Session& session) {
  fun_check_dbg((0 == icompare(session.GetConnector(), 0, 6, "sqlite")));
  return GetLastError(GetDbHandle(session));
}

inline bool Utility::MemoryToFile(const String& filename, const Session& session) {
  fun_check_dbg((0 == icompare(session.GetConnector(), 0, 6, "sqlite")));
  return MemoryToFile(filename, GetDbHandle(session));
}

inline bool Utility::FileToMemory(const Session& session, const String& filename) {
  fun_check_dbg((0 == icompare(session.GetConnector(), 0, 6, "sqlite")));
  return FileToMemory(GetDbHandle(session), filename);
}

} // namespace sqlite
} // namespace sql
} // namespace fun
