#pragma once

#include "fun/sql/odbc/handle.h"
#include "fun/sql/odbc/odbc.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqlext.h>

namespace fun {
namespace sql {
namespace odbc {

class FUN_ODBC_API Parameter {
 public:
  /**
   * Creates the Parameter.
   */
  explicit Parameter(const StatementHandle& stmt, size_t col_num);

  /**
   * Destroys the Parameter.
   */
  ~Parameter();

  /**
   * Returns the column number.
   */
  size_t number() const;

  /**
   * Returns the sql data type.
   */
  size_t dataType() const;

  /**
   * Returns the the size of the column or expression of the corresponding
   * parameter marker as defined by the data source.
   */
  size_t columnSize() const;

  /**
   * Returns the number of decimal digits of the column or expression
   * of the corresponding parameter as defined by the data source.
   */
  size_t decimalDigits() const;

  /**
   * Returns true if column allows null values, false otherwise.
   */
  bool IsNullable() const;

 private:
  Parameter();

  void Init();

  SQLSMALLINT data_type_;
  SQLULEN column_size_;
  SQLSMALLINT decimal_digits_;
  SQLSMALLINT is_nullable_;

  const StatementHandle& stmt_;
  size_t number_;
};

//
// inlines
//

inline size_t Parameter::number() const { return number_; }

inline size_t Parameter::dataType() const { return data_type_; }

inline size_t Parameter::columnSize() const { return column_size_; }

inline size_t Parameter::decimalDigits() const { return decimal_digits_; }

inline bool Parameter::IsNullable() const {
  return SQL_NULLABLE == is_nullable_;
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
