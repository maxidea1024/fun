#pragma once

#include "fun/sql/odbc/odbc.h"
#include "fun/sql/odbc/error.h"
#include "fun/sql/odbc/handle.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/meta_column.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqlext.h>

namespace fun {
namespace sql {
namespace odbc {

class FUN_ODBC_API OdbcMetaColumn : public MetaColumn {
 public:
  /**
   * Creates the OdbcMetaColumn.
   */
  OdbcMetaColumn(const StatementHandle& stmt, size_t position);

  /**
   * Destroys the OdbcMetaColumn.
   */
  ~OdbcMetaColumn();

  /**
   * A numeric value that is either the maximum or actual character length of a character
   * string or binary data type. It is the maximum character length for a fixed-length data type,
   * or the actual character length for a variable-length data type. Its value always excludes the
   * null-termination byte that ends the character string.
   * This information is returned from the SQL_DESC_LENGTH record field of the IRD.
   */
  size_t GetDataLength() const;

  /**
   * Returns true if column is unsigned or a non-numeric data type.
   */
  bool IsUnsigned() const;

 private:
  OdbcMetaColumn();

  static const int NAME_BUFFER_LENGTH = 2048;

  struct ColumnDescription {
    SQLCHAR name[NAME_BUFFER_LENGTH];
    SQLSMALLINT nameBufferLength;
    SQLSMALLINT dataType;
    SQLULEN size;
    SQLSMALLINT decimalDigits;
    SQLSMALLINT IsNullable;
  };

  void Init();
  void GetDescription();

  SQLLEN data_length_;
  const StatementHandle& stmt_;
  ColumnDescription column_desc_;
};


//
// inlines
//

inline size_t OdbcMetaColumn::GetDataLength() const {
  return data_length_;
}

} // namespace odbc
} // namespace sql
} // namespace fun
