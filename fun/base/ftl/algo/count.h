#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h" // Invoke

namespace fun {
namespace algo {

//전재조건:
// - collection은 ranged-for를 지원해야함.
// - element는 equal연산자를 지원해야함.

template <typename Collection, typename Element>
FUN_ALWAYS_INLINE size_t Count(const Collection& collection, const Element& value) {
  size_t count = 0;
  for (const auto& elem : collection) {
    if (elem == value) {
      ++count;
    }
  }

  return count;
}

template <typename Collection, typename Predicate>
FUN_ALWAYS_INLINE size_t CountIf(const Collection& collection, Predicate pred) {
  size_t count = 0;
  for (const auto& elem : collection) {
    if (Invoke(pred, elem)) {
      ++count;
    }
  }

  return count;
}

} // namespace algo
} // namespace fun
