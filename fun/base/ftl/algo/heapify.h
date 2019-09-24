#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/algo/impl/binary_heap.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/ftl/template.h"

namespace fun {
namespace algo {

template <typename Range>
FUN_ALWAYS_INLINE void Heapify(Range& range) {
  impl::HeapifyInternal(GetMutableData(range), GetCount(range),
                        IdentityFunctor(), Less<>());
}

template <typename Range, typename Predicate>
FUN_ALWAYS_INLINE void Heapify(Range& range, Predicate pred) {
  impl::HeapifyInternal(GetMutableData(range), GetCount(range),
                        IdentityFunctor(), pred);
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void HeapifyBy(Range& range, Projection proj) {
  impl::HeapifyInternal(GetMutableData(range), GetCount(range), proj, Less<>());
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void HeapifyBy(Range& range, Projection proj,
                                 Predicate pred) {
  impl::HeapifyInternal(GetMutableData(range), GetCount(range), proj, pred);
}

}  // namespace algo
}  // namespace fun
