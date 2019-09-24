#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/ftl/algo/impl/range_pointer_type.h"

namespace fun {
namespace algo {

namespace impl {

template <typename Range, typename Value, typename Projection>
typename RangePointerType<Range>::Type
FindBy(Range& range, const Value& value, Projection proj) {
  for (auto& elem : range) {
    if (Invoke(proj, elem) == value) {
      return &elem;
    }
  }

  return nullptr;
}

template <typename Range, typename Predicate>
typename RangePointerType<Range>::Type
FindIf(Range& range, Predicate pred) {
  for (auto& elem : range) {
    if (Invoke(pred, elem)) {
      return &elem;
    }
  }

  return nullptr;
}

} // namespace impl

template <typename Range, typename Value>
FUN_ALWAYS_INLINE decltype(auto) Find(Range& range, const Value& value) {
  return impl::FindBy(range, value, IdentityFunctor());
}

template <typename Range, typename Value, typename Projection>
FUN_ALWAYS_INLINE decltype(auto) FindBy(Range& range, const Value& value, Projection proj) {
  return impl::FindBy(range, value, MoveTemp(proj));
}

template <typename Range, typename Predicate>
FUN_ALWAYS_INLINE decltype(auto) FindIf(Range& range, Predicate pred) {
  return impl::FindIf(range, MoveTemp(pred));
}

} // namespace algo
} // namespace fun
