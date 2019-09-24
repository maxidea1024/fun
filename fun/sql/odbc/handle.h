#pragma once

#include "fun/sql/odbc/connection_handle.h"
#include "fun/sql/odbc/environment_handle.h"
#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/odbc/utility.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqltypes.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * ODBC handle class template
 */
template <typename H, SQLSMALLINT handle_type>
class Handle {
 public:
  /**
   * Creates the Handle.
   */
  Handle(const ConnectionHandle& connection)
      : connection_(connection), handle_(0) {
    if (Utility::IsError(SQLAllocHandle(handle_type, connection_, &handle_))) {
      throw OdbcException("Could not allocate statement handle.");
    }
  }

  /**
   * Destroys the Handle.
   */
  ~Handle() {
    try {
#if defined(_DEBUG)
      SQLRETURN rc =
#endif
          SQLFreeHandle(handle_type, handle_);
      // N.B. Destructors should not throw, but neither do we want to
      // leak resources. So, we throw here in debug mode if things go bad.
      fun_check_dbg(!Utility::IsError(rc));
    } catch (...) {
      fun_unexpected();
    }
  }

  /**
   * Const conversion operator into reference to native type.
   */
  operator const H&() const { return handle(); }

  /**
   * Returns const reference to native type.
   */
  const H& GetHandle() const { return handle_; }

 private:
  Handle(const Handle&);
  const Handle& operator=(const Handle&);

  /**
   * Conversion operator into reference to native type.
   */
  operator H&() { return GetHandle(); }

  /**
   * Returns reference to native type.
   */
  H& GetHandle() { return handle_; }

  const ConnectionHandle& connection_;
  H handle_;

  friend class OdbcStatementImpl;
};

typedef Handle<SQLHSTMT, SQL_HANDLE_STMT> StatementHandle;
typedef Handle<SQLHDESC, SQL_HANDLE_DESC> DescriptorHandle;

}  // namespace odbc
}  // namespace sql
}  // namespace fun
