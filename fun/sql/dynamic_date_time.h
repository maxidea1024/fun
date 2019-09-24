#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/date.h"
#include "fun/sql/time.h"
#include "fun/base/dynamic/var.h"

namespace fun {
namespace sql {

class Date;
class Time;

} // namespace sql
} // namespace fun

namespace fun {
namespace Dynamic {

template <> FUN_SQL_API Var::operator fun::sql::Date() const;
template <> FUN_SQL_API Var::operator fun::sql::Time() const;

} // namespace Dynamic
} // namespace fun
