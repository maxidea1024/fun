#pragma once

#include "fun/sql/odbc/odbc.h"
#include "fun/named_tuple.h"
#include "fun/base/dynamic_any.h"
#include "fun/sql/binder_base.h"
#include <vector>
#include <map>
#include <typeinfo>

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqlext.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * Datatypes mapping utility class.
 *
 * This class provides mapping between C and sql datatypes as well
 * as datatypes supported by the underlying database. In order for database
 * types to be available, a valid connection handle must be supplied at either
 * object construction time, or at a later point in time, through call to
 * fillTypeInfo member function.
 *
 * Class also provides a convenient debugging function that prints
 * tabulated data to an output stream.
 */
class FUN_ODBC_API TypeInfo {
 public:
  typedef std::map<int, int> DataTypeMap;
  typedef DataTypeMap::value_type ValueType;
  typedef fun::NamedTuple<String,
    SQLSMALLINT,
    SQLINTEGER,
    String,
    String,
    String,
    SQLSMALLINT,
    SQLSMALLINT,
    SQLSMALLINT,
    SQLSMALLINT,
    SQLSMALLINT,
    SQLSMALLINT,
    String,
    SQLSMALLINT,
    SQLSMALLINT,
    SQLSMALLINT,
    SQLSMALLINT,
    SQLINTEGER,
    SQLSMALLINT> TypeInfoTup;
  typedef std::vector<TypeInfoTup> TypeInfoVec;
  typedef const std::type_info* TypeInfoPtr;

  struct TypeInfoComp : public std::binary_function<TypeInfoPtr, TypeInfoPtr, bool> {
    bool operator()(const TypeInfoPtr& left, const TypeInfoPtr& right) const {
      // apply operator< to operands
      return ( left->before( *right ) );
    }
  };

  typedef std::map<TypeInfoPtr, SQLSMALLINT, TypeInfoComp> CppTypeInfoMap;

  /**
   * Creates the TypeInfo.
   */
  explicit TypeInfo(SQLHDBC* pHDBC=0);

  /**
   * Destroys the TypeInfo.
   */
  ~TypeInfo();

  /**
   * Returns C data type corresponding to supplied sql data type.
   */
  int cDataType(int sqlDataType) const;

  /**
   * Returns sql data type corresponding to supplied C data type.
   */
  int sqlDataType(int cDataType) const;

  /**
   * Fills the data type info structure for the database.
   */
  void fillTypeInfo(SQLHDBC pHDBC);

  /**
   * Returns information about specified data type as specified by parameter 'type'.
   * The requested information is specified by parameter 'param'.
   * Will fail with a fun::NotFoundException thrown if the param is not found
   */
  DynamicAny getInfo(SQLSMALLINT type, const String& param) const;

  /**
   * Returns information about specified data type as specified by parameter 'type' in param result.
   * The requested information is specified by parameter 'param'.
   * Will return false if the param is not found. The value of result will be not changed in this case.
   */
  bool tryGetInfo(SQLSMALLINT type, const String& param, DynamicAny& result) const;

  /**
   * Prints all the types (as reported by the underlying database)
   * to the supplied output stream.
   */
  void print(std::ostream& ostr);

  /**
   * try to find mapping of the given C++ typeid to the ODBC C-Type Code
   * will return the defaultVal if no match is found
   */
  SQLSMALLINT tryTypeidToCType(const std::type_info& ti, SQLSMALLINT defaultVal = SQL_C_TINYINT) const;

  /**
   * Map the null value type to ODBC buffer type
   */
  SQLSMALLINT nullDataType(const NullData val) const;

 private:
  void fillCTypes();
  void fillSQLTypes();

  DataTypeMap c_data_types_;
  DataTypeMap sql_data_types_;
  TypeInfoVec type_info_;
  CppTypeInfoMap cpp_data_types_;
  SQLHDBC*    hdbc_;
};

} // namespace odbc
} // namespace sql
} // namespace fun
