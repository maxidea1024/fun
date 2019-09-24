#pragma once

#include "fun/sql/postgresql/postgresql.h"
#include "fun/sql/postgresql/session_handle.h"
#include "fun/sql/session.h"

namespace fun {
namespace sql {
namespace postgresql {

/**
 * Various utility functions for PostgreSQL.
 */
class FUN_POSTGRESQL_API Utility {
 public:
  /**
   * Returns server info.
   */
  static String GetServerInfo(SessionHandle* handle);

  /**
   * Returns server info.
   */
  static String GetServerInfo(fun::sql::Session& session);

  /**
   * Returns server version.
   */
  static int GetServerVersion(SessionHandle* handle);

  /**
   * Returns server version.
   */
  static int GetServerVersion(fun::sql::Session& session);

  /**
   * Returns host info.
   */
  static String GetHostInfo(SessionHandle* handle);

  /**
   * Returns host info.
   */
  static String GetHostInfo(fun::sql::Session& session);

  /**
   * Returns session encoding.
   */
  static String GetSessionEncoding(SessionHandle* handle);

  /**
   * Returns session encoding.
   */
  static String GetSessionEncoding(fun::sql::Session& session);

  /**
   * Rturns true if microseconds are suported.
   */
  static bool HasMicrosecond() { return true; }

  /**
   * Returns native PostgreSQL handle for the session.
   */
  static SessionHandle* GetHandle(fun::sql::Session& session);
};

//
// inlines
//

inline SessionHandle* Utility::GetHandle(Session& session) {
  return fun::AnyCast<SessionHandle*>(session.GetProperty("handle"));
}

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
