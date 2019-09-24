#pragma once

#include "fun/base/ftl/identity_functor.h"
#include "fun/base/ftl/invoke.h"
#include "fun/base/ftl/less.h"

namespace fun {
namespace algo {

namespace BinarySearch_internal {

  /**
   * Performs binary search, resulting in position of the first element >= value
   *
   * \param first Pointer to array
   * \param count Number of elements in array
   * \param value value to look for
   * \param projection Called on values in array to get type that can be compared to value
   * \param sort_predicate Predicate for sort comparison
   *
   * \returns Position of the first element >= value, may be == count
   */
  template <typename RangeValueType, typename PredicateValueType, typename ProjectionType, typename SortPredicateType>
  inline size_t LowerBoundInternal(RangeValueType* first,
                                  const size_t count,
                                  const PredicateValueType& value,
                                  ProjectionType projection,
                                  SortPredicateType sort_predicate)
  {
    // Current start of sequence to check
    size_t start = 0;
    // size of sequence to check
    size_t size = count;

    // With this method, if size is even it will do one more comparison than necessary, but because size can be predicted by the CPU it is faster in practice
    while (size > 0) {
      const size_t left_over_size = size % 2;
      size = size / 2;

      const size_t check_index = start + size;
      const size_t start_if_less = check_index + left_over_size;

      auto&& check_value = Invoke(projection, first[check_index]);
      start = sort_predicate(check_value, value) ? start_if_less : start;
    }

    return start;
  }

  /**
   * Performs binary search, resulting in position of the first element that is larger than the given value
   *
   * \param first Pointer to array
   * \param count Number of elements in array
   * \param value value to look for
   * \param sort_predicate Predicate for sort comparison
   *
   * \returns Position of the first element > value, may be == count
   */
  template <typename RangeValueType, typename PredicateValueType, typename ProjectionType, typename SortPredicateType>
  inline size_t UpperBoundInternal(RangeValueType* first,
                                  const size_t count,
                                  const PredicateValueType& value,
                                  ProjectionType projection,
                                  SortPredicateType sort_predicate)
  {
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

      auto&& check_value = Invoke(projection, first[check_index]);
      start = !sort_predicate(value, check_value) ? start_if_less : start;
    }

    return start;
  }

} // namespace BinarySearch_internal


/**
 * Performs binary search, resulting in position of the first element >= value using predicate
 *
 * \param range range to search through, must be already sorted by sort_predicate
 * \param value value to look for
 * \param sort_predicate Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element >= value, may be position after last element in range
 */
template <typename RangeType, typename ValueType, typename SortPredicateType>
inline int32 LowerBound(RangeType& range, const ValueType& value, SortPredicateType sort_predicate)
{
  return BinarySearch_internal::LowerBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), sort_predicate);
}

template <typename RangeType, typename ValueType>
inline int32 LowerBound(RangeType& range, const ValueType& value)
{
  return BinarySearch_internal::LowerBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), Less<>());
}

/**
 * Performs binary search, resulting in position of the first element with projected value >= value using predicate
 *
 * \param range range to search through, must be already sorted by sort_predicate
 * \param value value to look for
 * \param projection Functor or data member pointer, called via Invoke to compare to value
 * \param sort_predicate Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element >= value, may be position after last element in range
 */
template <typename RangeType, typename ValueType, typename ProjectionType, typename SortPredicateType>
inline int32 LowerBoundBy(RangeType& range, const ValueType& value, ProjectionType projection, SortPredicateType sort_predicate)
{
  return BinarySearch_internal::LowerBoundInternal(GetMutableData(range), GetCount(range), value, projection, sort_predicate);
}

template <typename RangeType, typename ValueType, typename ProjectionType>
inline int32 LowerBoundBy(RangeType& range, const ValueType& value, ProjectionType projection)
{
  return BinarySearch_internal::LowerBoundInternal(GetMutableData(range), GetCount(range), value, projection, Less<>());
}

