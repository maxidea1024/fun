#pragma once

#include "fun/base/format.h"
#include "fun/sql/odbc/diagnostics.h"
#include "fun/sql/odbc/error.h"
#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/sql_exception.h"

namespace fun {
namespace sql {
namespace odbc {

FUN_DECLARE_EXCEPTION(FUN_ODBC_API, OdbcException, fun::sql::SqlException)
FUN_DECLARE_EXCEPTION(FUN_ODBC_API, InsufficientStorageException, OdbcException)
FUN_DECLARE_EXCEPTION(FUN_ODBC_API, UnknownDataLengthException, OdbcException)
FUN_DECLARE_EXCEPTION(FUN_ODBC_API, DataTruncatedException, OdbcException)

template <typename H, SQLSMALLINT handleType>
class HandleException : public OdbcException {
 public:
  /**
   * Creates HandleException
   */
  HandleException(const H& handle, int code = 0)
      : OdbcException(code), error_(handle) {
    message(error_.ToString());
  }

  /**
   * Creates HandleException
   */
  HandleException(const H& handle, const String& msg)
      : OdbcException(msg), error_(handle) {
    extendedMessage(error_.ToString());
  }

  /**
   * Creates HandleException
   */
  HandleException(const H& handle, const String& msg, const String& arg)
      : OdbcException(msg, arg), error_(handle) {}

  /**
   * Creates HandleException
   */
  HandleException(const H& handle, const String& msg, const fun::Exception& e)
      : OdbcException(msg, e), error_(handle) {}

  /**
   * Creates HandleException
   */
  HandleException(const HandleException& e)
      : OdbcException(e), error_(e.error_) {}

  /**
   * Destroys HandleException
   */
  ~HandleException() throw() {}

  /**
   * Assignment operator
   */
  HandleException& operator=(const HandleException& e) {
    if (&e != this) {
      error_ = e.error_;
    }

    return *this;
  }

  /**
   * Returns the name of the exception
   */
  const char* GetName() const throw() { return "ODBC handle exception"; }

  /**
   * Returns the HandleException class name.
   */
  const char* GetClassName() const throw() { return typeid(*this).name(); }

  /**
   * Clones the HandleException
   */
  fun::Exception* Clone() const { return new HandleException(*this); }

  /**
   * Re-throws the HandleException.
   */
  void Rethrow() const { throw *this; }

  /**
   * Returns error diagnostics.
   */
  const Diagnostics<H, handleType>& diagnostics() const {
    return error_.diagnostics();
  }

  /**
   * Returns the formatted error diagnostics for the handle.
   */
  String ToString() const {
    return fun::Format("ODBC Error: %s\n===================\n%s\n",
                       String(what()), error_.ToString());
  }

  /**
   * Returns the error diagnostics string
   */
  String errorString() const { return error_.ToString(); }

  /**
   * Returns the error diagnostics string for the handle.
   */
  static String errorString(const H& handle) {
    return Error<H, handleType>(handle).ToString();
  }

 protected:
  const Error<H, handleType>& error() const { return error_; }

 private:
  Error<H, handleType> error_;
};

typedef HandleException<SQLHENV, SQL_HANDLE_ENV> EnvironmentException;
typedef HandleException<SQLHDBC, SQL_HANDLE_DBC> ConnectionException;
typedef HandleException<SQLHSTMT, SQL_HANDLE_STMT> StatementException;
typedef HandleException<SQLHDESC, SQL_HANDLE_DESC> DescriptorException;

}  // namespace odbc
}  // namespace sql
}  // namespace fun
