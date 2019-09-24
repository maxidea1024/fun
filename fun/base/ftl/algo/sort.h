#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/algo/intro_sort.h"

namespace fun {
namespace algo {

template <typename Range>
FUN_ALWAYS_INLINE void Sort(Range& range) {
  IntroSort(range);
}

template <typename Range, typename Predicate>
FUN_ALWAYS_INLINE void Sort(Range& range, Predicate pred) {
  IntroSort(range, MoveTemp(pred));
}

template <typename Range, typename Projection>
FUN_ALWAYS_INLINE void SortBy(Range& range, Projection proj) {
  IntroSortBy(range, MoveTemp(proj));
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void SortBy(Range& range, Projection proj, Predicate pred) {
  IntroSortBy(range, MoveTemp(proj), MoveTemp(pred));
}

}  // namespace algo
}  // namespace fun