/**
 * Performs binary search, resulting in position of the first element > value using predicate
 *
 * \param range range to search through, must be already sorted by sort_predicate
 * \param value value to look for
 * \param sort_predicate Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element > value, may be past end of range
 */
template <typename RangeType, typename ValueType, typename SortPredicateType>
inline int32 UpperBound(RangeType& range, const ValueType& value, SortPredicateType sort_predicate)
{
  return BinarySearch_internal::UpperBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), sort_predicate);
}

template <typename RangeType, typename ValueType>
inline int32 UpperBound(RangeType& range, const ValueType& value)
{
  return BinarySearch_internal::UpperBoundInternal(GetMutableData(range), GetCount(range), value, IdentityFunctor(), Less<>());
}

/**
 * Performs binary search, resulting in position of the first element with projected value > value using predicate
 *
 * \param range range to search through, must be already sorted by sort_predicate
 * \param value value to look for
 * \param projection Functor or data member pointer, called via Invoke to compare to value
 * \param sort_predicate Predicate for sort comparison, defaults to <
 *
 * \returns Position of the first element > value, may be past end of range
 */
template <typename RangeType, typename ValueType, typename ProjectionType, typename SortPredicateType>
inline int32 UpperBoundBy(RangeType& range, const ValueType& value, ProjectionType projection, SortPredicateType sort_predicate)
{
  return BinarySearch_internal::UpperBoundInternal(GetMutableData(range), GetCount(range), value, projection, sort_predicate);
}

template <typename RangeType, typename ValueType, typename ProjectionType>
inline int32 UpperBoundBy(RangeType& range, const ValueType& value, ProjectionType projection)
{
  return BinarySearch_internal::UpperBoundInternal(GetMutableData(range), GetCount(range), value, projection);
}

/**
 * Returns index to the first found element matching a value in a range, the range must be sorted by <
 *
 * \param range The range to search, must be already sorted by sort_predicate
 * \param value The value to search for
 * \param sort_predicate Predicate for sort comparison, defaults to <
 * \return Index of found element, or INVALID_INDEX
 */
template <typename RangeType, typename ValueType, typename SortPredicateType>
inline int32 BinarySearch(RangeType& range, const ValueType& value, SortPredicateType sort_predicate)
{
  const size_t check_index = LowerBound(range, value, sort_predicate);
  if (check_index < GetCount(range)) {
    auto&& check_value = GetMutableData(range)[check_index];
    // Since we returned lower bound we already know value <= check_value. So if value is not < check_value, they must be equal
    if (!sort_predicate(value, check_value)) {
      return check_index;
    }
  }
  return INVALID_INDEX;
}

template <typename RangeType, typename ValueType>
inline int32 BinarySearch(RangeType& range, const ValueType& value)
{
  return BinarySearch(range, value, Less<>());
}

/**
 * Returns index to the first found element with projected value matching value in a range, the range must be sorted by predicate
 *
 * \param range The range to search, must be already sorted by sort_predicate
 * \param value The value to search for
 * \param projection Functor or data member pointer, called via Invoke to compare to value
 * \param sort_predicate Predicate for sort comparison, defaults to <
 * \return Index of found element, or INVALID_INDEX
 */
template <typename RangeType, typename ValueType, typename ProjectionType, typename SortPredicateType>
inline int32 BinarySearchBy(RangeType& range, const ValueType& value, ProjectionType projection, SortPredicateType sort_predicate)
{
  const size_t check_index = LowerBoundBy(range, value, projection, sort_predicate);
  if (check_index < GetCount(range)) {
    auto&& check_value = Invoke(projection, GetMutableData(range)[check_index]);
    // Since we returned lower bound we already know value <= check_value. So if value is not < check_value, they must be equal
    if (!sort_predicate(value, check_value)) {
      return check_index;
    }
  }
  return INVALID_INDEX;
}

template <typename RangeType, typename ValueType, typename ProjectionType>
inline int32 BinarySearchBy(RangeType& range, const ValueType& value, ProjectionType projection)
{
  return BinarySearchBy(range, value, projection, Less<>());
}

} // namespace algo
} // namespace fun
