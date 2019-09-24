#include "fun/sql/odbc/type_info.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/lob.h"
#include "fun/base/format.h"
#include "fun/base/exception.h"
#include <iostream>

namespace fun {
namespace sql {
namespace odbc {

TypeInfo::TypeInfo(SQLHDBC* pHDBC): hdbc_(pHDBC) {
  fillCTypes();
  fillSQLTypes();
  if (hdbc_) fillTypeInfo(*hdbc_);

  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(String), static_cast<SQLSMALLINT>(SQL_C_CHAR)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(std::wstring), static_cast<SQLSMALLINT>(SQL_C_WCHAR)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(fun::UString), static_cast<SQLSMALLINT>(SQL_C_WCHAR)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(Date), static_cast<SQLSMALLINT>(SQL_TYPE_DATE)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(Time), static_cast<SQLSMALLINT>(SQL_TYPE_TIME)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(DateTime), static_cast<SQLSMALLINT>(SQL_TYPE_TIMESTAMP)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(BLOB), static_cast<SQLSMALLINT>(SQL_BINARY)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(float), static_cast<SQLSMALLINT>(SQL_REAL)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(double), static_cast<SQLSMALLINT>(SQL_DOUBLE)));
  cpp_data_types_.insert(CppTypeInfoMap::value_type(&typeid(bool), static_cast<SQLSMALLINT>(SQL_BIT)));
}

TypeInfo::~TypeInfo() {}

void TypeInfo::fillCTypes() {
  c_data_types_.insert(ValueType(SQL_CHAR, SQL_C_CHAR));
  c_data_types_.insert(ValueType(SQL_VARCHAR, SQL_C_CHAR));
  c_data_types_.insert(ValueType(SQL_LONGVARCHAR, SQL_C_CHAR));
  c_data_types_.insert(ValueType(SQL_DECIMAL, SQL_C_DOUBLE));
  c_data_types_.insert(ValueType(SQL_NUMERIC, SQL_C_DOUBLE));
  c_data_types_.insert(ValueType(SQL_BIT, SQL_C_BIT));
  c_data_types_.insert(ValueType(SQL_TINYINT, SQL_C_STINYINT));
  c_data_types_.insert(ValueType(SQL_SMALLINT, SQL_C_SSHORT));
  c_data_types_.insert(ValueType(SQL_INTEGER, SQL_C_SLONG));
  c_data_types_.insert(ValueType(SQL_BIGINT, SQL_C_SBIGINT));
  c_data_types_.insert(ValueType(SQL_REAL, SQL_C_FLOAT));
  c_data_types_.insert(ValueType(SQL_FLOAT, SQL_C_DOUBLE));
  c_data_types_.insert(ValueType(SQL_DOUBLE, SQL_C_DOUBLE));
  c_data_types_.insert(ValueType(SQL_BINARY, SQL_C_BINARY));
  c_data_types_.insert(ValueType(SQL_VARBINARY, SQL_C_BINARY));
  c_data_types_.insert(ValueType(SQL_LONGVARBINARY, SQL_C_BINARY));
  c_data_types_.insert(ValueType(SQL_TYPE_DATE, SQL_C_TYPE_DATE));
  c_data_types_.insert(ValueType(SQL_TYPE_TIME, SQL_C_TYPE_TIME));
  c_data_types_.insert(ValueType(SQL_TYPE_TIMESTAMP, SQL_C_TYPE_TIMESTAMP));
}

void TypeInfo::fillSQLTypes() {
  sql_data_types_.insert(ValueType(SQL_C_CHAR, SQL_LONGVARCHAR));
  sql_data_types_.insert(ValueType(SQL_C_BIT, SQL_BIT));
  sql_data_types_.insert(ValueType(SQL_C_TINYINT, SQL_TINYINT));
  sql_data_types_.insert(ValueType(SQL_C_STINYINT, SQL_TINYINT));
  sql_data_types_.insert(ValueType(SQL_C_UTINYINT, SQL_TINYINT));
  sql_data_types_.insert(ValueType(SQL_C_SHORT, SQL_SMALLINT));
  sql_data_types_.insert(ValueType(SQL_C_SSHORT, SQL_SMALLINT));
  sql_data_types_.insert(ValueType(SQL_C_USHORT, SQL_SMALLINT));
  sql_data_types_.insert(ValueType(SQL_C_LONG, SQL_INTEGER));
  sql_data_types_.insert(ValueType(SQL_C_SLONG, SQL_INTEGER));
  sql_data_types_.insert(ValueType(SQL_C_ULONG, SQL_INTEGER));
  sql_data_types_.insert(ValueType(SQL_C_SBIGINT, SQL_BIGINT));
  sql_data_types_.insert(ValueType(SQL_C_UBIGINT, SQL_BIGINT));
  sql_data_types_.insert(ValueType(SQL_C_FLOAT, SQL_REAL));
  sql_data_types_.insert(ValueType(SQL_C_DOUBLE, SQL_DOUBLE));
  sql_data_types_.insert(ValueType(SQL_C_BINARY, SQL_LONGVARBINARY));
  sql_data_types_.insert(ValueType(SQL_C_TYPE_DATE, SQL_TYPE_DATE));
  sql_data_types_.insert(ValueType(SQL_C_TYPE_TIME, SQL_TYPE_TIME));
  sql_data_types_.insert(ValueType(SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP));
}

void TypeInfo::fillTypeInfo(SQLHDBC pHDBC) {
  hdbc_ = &pHDBC;

  if (type_info_.IsEmpty() && hdbc_) {
    const static int stringSize = 512;
    TypeInfoVec().Swap(type_info_);

    SQLRETURN rc;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;

    rc = SQLAllocHandle(SQL_HANDLE_STMT, *hdbc_, &hstmt);
    if (!SQL_SUCCEEDED(rc)) {
      throw StatementException(hstmt, "SQLGetData()");
    }

    rc = fun::sql::odbc::SQLGetTypeInfo(hstmt, SQL_ALL_TYPES);
    if (SQL_SUCCEEDED(rc)) {
      while (SQLFetch(hstmt) != SQL_NO_DATA_FOUND) {
        char typeName[stringSize] = { 0 };
        char literalPrefix[stringSize] = { 0 };
        char literalSuffix[stringSize] = { 0 };
        char createParams[stringSize] = { 0 };
        char localTypeName[stringSize] = { 0 };

        TypeInfoTup ti("TYPE_NAME", "",
          "DATA_TYPE", 0,
          "COLUMN_SIZE", 0,
          "LITERAL_PREFIX", "",
          "LITERAL_SUFFIX", "",
          "CREATE_PARAMS", "",
          "NULLABLE", 0,
          "CASE_SENSITIVE", 0,
          "SEARCHABLE", 0,
          "UNSIGNED_ATTRIBUTE", 0,
          "FIXED_PREC_SCALE", 0,
          "AUTO_UNIQUE_VALUE", 0,
          "LOCAL_TYPE_NAME", "",
          "MINIMUM_SCALE", 0,
          "MAXIMUM_SCALE", 0,
          "SQL_DATA_TYPE", 0,
          "SQL_DATETIME_SUB", 0,
          "NUM_PREC_RADIX", 0,
          "INTERVAL_PRECISION", 0);

        SQLLEN ind = 0;
        rc = SQLGetData(hstmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind);
        ti.set<0>(typeName);
        rc = SQLGetData(hstmt, 2, SQL_C_SSHORT, &ti.Get<1>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 3, SQL_C_SLONG, &ti.Get<2>(), sizeof(SQLINTEGER), &ind);
        rc = SQLGetData(hstmt, 4, SQL_C_CHAR, literalPrefix, sizeof(literalPrefix), &ind);
        ti.set<3>(literalPrefix);
        rc = SQLGetData(hstmt, 5, SQL_C_CHAR, literalSuffix, sizeof(literalSuffix), &ind);
        ti.set<4>(literalSuffix);
        rc = SQLGetData(hstmt, 6, SQL_C_CHAR, createParams, sizeof(createParams), &ind);
        ti.set<5>(createParams);
        rc = SQLGetData(hstmt, 7, SQL_C_SSHORT, &ti.Get<6>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 8, SQL_C_SSHORT, &ti.Get<7>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 9, SQL_C_SSHORT, &ti.Get<8>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 10, SQL_C_SSHORT, &ti.Get<9>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 11, SQL_C_SSHORT, &ti.Get<10>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 12, SQL_C_SSHORT, &ti.Get<11>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 13, SQL_C_CHAR, localTypeName, sizeof(localTypeName), &ind);
        ti.set<12>(localTypeName);
        rc = SQLGetData(hstmt, 14, SQL_C_SSHORT, &ti.Get<13>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 15, SQL_C_SSHORT, &ti.Get<14>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 16, SQL_C_SSHORT, &ti.Get<15>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 17, SQL_C_SSHORT, &ti.Get<16>(), sizeof(SQLSMALLINT), &ind);
        rc = SQLGetData(hstmt, 18, SQL_C_SLONG, &ti.Get<17>(), sizeof(SQLINTEGER), &ind);
        rc = SQLGetData(hstmt, 19, SQL_C_SSHORT, &ti.Get<18>(), sizeof(SQLSMALLINT), &ind);

        type_info_.push_back(ti);
      }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
  }
}

DynamicAny TypeInfo::getInfo(SQLSMALLINT type, const String& param) const {
  TypeInfoVec::const_iterator it = type_info_.begin();
  TypeInfoVec::const_iterator end = type_info_.end();
  for (; it != end; ++it) {
    if (type == it->Get<1>()) {
      return (*it)[param];
    }
  }

  throw NotFoundException(param);
}

bool TypeInfo::tryGetInfo(SQLSMALLINT type, const String& param, DynamicAny& result) const {
  TypeInfoVec::const_iterator it = type_info_.begin();
  TypeInfoVec::const_iterator end = type_info_.end();
  for (; it != end; ++it) {
    if (type == it->Get<1>()) {
      result = (*it)[param];
      return true;
    }
  }

  return false;
}

int TypeInfo::cDataType(int sqlDataType) const {
  DataTypeMap::const_iterator it = c_data_types_.find(sqlDataType);

  if (c_data_types_.end() == it) {
    throw NotFoundException(format("C data type not found for sql data type: %d", sqlDataType));
  }

  return it->second;
}

int TypeInfo::sqlDataType(int cDataType) const {
  DataTypeMap::const_iterator it = sql_data_types_.find(cDataType);

  if (sql_data_types_.end() == it) {
    throw NotFoundException(format("sql data type not found for C data type: %d", cDataType));
  }

  return it->second;
}

void TypeInfo::print(std::ostream& ostr) {
  if (type_info_.IsEmpty()) {
    ostr << "No data found.";
    return;
  }

  TypeInfoTup::NameVec::const_iterator nIt = (*type_info_[0].names()).begin();
  TypeInfoTup::NameVec::const_iterator nItEnd = (*type_info_[0].names()).end();
  for (; nIt != nItEnd; ++nIt) {
    ostr << *nIt << "\t";
  }

  ostr << std::endl;

  TypeInfoVec::const_iterator it = type_info_.begin();
  TypeInfoVec::const_iterator end = type_info_.end();

  for (; it != end; ++it) {
    ostr << it->Get<0>() << "\t"
      << it->Get<1>() << "\t"
      << it->Get<2>() << "\t"
      << it->Get<3>() << "\t"
      << it->Get<4>() << "\t"
      << it->Get<5>() << "\t"
      << it->Get<6>() << "\t"
      << it->Get<7>() << "\t"
      << it->Get<8>() << "\t"
      << it->Get<9>() << "\t"
      << it->Get<10>() << "\t"
      << it->Get<11>() << "\t"
      << it->Get<12>() << "\t"
      << it->Get<13>() << "\t"
      << it->Get<14>() << "\t"
      << it->Get<15>() << "\t"
      << it->Get<16>() << "\t"
      << it->Get<17>() << "\t"
      << it->Get<18>() << std::endl;
  }
}

SQLSMALLINT TypeInfo::tryTypeidToCType(const std::type_info& ti, SQLSMALLINT defaultVal) const {
  CppTypeInfoMap::const_iterator res = cpp_data_types_.find(&ti);
  if (res == cpp_data_types_.end()) {
    return defaultVal;
  }

  return res->second;
}

SQLSMALLINT TypeInfo::nullDataType(const NullData val) const {
  switch (val) {
    case NULL_GENERIC:
    case DATA_NULL_INTEGER:
      return SQL_C_TINYINT;

    case DATA_NULL_STRING:
      return SQL_C_CHAR;

    case DATA_NULL_DATE:
      return SQL_C_TYPE_DATE;

    case DATA_NULL_TIME:
      return SQL_C_TYPE_TIME;

    case DATA_NULL_DATETIME:
      return SQL_C_TYPE_TIMESTAMP;

    case DATA_NULL_BLOB:
      return SQL_C_BINARY;

    case DATA_NULL_FLOAT:
      return SQL_C_FLOAT;
  }

  return SQL_C_TINYINT;
}

} // namespace odbc
} // namespace sql
} // namespace fun
