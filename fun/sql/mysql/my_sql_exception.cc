#include <mysql.h>
#include <stdio.h>
#include "fun/sql/mysql/mysql_exception.h"

namespace fun {
namespace sql {
namespace mysql {

MySqlException::MySqlException(const String& msg, int error_code)
    : fun::sql::SqlException(String("[MySQL]: ") + msg, error_code) {}

MySqlException::MySqlException(const MySqlException& e)
    : fun::sql::SqlException(e) {}

MySqlException::~MySqlException() throw() {}

//
// ConnectionException
//

ConnectionException::ConnectionException(const String& msg)
    : MySqlException(msg) {}

ConnectionException::ConnectionException(const String& text, MYSQL* h)
    : MySqlException(Compose(text, h), mysql_errno(h)) {}

String ConnectionException::Compose(const String& text, MYSQL* h) {
  String str;
  str += "[Comment]: ";
  str += text;
  str += "\t[mysql_error]: ";
  str += mysql_error(h);

  str += "\t[mysql_errno]: ";
  char buff[30];
  sprintf(buff, "%d", mysql_errno(h));
  str += buff;

  str += "\t[mysql_sqlstate]: ";
  str += mysql_sqlstate(h);
  return str;
}

//
// TransactionException
//

TransactionException::TransactionException(const String& msg)
    : ConnectionException(msg) {}

TransactionException::TransactionException(const String& text, MYSQL* h)
    : ConnectionException(text, h) {}

//
// StatementException
//

StatementException::StatementException(const String& msg)
    : MySqlException(msg) {}

StatementException::StatementException(const String& text, MYSQL_STMT* h,
                                       const String& stmt)
    : MySqlException(Compose(text, h, stmt), mysql_stmt_errno(h)) {}

String StatementException::Compose(const String& text, MYSQL_STMT* h,
                                   const String& stmt) {
  String str;
  str += "[Comment]: ";
  str += text;

  if (h != 0) {
    str += "\t[mysql_stmt_error]: ";
    str += mysql_stmt_error(h);

    str += "\t[mysql_stmt_errno]: ";
    char buff[30];
    sprintf(buff, "%d", mysql_stmt_errno(h));
    str += buff;

    str += "\t[mysql_stmt_sqlstate]: ";
    str += mysql_stmt_sqlstate(h);
  }

  if (stmt.Len() > 0) {
    str += "\t[statement]: ";
    str += stmt;
  }

  return str;
}

}  // namespace mysql
}  // namespace sql
}  // namespace fun
