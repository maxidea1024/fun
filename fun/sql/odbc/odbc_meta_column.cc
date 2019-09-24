#include "fun/sql/odbc/odbc_meta_column.h"
#include "fun/sql/odbc/utility.h"

namespace fun {
namespace sql {
namespace odbc {

OdbcMetaColumn::OdbcMetaColumn(const StatementHandle& stmt, size_t position)
  : MetaColumn(position), stmt_(stmt) {
  Init();
}

OdbcMetaColumn::~OdbcMetaColumn() {}

void OdbcMetaColumn::GetDescription() {
  UnsafeMemory::Memset(column_desc_.name, 0, NAME_BUFFER_LENGTH);
  column_desc_.nameBufferLength = 0;
  column_desc_.dataType = 0;
  column_desc_.size = 0;
  column_desc_.decimalDigits = 0;
  column_desc_.IsNullable = 0;

  if (Utility::IsError(fun::sql::odbc::SQLDescribeCol(stmt_,
          (SQLUSMALLINT) position() + 1, // ODBC columns are 1-based
          column_desc_.name,
          NAME_BUFFER_LENGTH,
          &column_desc_.nameBufferLength,
          &column_desc_.dataType,
          &column_desc_.size,
          &column_desc_.decimalDigits,
          &column_desc_.IsNullable))) {
    throw StatementException(stmt_);
  }
}

bool OdbcMetaColumn::IsUnsigned() const {
  SQLLEN val = 0;
  if (Utility::IsError(fun::sql::odbc::SQLColAttribute(stmt_,
    (SQLUSMALLINT)position() + 1, // ODBC columns are 1-based
    SQL_DESC_UNSIGNED,
    0,
    0,
    0,
    & val))) {
    throw StatementException(stmt_);
  }

  return (val == SQL_TRUE);
}

void OdbcMetaColumn::Init() {
  GetDescription();

  if (Utility::IsError(fun::sql::odbc::SQLColAttribute(stmt_,
      (SQLUSMALLINT) position() + 1, // ODBC columns are 1-based
      SQL_DESC_LENGTH,
      0,
      0,
      0,
      &data_length_))) {
    throw StatementException(stmt_);
  }

  SetName(String((char*) column_desc_.name));
  SetLength(column_desc_.size);
  SetPrecision(column_desc_.decimalDigits);
  SetNullable(SQL_NULLABLE == column_desc_.IsNullable);
  switch(column_desc_.dataType) {
    case SQL_BIT:
      SetType(MetaColumn::FDT_BOOL); break;

    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
#ifdef SQL_GUID
    case SQL_GUID:
#endif
      SetType(MetaColumn::FDT_STRING); break;

    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
    case -350:  // IBM DB2 CLOB, which long unicode string
      SetType(MetaColumn::FDT_WSTRING); break;

    case SQL_TINYINT:
      SetType(IsUnsigned() ? MetaColumn::FDT_UINT8 : MetaColumn::FDT_INT8);
      break;

    case SQL_SMALLINT:
      SetType(IsUnsigned() ? MetaColumn::FDT_UINT16 : MetaColumn::FDT_INT16);
      break;

    case SQL_INTEGER:
      SetType(IsUnsigned() ? MetaColumn::FDT_UINT32 : MetaColumn::FDT_INT32);
      break;

    case SQL_BIGINT:
      SetType(IsUnsigned() ? MetaColumn::FDT_UINT64 : MetaColumn::FDT_INT64);
      break;

    case SQL_DOUBLE:
    case SQL_FLOAT:
      SetType(MetaColumn::FDT_DOUBLE); break;

    case SQL_NUMERIC:
    case SQL_DECIMAL:
    case -360: // IBM DB2 DecFloat
      // Oracle has no INTEGER type - it's essentially NUMBER with 38 whole and
      // 0 fractional digits. It also does not recognize SQL_BIGINT type,
      // so the workaround here is to hardcode it to 32 bit integer
      if (0 == column_desc_.decimalDigits) {
        SetType(MetaColumn::FDT_INT32);
      } else {
        SetType(MetaColumn::FDT_DOUBLE);
      }
      break;

    case SQL_REAL:
      SetType(MetaColumn::FDT_FLOAT); break;

    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
    case -98:// IBM DB2 non-standard type
    case -370: // IBM DB2 XML, documentation advises to bind it as BLOB, not CLOB
      SetType(MetaColumn::FDT_BLOB); break;

    case -99: // IBM DB2 CLOB
      SetType(MetaColumn::FDT_CLOB); break;

    case SQL_TYPE_DATE:
      SetType(MetaColumn::FDT_DATE); break;

    case SQL_TYPE_TIME:
      SetType(MetaColumn::FDT_TIME); break;

    case SQL_TYPE_TIMESTAMP:
      SetType(MetaColumn::FDT_TIMESTAMP); break;

    default:
      throw DataFormatException("Unsupported data type.");
  }
}

} // namespace odbc
} // namespace sql
} // namespace fun
