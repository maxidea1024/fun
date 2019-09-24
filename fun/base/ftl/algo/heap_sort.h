#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/algo/impl/binary_heap.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/functional.h"

namespace fun {
namespace algo {

template <typename Range>
FUN_ALWAYS_INLINE void HeapSort(Range& range) {
  impl::HeapSortInternal(GetMutableData(range), GetCount(range), IdentityFunctor(), Less<>());
}

template <typename Range, typename Predicate>
FUN_ALWAYS_INLINE void HeapSort(Range& range, Predicate pred) {
  impl::HeapSortInternal(GetMutableData(range), GetCount(range), IdentityFunctor(), pred);
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void HeapSortBy(Range& range, Projection proj) {
  impl::HeapSortInternal(GetMutableData(range), GetCount(range), proj, Less<>());
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void HeapSortBy(Range& range, Projection proj, Predicate pred) {
  impl::HeapSortInternal(GetMutableData(range), GetCount(range), MoveTemp(proj), MoveTemp(pred));
}

} // namespace algo
} // namespace fun
