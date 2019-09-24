#pragma once

#include "fun/sql/mysql/mysql.h"
#include "fun/sql/connector.h"
#include "fun/base/mutex.h"

// Note: to avoid static (de)initialization problems,
// during connector automatic (un)registration, it is
// best to have this as a macro.
#define FUN_DATA_MYSQL_CONNECTOR_NAME  "mysql"

namespace fun {
namespace sql {
namespace mysql {

/**
 * Connector instantiates MySQL SessionImpl objects.
 */
class FUN_MYSQL_API Connector : public fun::sql::Connector {
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
  virtual const String& name() const;

  /**
   * Creates a MySQL SessionImpl object and initializes it with the given connection_string.
   */
  virtual fun::RefCountedPtr<fun::sql::SessionImpl>
  CreateSession(const String& connection_string,
                size_t timeout = fun::sql::SessionImpl::LOGIN_TIMEOUT_DEFAULT);

  /**
   * Registers the Connector under the Keyword Connector::KEY at the fun::sql::SessionFactory
   */
  static void RegisterConnector();

  /**
   * Unregisters the Connector under the Keyword Connector::KEY at the fun::sql::SessionFactory
   */
  static void UnregisterConnector();

  static fun::FastMutex mutex_;
};

} // namespace mysql
} // namespace sql
} // namespace fun


//
// Automatic Connector registration
//

/**
 * Connector registering class.
 * A global instance of this class is instantiated
 * with sole purpose to automatically register the
 * MySQL connector with central fun Data registry.
 */
struct FUN_MYSQL_API MySqlConnectorRegistrator {
  /**
   * Calls fun::sql::MySql::RegisterConnector();
   */
  MySqlConnectorRegistrator() {
    fun::sql::MySql::Connector::RegisterConnector();
  }

  /**
   ** Calls fun::sql::MySql::UnregisterConnector();
   */
  ~MySqlConnectorRegistrator() {
    try {
      fun::sql::MySql::Connector::UnregisterConnector();
    } catch (...) {
      fun_unexpected();
    }
  }
};


#if !defined(FUN_NO_AUTOMATIC_LIB_INIT)
  #if defined(FUN_PLATFORM_WINDOWS_FAMILY) && !defined(__GNUC__)
    extern "C" const struct FUN_MYSQL_API MySqlConnectorRegistrator funMySqlConnectorRegistrator;
    #if defined(FUN_MYSQL_EXPORTS)
      #if defined(_WIN64)
        #define FUN_DATA_MYSQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/export:"#s))
      #elif defined(_WIN32)
        #define FUN_DATA_MYSQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/export:_"#s))
      #endif
    #else  // !FUN_MYSQL_EXPORTS
      #if defined(_WIN64)
        #define FUN_DATA_MYSQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/include:"#s))
      #elif defined(_WIN32)
        #define FUN_DATA_MYSQL_FORCE_SYMBOL(s) __pragma(comment (linker, "/include:_"#s))
      #endif
    #endif // FUN_MYSQL_EXPORTS
  #else // !FUN_PLATFORM_WINDOWS_FAMILY
      #define FUN_DATA_MYSQL_FORCE_SYMBOL(s) extern "C" const struct MySqlConnectorRegistrator s;
  #endif // FUN_PLATFORM_WINDOWS_FAMILY
  FUN_DATA_MYSQL_FORCE_SYMBOL(funMySqlConnectorRegistrator)
#endif // FUN_NO_AUTOMATIC_LIB_INIT

//
// End automatic Connector registration
//
