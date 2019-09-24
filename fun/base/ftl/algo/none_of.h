#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"  // Invoke

namespace fun {
namespace algo {

template <typename Range>
FUN_ALWAYS_INLINE bool NoneOf(const Range& range) {
  for (const auto& elem : range) {
    if (elem) {
      return false;
    }
  }

  return true;
}

template <typename Range, typename Projection>
FUN_ALWAYS_INLINE bool NoneOf(const Range& range, Projection proj) {
  for (const auto& elem : range) {
    if (Invoke(elem, proj)) {
      return false;
    }
  }

  return true;
}

}  // namespace algo
}  // namespace fun
