#include "fun/sql/odbc/odbc.h"

#if defined(FUN_ODBC_UNICODE_WINDOWS)
#include "unicode_win32_odbc.cc"
#elif defined(FUN_ODBC_UNICODE_UNIXODBC)
#include "unicode_unix_odbc.cc"
#endif
