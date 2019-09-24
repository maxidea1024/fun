#include "fun/sql/odbc/environment_handle.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/odbc/odbc_exception.h"

namespace fun {
namespace sql {
namespace odbc {

EnvironmentHandle::EnvironmentHandle()
  : henv_(SQL_NULL_HENV), is_owner_(false) {
  Init();
}

EnvironmentHandle::EnvironmentHandle(const SQLHENV* henv)
  : henv_(SQL_NULL_HENV), is_owner_(false) {
  if (!henv || *henv == SQL_NULL_HENV) {
    Init();
  } else {
    henv_ = *henv;
  }
}

void EnvironmentHandle::Init() {
  if (Utility::IsError(SQLAllocHandle(SQL_HANDLE_ENV,
      SQL_NULL_HANDLE,
      &henv_)) ||
      Utility::IsError(SQLSetEnvAttr(henv_,
      SQL_ATTR_ODBC_VERSION,
      (SQLPOINTER)SQL_OV_ODBC3,
      0))) {
    throw OdbcException("Could not initialize environment.");
  }

  is_owner_ = true;
}

EnvironmentHandle::~EnvironmentHandle() {
  try {
    if (is_owner_ && henv_ != SQL_NULL_HENV) {
      SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, henv_);
      henv_ = SQL_NULL_HENV;
      fun_check(!Utility::IsError(rc));
    }
  } catch (...) {
    fun_unexpected();
  }
}

} // namespace odbc
} // namespace sql
} // namespace fun
