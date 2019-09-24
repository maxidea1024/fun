#pragma once

#include "fun/sql/postgresql/postgresql.h"
#include "fun/sql/sql_exception.h"

#include <typeinfo>
#include <string>

namespace fun {
namespace sql {
namespace postgresql {

// End-user include this file and use in code ConnectionException/StatementException
// So it need not know

/**
 * Base class for all PostgreSQL exceptions
 */
class FUN_POSTGRESQL_API PostgreSqlException : public fun::sql::SqlException {
 public:
  /**
   * Creates PostgreSqlException.
   */
  PostgreSqlException(const String& message);

  /**
   * Creates PostgreSqlException.
   */
  PostgreSqlException(const PostgreSqlException& e);

  /**
   * Destroys PostgreSQLexception.
   */
  ~PostgreSqlException() throw();

  /**
   * Assignment operator.
   */
  PostgreSqlException& operator=(const PostgreSqlException& e);

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
class ConnectionException : public PostgreSqlException {
 public:
  /**
   * Creates ConnectionException from string.
   */
  ConnectionException(const String& message);
};

/**
 * TrabsactionException
 */
class TransactionException : public ConnectionException {
 public:
  /**
   * Creates TransactionException from string.
   */
  TransactionException(const String& message);
};

/**
 * StatementException
 */
class StatementException : public PostgreSqlException {
 public:
  /**
   * Creates StatementException from string.
   */
  StatementException(const String& message);
};


//
// inlines
//

inline PostgreSqlException& PostgreSqlException::operator=(const PostgreSqlException& e) {
  fun::sql::SqlException::operator=(e);
  return *this;
}

inline const char* PostgreSqlException::GetName() const throw() {
  return "PostgreSqlException";
}

inline const char* PostgreSqlException::GetClassName() const throw() {
  return typeid(*this).name();
}

inline fun::Exception* PostgreSqlException::Clone() const {
  return new PostgreSqlException(*this);
}

inline void PostgreSqlException::Rethrow() const {
  throw *this;
}

} // namespace postgresql
} // namespace sql
} // namespace fun
