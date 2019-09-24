#pragma once

#include "fun/base/shared_ptr.h"
#include "fun/sql/session_impl.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * A Connector creates SessionImpl objects.
 *
 * Every connector library (like the SQLite or the ODBC connector)
 * provides a subclass of this class, an instance of which is
 * registered with the SessionFactory.
 */
class FUN_SQL_API Connector {
 public:
  typedef fun::SharedPtr<Connector> Ptr;

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
  virtual const String& GetName() const = 0;

  /**
   * Create a SessionImpl object and initialize it with the given
   * connection_string.
   */
  virtual SessionImpl::Ptr CreateSession(
      const String& connection_string,
      size_t timeout = SessionImpl::LOGIN_TIMEOUT_DEFAULT) = 0;
};

}  // namespace sql
}  // namespace fun
