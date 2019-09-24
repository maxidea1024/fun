#pragma once

#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/odbc/diagnostics.h"
#include "fun/base/format.h"
#include <vector>

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqlext.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * Class encapsulating ODBC diagnostic record collection. Collection is generated
 * during construction. Class provides access and string generation for the collection
 * as well as individual diagnostic records.
 */
template <typename H, SQLSMALLINT handleType>
class Error {
 public:
  /**
   * Creates the Error.
   */
  explicit Error(const H& handle) : diagnostics_(handle) {}

  /**
   * Destroys the Error.
   */
  ~Error() {}

  /**
   * Returns the associated diagnostics.
   */
  const Diagnostics<H, handleType>& diagnostics() const {
    return diagnostics_;
  }

  /**
   * Returns the count of diagnostic records.
   */
  int count() const {
    return (int) diagnostics_.count();
  }

  /**
   * Generates the string for the diagnostic record.
   */
  String& ToString(int index, String& str) const {
    if ((index < 0) || (index > (count() - 1))) {
      return str;
    }

    String s;
    fun::Format(s,
      "===========================\n"
      "ODBC Diagnostic record #%d:\n"
      "===========================\n"
      "SQLSTATE = %s\nNative Error Code = %ld\n%s\n\n",
      index + 1,
      diagnostics_.sqlState(index),
      diagnostics_.nativeError(index),
      diagnostics_.message(index));

    str.append(s);

    return str;
  }

  /**
   * Generates the string for the diagnostic record collection.
   */
  String ToString() const {
    String str;

    fun::Format(str,
      "Connection:%s\nServer:%s\n",
      diagnostics_.connectionName(),
      diagnostics_.serverName());

    String s;
    for (int i = 0; i < count(); ++i) {
      s.clear();
      str.append(ToString(i, s));
    }

    return str;
  }

 private:
  Error();

  Diagnostics<H, handleType> diagnostics_;
};


typedef Error<SQLHENV, SQL_HANDLE_ENV> EnvironmentError;
typedef Error<SQLHDBC, SQL_HANDLE_DBC> ConnectionError;
typedef Error<SQLHSTMT, SQL_HANDLE_STMT> StatementError;
typedef Error<SQLHSTMT, SQL_HANDLE_DESC> DescriptorError;

} // namespace odbc
} // namespace sql
} // namespace fun
