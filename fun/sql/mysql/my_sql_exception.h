#pragma once

#include <string>
#include <typeinfo>
#include "fun/sql/mysql/mysql.h"
#include "fun/sql/sql_exception.h"

typedef struct st_mysql MYSQL;
typedef struct st_mysql_stmt MYSQL_STMT;

namespace fun {
namespace sql {
namespace mysql {

// End-user include this file and use in code
// ConnectionException/StatementException So it need not know

/**
 * Base class for all MySQL exceptions
 */
class FUN_MYSQL_API MySqlException : public fun::sql::SqlException {
 public:
  /**
   * Creates MySqlException.
   */
  MySqlException(const String& msg, int error_code = 0);

  /**
   * Creates MySqlException.
   */
  MySqlException(const MySqlException& e);

  /**
   * Destroys MySQLexception.
   */
  ~MySqlException() throw();

  /**
   * Assignment operator.
   */
  MySqlException& operator=(const MySqlException& e);

  /**
   * Returns exception name.
   */
  const char* GetName() const throw();

  /**
   * Returns the name of the exception class.
   */
  const char* GetClassName() const throw();

  /**
   * Creates an exact copy of the exception.
   *
   * The copy can later be thrown again by
   * invoking Rethrow() on it.
   */
  fun::Exception* Clone() const;

  /**
   * (Re)Throws the exception.
   *
   * This is useful for temporarily storing a
   * copy of an exception (see Clone()), then
   * throwing it again.
   */
  void Rethrow() const;
};

/**
 * ConnectionException
 */
class ConnectionException : public MySqlException {
 public:
  /**
   * Creates ConnectionException from string.
   */
  ConnectionException(const String& msg);

  /**
   * Creates ConnectionException from string and handle.
   */
  ConnectionException(const String& text, MYSQL* h);

 private:
  static String Compose(const String& text, MYSQL* h);
};

/**
 * TransactionException
 */
class TransactionException : public ConnectionException {
 public:
  /**
   * Creates TransactionException from string.
   */
  TransactionException(const String& msg);

  /**
   * Creates TransactionException from string and handle.
   */
  TransactionException(const String& text, MYSQL* h);
};

/**
 * StatementException
 */
class StatementException : public MySqlException {
 public:
  /**
   * Creates StatementException from string.
   */
  StatementException(const String& msg);

  /**
   * Creates StatementException from string and handle.
   */
  StatementException(const String& text, MYSQL_STMT* h,
                     const String& stmt = "");

 private:
  static String Compose(const String& text, MYSQL_STMT* h, const String& stmt);
};

//
// inlines
//

inline MySqlException& MySqlException::operator=(const MySqlException& e) {
  fun::sql::SqlException::operator=(e);
  return *this;
}

inline const char* MySqlException::GetName() const throw() { return "MySQL"; }

inline const char* MySqlException::GetClassName() const throw() {
  return typeid(*this).name();
}

inline fun::Exception* MySqlException::Clone() const {
  return new MySqlException(*this);
}

inline void MySqlException::Rethrow() const { throw *this; }

}  // namespace mysql
}  // namespace sql
}  // namespace fun
