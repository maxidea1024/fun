#include "fun/sql/range.h"
#include "fun/sql/sql_exception.h"

namespace fun {
namespace sql {

using namespace Keywords;

Range::Range(Limit::SizeT low_value, Limit::SizeT up_value, bool hard_limit)
    : lower_(lowerLimit(low_value)), upper_(upperLimit(up_value, hard_limit)) {
  if (low_value > up_value) {
    throw LimitException("lowerLimit > upperLimit!");
  }
}

Range::~Range() {}

}  // namespace sql
}  // namespace fun
