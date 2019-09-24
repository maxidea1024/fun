#include "fun/sql/odbc/connection_handle.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/odbc/utility.h"

namespace fun {
namespace sql {
namespace odbc {

ConnectionHandle::ConnectionHandle(EnvironmentHandle* environment)
    : environment_(environment ? &environment->GetHandle() : nullptr),
      hdbc_(SQL_NULL_HDBC) {
  if (Utility::IsError(
          SQLAllocHandle(SQL_HANDLE_DBC, environment_.GetHandle(), &hdbc_))) {
    throw OdbcException("Could not allocate connection handle.");
  }
}

ConnectionHandle::~ConnectionHandle() {
  try {
    if (hdbc_ != SQL_NULL_HDBC) {
      SQLDisconnect(hdbc_);
      SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
      hdbc_ = SQL_NULL_HDBC;

      fun_check(!Utility::IsError(rc));
    }
  } catch (...) {
    fun_unexpected();
  }
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
