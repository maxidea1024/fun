#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/functional.h" // Invoke

namespace fun {
namespace algo {

namespace impl {

/**
 * Performs binary search, resulting in position of the first element >= value
 *
 * \param first Pointer to array
 * \param count Number of elements in array
 * \param value value to look for
 * \param proj Called on values in array to get type that can be compared to value
 * \param sort_pred Predicate for sort comparison
 *
 * \returns Position of the first element >= value, may be == count
 */
template <
    typename RangeValue,
    typename PredicateValue,
    typename Projection,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE size_t LowerBoundInternal(
      RangeValue* first,
      const size_t count,
      const PredicateValue& value,
      const Projection& proj,
      const SortPredicate& sort_pred) {
  // Current start of sequence to check
  size_t start = 0;
  // size of sequence to check
  size_t size = count;

  // With this method, if size is even it will do one more comparison than necessary,
  // but because size can be predicted by the CPU it is faster in practice
  while (size > 0) {
    const size_t left_over_size = size % 2;
    size = size / 2;

    const size_t check_index = start + size;
    const size_t start_if_less = check_index + left_over_size;

    auto&& check_value = Invoke(proj, first[check_index]);
    start = sort_pred(check_value, value) ? start_if_less : start;
  }

  return start;
}

/**
 * Performs binary search, resulting in position of the first element
 * that is larger than the given value
 *
 * \param first Pointer to array
 * \param count Number of elements in array
 * \param value value to look for
 * \param sort_pred Predicate for sort comparison
 *
 * \returns Position of the first element > value, may be == count
 */
template <
    typename RangeValue,
    typename PredicateValue,
    typename Projection,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE size_t UpperBoundInternal(
      RangeValue* first,
      const size_t count,
      const PredicateValue& value,
      const Projection& proj,
      const SortPredicate& sort_pred) {
  // Current start of sequence to check
  size_t start = 0;
  // size of sequence to check
  size_t size = count;

  // With this method, if size is even it will do one more comparison
  // than necessary, but because size can be predicted
  // by the CPU it is faster in practice
  while (size > 0) {
    const size_t left_over_size = size % 2;
    size = size / 2;

    const size_t check_index = start + size;
    const size_t start_if_less = check_index + left_over_size;

    auto&& check_value = Invoke(proj, first[check_index]);
    start = !sort_pred(value, check_value) ? start_if_less : start;
  }

  return start;
}

} // namespace impl

/**
 * Performs binary search, resulting in position of the first element >= value using predicate
 *
 * \param range range to search through, must be already sorted by sort_pred
 * \param value value to look for
 * \param sort_pred Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element >= value, may be position after last element in range
 */
template <
    typename Range,
    typename Value,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE int32 LowerBound(
      Range& range,
      const Value& value,
      const SortPredicate& sort_pred) {
  return impl::LowerBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), sort_pred);
}

template <typename Range, typename Value>
FUN_ALWAYS_INLINE int32 LowerBound(Range& range, const Value& value) {
  return impl::LowerBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), Less<>());
}

/**
 * Performs binary search, resulting in position of the first element with projected value >= value using predicate
 *
 * \param range range to search through, must be already sorted by sort_pred
 * \param value value to look for
 * \param proj Functor or data member pointer, called via Invoke to compare to value
 * \param sort_pred Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element >= value, may be position after last element in range
 */
template <
    typename Range,
    typename Value,
    typename Projection,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE int32 LowerBoundBy(
      Range& range,
      const Value& value,
      const Projection& proj,
      const SortPredicate& sort_pred) {
  return impl::LowerBoundInternal(GetMutableData(range), GetCount(range), value, proj, sort_pred);
}

template <
    typename Range,
    typename Value,
    typename Projection
  >
FUN_ALWAYS_INLINE int32 LowerBoundBy(
      Range& range,
      const Value& value,
      const Projection& proj) {
  return impl::LowerBoundInternal(GetMutableData(range), GetCount(range), value, proj, Less<>());
}

