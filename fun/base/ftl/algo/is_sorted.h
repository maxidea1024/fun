#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/ftl/template.h"

/*
정렬되어 있는지 여부를 알아본다.

기본적으로 오름차순인지 여부를 체크하도록 되어 있다. 만약 내림차순으로 정렬된 상태를 확인하고 싶다면,
Predicate에서 Less<T> 대신 Greater<T>를 지정하면 된다.

Predicate를 지정하지 않으면 Less<T>가 사용된다. 즉, 위에서 얘기한것처럼 오름차순인지를 확인하는 것으로
간주한다.
*/

namespace fun {
namespace algo {

namespace impl {

template <typename T, typename Projection, typename Predicate>
bool IsSortedByInternal(const T* range, size_t range_count, Projection proj, Predicate pred) {
  if (range_count == 0) {
    return true;
  }

  // When comparing N elements, we do N-1 comparisons
  --range_count;

  const T* next = range + 1;
  for (;;) {
    if (range_count == 0) {
      return true;
    }

    auto& ref1 = Invoke(proj, *next);
    auto& ref2 = Invoke(proj, *range);

    if (Invoke(pred, ref1, ref2)) {
      return false;
    }

    ++range;
    ++next;
    --range_count;
  }
}

} // namespace impl

template <typename Range>
FUN_ALWAYS_INLINE bool IsSorted(const Range& range) {
  return impl::IsSortedByInternal(GetConstData(range), GetCount(range), IdentityFunctor(), Less<>());
}

template <typename Range, typename Predicate>
FUN_ALWAYS_INLINE bool IsSorted(const Range& range, Predicate pred) {
  return impl::IsSortedByInternal(GetConstData(range), GetCount(range), IdentityFunctor(), MoveTemp(pred));
}

template <typename Range, typename Projection>
FUN_ALWAYS_INLINE bool IsSortedBy(const Range& range, Projection proj) {
  return impl::IsSortedByInternal(GetConstData(range), GetCount(range), MoveTemp(proj), Less<>());
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE bool IsSortedBy(const Range& range, Projection proj, Predicate pred) {
  return impl::IsSortedByInternal(GetConstData(range), GetCount(range), MoveTemp(proj), MoveTemp(pred));
}

} // namespace algo
} // namespace fun
