#pragma once

#include "fun/sql/connector.h"
#include "fun/sql/sqlite/sqlite.h"

// Note: to avoid static (de)initialization problems,
// during connector automatic (un)registration, it is
// best to have this as a macro.
#define FUN_DATA_SQLITE_CONNECTOR_NAME "sqlite"

namespace fun {
namespace sql {
namespace sqlite {

/**
 * Connector instantiates SqLite SessionImpl objects.
 */
class FUN_SQLITE_API Connector : public fun::sql::Connector {
 public:
  /**
   * Keyword for creating SQLite sessions ("SQLite").
   */
  static const String KEY;

  /**
   * Creates the Connector.
   */
  Connector();

  /**
   * Destroys the Connector.
   */
  ~Connector();

  /**
   * Returns the name associated with this connector.
   */
  const String& GetName() const;

  /**
   * Creates a SQLite SessionImpl object and initializes it with the given
   * connection_string.
   */
  fun::RefCountedPtr<fun::sql::SessionImpl> CreateSession(
      const String& connection_string,
      size_t timeout = fun::sql::SessionImpl::LOGIN_TIMEOUT_DEFAULT);

  /**
   * Registers the Connector under the Keyword Connector::KEY at the
   * fun::sql::SessionFactory.
   */
  static void RegisterConnector();

  /**
   * Unregisters the Connector under the Keyword Connector::KEY at the
   * fun::sql::SessionFactory.
   */
  static void UnregisterConnector();

  /**
   * Enables or disables SQlite shared cache mode
   * (see http://www.sqlite.org/sharedcache.html for a discussion).
   */
  static void EnableSharedCache(bool flag = true);

  /**
   * Sets a soft upper limit to the amount of memory allocated
   * by SQLite. For more information, please see the SQLite
   * sqlite_soft_heap_limit() function
   * (http://www.sqlite.org/c3ref/soft_heap_limit.html).
   */
  static void EnableSoftHeapLimit(int limit);
};

//
// inlines
//

inline const String& Connector::GetName() const {
  static const String n(FUN_DATA_SQLITE_CONNECTOR_NAME);
  return n;
}

}  // namespace sqlite
}  // namespace sql
}  // namespace fun

//
// Automatic Connector registration
//

/**
 * Connector registering class.
 * A global instance of this class is instantiated
 * with sole purpose to automatically register the
 * SQLite connector with central fun Data registry.
 */
struct FUN_SQLITE_API SQLiteConnectorRegistrator {
  /**
   * Calls fun::sql::sqlite::RegisterConnector();
   */
  SQLiteConnectorRegistrator() {
    fun::sql::sqlite::Connector::RegisterConnector();
  }

  /**
   * Calls fun::sql::sqlite::UnregisterConnector();
   */
  ~SQLiteConnectorRegistrator() {
    try {
      fun::sql::sqlite::Connector::UnregisterConnector();
    } catch (...) {
      fun_unexpected();
    }
  }
};

// TODO 수정해야함!!

#if !defined(FUN_NO_AUTOMATIC_LIB_INIT)
#if defined(FUN_PLATFORM_WINDOWS_FAMILY) && !defined(__GNUC__)
extern "C" const struct FUN_SQLITE_API SQLiteConnectorRegistrator
    funSQLiteConnectorRegistrator;
#if defined(FUN_SQLITE_EXPORTS)
#if defined(_WIN64) || defined(_WIN32_WCE)
#define FUN_DATA_SQLITE_FORCE_SYMBOL(s) __pragma(comment(linker, "/export:" #s))
#elif defined(_WIN32)
#define FUN_DATA_SQLITE_FORCE_SYMBOL(s) \
  __pragma(comment(linker, "/export:_" #s))
#endif
#else  // !FUN_SQLITE_EXPORTS
#if defined(_WIN64) || defined(_WIN32_WCE)
#define FUN_DATA_SQLITE_FORCE_SYMBOL(s) \
  __pragma(comment(linker, "/include:" #s))
#elif defined(_WIN32)
#define FUN_DATA_SQLITE_FORCE_SYMBOL(s) \
  __pragma(comment(linker, "/include:_" #s))
#endif
#endif  // FUN_SQLITE_EXPORTS
#else   // !FUN_PLATFORM_WINDOWS_FAMILY
#define FUN_DATA_SQLITE_FORCE_SYMBOL(s) \
  extern "C" const struct SQLiteConnectorRegistrator s;
#endif  // FUN_PLATFORM_WINDOWS_FAMILY
FUN_DATA_SQLITE_FORCE_SYMBOL(funSQLiteConnectorRegistrator)
#endif  // FUN_NO_AUTOMATIC_LIB_INIT

//
// End automatic Connector registration
//
