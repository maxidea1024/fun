#pragma once

#include "fun/sql/postgresql/postgresql.h"
#include "fun/sql/session_impl.h"
#include "fun/sql/connector.h"

#include "fun/base/ref_counted_ptr.h"

#include <string>

// Note: to avoid static (de)initialization problems,
// during connector automatic (un)registration, it is
// best to have this as a macro.

#define FUN_DATA_POSTGRESQL_CONNECTOR_NAME  "postgresql"

namespace fun {
namespace sql {
namespace postgresql {

/**
 * Connector instantiates PostgreSQL SessionImpl objects.
 */
class FUN_POSTGRESQL_API Connector : public fun::sql::Connector {
 public:
  static String KEY;

  /**
   * Creates the Connector.
   */
  Connector();

  /**
   * Destroys the Connector.
   */
  virtual ~Connector();

  /**
   * Returns the name associated with this connector.
   */
  virtual const String& GetName() const;

  /**
   * Creates a PostgreSQL SessionImpl object and initializes it with the given connection_string.
   */
  virtual fun::sql::SessionImpl::Ptr
  CreateSession(const String&  connection_string,
                size_t timeout = fun::sql::SessionImpl::LOGIN_TIMEOUT_DEFAULT);

  /**
   * Registers the Connector under the Keyword Connector::KEY at the fun::sql::SessionFactory
   */
  static void RegisterConnector();

  /**
   * Unregisters the Connector under the Keyword Connector::KEY at the fun::sql::SessionFactory
   */
  static void UnregisterConnector();
};

} // namespace postgresql
} // namespace sql
} // namespace fun


//
// Automatic Connector registration
//

/**
 * Connector registering class.
 * A global instance of this class is instantiated
 * with sole purpose to automatically register the
 * PostgreSQL connector with central fun Data registry.
 */
struct FUN_POSTGRESQL_API PostgreSqlConnectorRegistrator {
  /**
   * Calls fun::sql::postgresql::RegisterConnector();
   */
  PostgreSqlConnectorRegistrator() {
    fun::sql::postgresql::Connector::RegisterConnector();
  }

  /**
   * Calls fun::sql::postgresql::UnregisterConnector();
   */
  ~PostgreSqlConnectorRegistrator() {
    fun::sql::postgresql::Connector::UnregisterConnector();
  }
};

#if !defined(FUN_NO_AUTOMATIC_LIB_INIT)
  #if defined(_MSC_VER)
    extern "C" const struct FUN_POSTGRESQL_API PostgreSqlConnectorRegistrator funPostgreSqlConnectorRegistrator;
    #if defined(FUN_POSTGRESQL_EXPORTS)
      #if defined(_WIN64)
        #define FUN_DATA_POSTGRESQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/export:"#s))
      #elif defined(_WIN32)
        #define FUN_DATA_POSTGRESQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/export:_"#s))
      #endif
    #else  // !FUN_POSTGRESQL_EXPORTS
      #if defined(_WIN64)
        #define FUN_DATA_POSTGRESQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/include:"#s))
      #elif defined(_WIN32)
        #define FUN_DATA_POSTGRESQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/include:_"#s))
      #endif
    #endif // FUN_POSTGRESQL_EXPORTS
  #else // !FUN_PLATFORM_WINDOWS_FAMILY
      #define FUN_DATA_POSTGRESQL_FORCE_SYMBOL(s) extern "C" const struct PostgreSqlConnectorRegistrator s;
  #endif // FUN_PLATFORM_WINDOWS_FAMILY
  FUN_DATA_POSTGRESQL_FORCE_SYMBOL(funPostgreSqlConnectorRegistrator)
#endif // FUN_NO_AUTOMATIC_LIB_INIT

//
// End automatic Connector registration
//