/**
 * Performs binary search, resulting in position of the first element > value using predicate
 *
 * \param range range to search through, must be already sorted by sort_pred
 * \param value value to look for
 * \param sort_pred Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element > value, may be past end of range
 */
template <
    typename Range,
    typename Value,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE int32 UpperBound(
      Range& range,
      const Value& value,
      const SortPredicate& sort_pred) {
  return impl::UpperBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), sort_pred);
}

template <typename Range, typename Value>
FUN_ALWAYS_INLINE int32 UpperBound(Range& range, const Value& value) {
  return impl::UpperBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), Less<>());
}

/**
 * Performs binary search, resulting in position of the first element with projected value > value using predicate
 *
 * \param range range to search through, must be already sorted by sort_pred
 * \param value value to look for
 * \param proj Functor or data member pointer, called via Invoke to compare to value
 * \param sort_pred Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element > value, may be past end of range
 */
template <
    typename Range,
    typename Value,
    typename Projection,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE int32 UpperBoundBy(
      Range& range,
      const Value& value,
      const Projection& proj,
      const SortPredicate& sort_pred) {
  return impl::UpperBoundInternal(GetMutableData(range), GetCount(range), value, proj, sort_pred);
}

template <
    typename Range,
    typename Value,
    typename Projection
  >
FUN_ALWAYS_INLINE int32 UpperBoundBy(
      Range& range,
      const Value& value,
      const Projection& proj) {
  return impl::UpperBoundInternal(GetMutableData(range), GetCount(range), value, proj);
}

/**
 * Returns index to the first found element matching a value in a range, the range must be sorted by <
 *
 * \param range The range to search, must be already sorted by sort_pred
 * \param value The value to search for
 * \param sort_pred Predicate for sort comparison, defaults to <
 * \return Index of found element, or INVALID_INDEX
 */
template <
    typename Range,
    typename Value,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE int32 BinarySearch(
      Range& range,
      const Value& value,
      const SortPredicate& sort_pred) {
  const size_t check_index = LowerBound(range, value, sort_pred);
  if (check_index < GetCount(range)) {
    auto&& check_value = GetMutableData(range)[check_index];
    // Since we returned lower bound we already know value <= check_value.
    // So if value is not < check_value, they must be equal
    if (!sort_pred(value, check_value)) {
      return check_index;
    }
  }
  return INVALID_INDEX;
}

template <typename Range, typename Value>
FUN_ALWAYS_INLINE int32 BinarySearch(Range& range, const Value& value) {
  return BinarySearch(range, value, Less<>());
}

/**
 * Returns index to the first found element with projected value matching
 * value in a range, the range must be sorted by predicate
 *
 * \param range The range to search, must be already sorted by sort_pred
 * \param value The value to search for
 * \param proj Functor or data member pointer, called via Invoke to compare to value
 * \param sort_pred Predicate for sort comparison, defaults to <
 * \return Index of found element, or INVALID_INDEX
 */
template <
    typename Range,
    typename Value,
    typename Projection,
    typename SortPredicate
  >
FUN_ALWAYS_INLINE int32 BinarySearchBy(
      Range& range,
      const Value& value,
      const Projection& proj,
      const SortPredicate& sort_pred) {
  const size_t check_index = LowerBoundBy(range, value, proj, sort_pred);
  if (check_index < GetCount(range)) {
    auto&& check_value = Invoke(proj, GetMutableData(range)[check_index]);
    // Since we returned lower bound we already know value <= check_value.
    // So if value is not < check_value, they must be equal
    if (!sort_pred(value, check_value)) {
      return check_index;
    }
  }
  return INVALID_INDEX;
}

template <
    typename Range,
    typename Value,
    typename Projection
  >
FUN_ALWAYS_INLINE int32 BinarySearchBy(
      Range& range,
      const Value& value,
      const Projection& proj) {
  return BinarySearchBy(range, value, proj, Less<>());
}

} // namespace algo
} // namespace fun
