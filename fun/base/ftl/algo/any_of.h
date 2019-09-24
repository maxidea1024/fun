#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/algo/none_of.h"
#include "fun/base/ftl/template.h" // MoveTemp

namespace fun {
namespace algo {

template <typename Range>
FUN_ALWAYS_INLINE bool AnyOf(const Range& range) {
  return !NoneOf(range);
}

template <typename Range, typename Projection>
FUN_ALWAYS_INLINE bool AnyOf(const Range& range, Projection proj) {
  return !NoneOf(range, MoveTemp(proj));
}

} // namespace algo
} // namespace fun
