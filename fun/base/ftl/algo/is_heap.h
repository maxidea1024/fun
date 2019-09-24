#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/ftl/template.h"

namespace fun {
namespace algo {

namespace impl {

template <typename RangeValue, typename Projection, typename Predicate>
bool IsHeapInternal(RangeValue* heap, size_t count, Projection proj,
                    Predicate pred) {
  for (size_t i = 1; i < count; ++i) {
    int32 parent_index = HeapGetParentIndex((int32)i);
    if (pred(Invoke(proj, heap[i]), Invoke(proj, heap[parent_index]))) {
      return false;
    }
  }

  return true;
}

}  // namespace impl

template <typename Range>
FUN_ALWAYS_INLINE bool IsHeap(Range& range) {
  return impl::IsHeapInternal(GetMutableData(range), GetCount(range),
                              IdentityFunctor(), Less<>());
}

template <typename Range, typename Projection>
FUN_ALWAYS_INLINE bool IsHeapBy(Range& range, Projection proj) {
  return impl::IsHeapInternal(GetMutableData(range), GetCount(range),
                              MoveTemp(proj), Less<>());
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE bool IsHeapBy(Range& range, Projection proj, Predicate pred) {
  return impl::IsHeapInternal(GetMutableData(range), GetCount(range),
                              MoveTemp(proj), MoveTemp(pred));
}

}  // namespace algo
}  // namespace fun
