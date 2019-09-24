#include "fun/sql/postgresql/postgresql_types.h"

namespace fun {
namespace sql {
namespace postgresql {

fun::sql::MetaColumn::ColumnDataType OidToColumnDataType(const Oid oid) {
  fun::sql::MetaColumn::ColumnDataType cdt = fun::sql::MetaColumn::FDT_UNKNOWN;

  switch (oid) {
    // bool
    case BOOLOID:
      cdt = fun::sql::MetaColumn::FDT_BOOL;
      break;

    // integers
    case INT2OID:
      cdt = fun::sql::MetaColumn::FDT_INT16;
      break;
    case INT4OID:
      cdt = fun::sql::MetaColumn::FDT_INT32;
      break;
    case INT8OID:
      cdt = fun::sql::MetaColumn::FDT_INT64;
      break;

    // floating point
    case FLOAT8OID:
      cdt = fun::sql::MetaColumn::FDT_DOUBLE;
      break;
    case FLOAT4OID:
      // cdt = fun::sql::MetaColumn::FDT_FLOAT;  This a bug in fun::sql as a 4 byte "float" can't be cast/ugraded to an 8 byte "double"
      cdt = fun::sql::MetaColumn::FDT_DOUBLE;
      break;
    case NUMERICOID:
      cdt = fun::sql::MetaColumn::FDT_DOUBLE;
      break;

    // character strings
    case CHAROID:
      cdt = fun::sql::MetaColumn::FDT_STRING;
      break;
    case BPCHAROID:
      cdt = fun::sql::MetaColumn::FDT_STRING;
      break;
    case VARCHAROID:
      cdt = fun::sql::MetaColumn::FDT_STRING;
      break;

    // BLOB, CLOB
    case BYTEAOID:
      cdt = fun::sql::MetaColumn::FDT_BLOB;
      break;
    case TEXTOID:
      cdt = fun::sql::MetaColumn::FDT_CLOB;
      break;

    // date
    case DATEOID:
      cdt = fun::sql::MetaColumn::FDT_DATE;
      break;

    // time
    case TIMEOID:
      cdt = fun::sql::MetaColumn::FDT_TIME;
      break;
    case TIMETZOID:
      cdt = fun::sql::MetaColumn::FDT_TIME;
      break;

    //timestamp
    case TIMESTAMPOID:
      cdt = fun::sql::MetaColumn::FDT_TIMESTAMP;
      break;
    case TIMESTAMPZOID:
      cdt = fun::sql::MetaColumn::FDT_TIMESTAMP;
      break;

    // everything else is a string
    default:
      cdt = fun::sql::MetaColumn::FDT_STRING;
      break;
  }

  return cdt;
}

} // namespace postgresql
} // namespace sql
} // namespace fun
