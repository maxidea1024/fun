#pragma once

#include "fun/sql/mysql/mysql.h"
#include "fun/sql/session.h"

struct st_mysql;
typedef struct st_mysql MYSQL;

namespace fun {
namespace sql {
namespace mysql {

/**
 * Various utility functions for MySQL.
 */
class FUN_MYSQL_API Utility {
 public:
  /**
   * Returns server info.
   */
  static String GetServerInfo(MYSQL* handle);

  /**
   * Returns server info.
   */
  static String GetServerInfo(fun::sql::Session& session);

  /**
   * Returns server version.
   */
  static unsigned long GetServerVersion(MYSQL* handle);

  /**
   * Returns server version.
   */
  static unsigned long GetServerVersion(fun::sql::Session& session);

  /**
   * Returns host info.
   */
  static String GetHostInfo(MYSQL* handle);

  /**
   * Returns host info.
   */
  static String GetHostInfo(fun::sql::Session& session);

  /**
   * Returns true if microseconds are supported.
   */
  static bool HasMicrosecond();

  /**
   * Returns native MySQL handle for the session.
   */
  static MYSQL* GetHandle(fun::sql::Session& session);
};


//
// inlines
//

inline MYSQL* Utility::GetHandle(Session& session) {
  return fun::AnyCast<MYSQL*>(session.GetProperty("handle"));
}

} // namespace mysql
} // namespace sql
} // namespace fun
