#pragma once

#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/environment_handle.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqltypes.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * ODBC connection handle class
 */
class FUN_ODBC_API ConnectionHandle {
 public:
  /**
   * Creates the ConnectionHandle.
   */
  ConnectionHandle(EnvironmentHandle* environment = nullptr);

  /**
   * Creates the ConnectionHandle.
   */
  ~ConnectionHandle();

  /**
   * Const conversion operator into reference to native type.
   */
  operator const SQLHDBC& () const;

  /**
   * Returns const reference to handle;
   */
  const SQLHDBC& handle() const;

  /**
   * Returns true if the handle is valid
   */
  operator bool() const;

 private:
  /**
   * Conversion operator into reference to native type.
   */
  operator SQLHDBC& ();

  /**
   * Returns reference to handle;
   */
  SQLHDBC& GetHandle();

  ConnectionHandle(const ConnectionHandle&) = delete;
  const ConnectionHandle& operator=(const ConnectionHandle&) = delete;

  const EnvironmentHandle environment_;
  SQLHDBC hdbc_;
};


//
// inlines
//

inline ConnectionHandle::operator const SQLHDBC& () const {
  return GetHandle();
}

inline const SQLHDBC& ConnectionHandle::GetHandle() const {
  return hdbc_;
}

inline ConnectionHandle::operator SQLHDBC& () {
  return GetHandle();
}

inline SQLHDBC& ConnectionHandle::GetHandle() {
  return hdbc_;
}

inline ConnectionHandle::operator bool () const {
  return hdbc_ != SQL_NULL_HDBC;
}

} // namespace odbc
} // namespace sql
} // namespace fun
