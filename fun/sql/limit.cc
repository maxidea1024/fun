#include "fun/sql/limit.h"

namespace fun {
namespace sql {

Limit::Limit(SizeT limit_value, bool hard_limit, bool is_lower_limit)
    : value_(limit_value),
      hard_limit_(hard_limit),
      is_lower_limit_(is_lower_limit) {}

Limit::~Limit() {}

}  // namespace sql
}  // namespace fun
