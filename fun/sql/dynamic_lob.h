#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/lob.h"
#include "fun/base/dynamic/var.h"

namespace fun {
namespace sql {

template <typename T> class LOB;
typedef LOB<unsigned char> BLOB;
typedef LOB<char> CLOB;

} // namespace sql
} // namespace fun


namespace fun {
namespace Dynamic {

template <> FUN_SQL_API Var::operator fun::sql::CLOB() const;
template <> FUN_SQL_API Var::operator fun::sql::BLOB() const;

} // namespace Dynamic
} // namespace fun
