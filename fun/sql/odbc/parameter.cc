#include "fun/sql/odbc/parameter.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/odbc/error.h"
#include "fun/sql/odbc/odbc_exception.h"

namespace fun {
namespace sql {
namespace odbc {

Parameter::Parameter(const StatementHandle& stmt, size_t col_num)
  : stmt_(stmt),
    number_(col_num) {
  Init();
}

Parameter::~Parameter() {}

void Parameter::Init() {
  if (Utility::IsError(SQLDescribeParam(stmt_,
                                        (SQLUSMALLINT) number_ + 1,
                                        &data_type_,
                                        &column_size_,
                                        &decimal_digits_,
                                        &is_nullable_))) {
    throw StatementException(stmt_);
  }
}

} // namespace odbc
} // namespace sql
} // namespace fun
