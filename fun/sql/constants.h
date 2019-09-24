#pragma once

#undef max
#include <cstddef>
#include <limits>

namespace fun {
namespace sql {

static const size_t FUN_DATA_INVALID_ROW = std::numeric_limits<size_t>::max();

}  // namespace sql
}  // namespace fun
