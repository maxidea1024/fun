#pragma once

#include "Templates/Less.h

/*
정렬되어 있는지 여부를 알아본다.

기본적으로 오름차순인지 여부를 체크하도록 되어 있다. 만약 내림차순으로 정렬된 상태를 확인하고 싶다면,
Predicate에서 Less<T> 대신 Greater<T>를 지정하면 된다.

Predicate를 지정하지 않으면 Less<T>가 사용된다. 즉, 위에서 얘기한것처럼 오름차순인지를 확인하는 것으로
간주한다.
*/

namespace fun {
namespace algo {

namespace IsSorted_internal {

  template <typename T, typename Predicate>
  bool IsSorted(const T* range, int32 range_count, const Predicate& pred)
  {
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

      if (pred(*next, *range)) {
        return false;
      }

      ++range;
      ++next;
      --range_count;
    }
  }

} // namespace IsSorted_internal


/**
 * Tests is a range is sorted.
 *
 * \param array - The array to test for being sorted.
 *
 * \return true if the range is sorted, false otherwise.
 */
template <typename T, int32 N>
inline bool IsSorted(const T (&array)[N])
{
  return IsSorted_internal::IsSorted((const T*)array, N, Less<T>());
}

/**
 * Tests is a range is sorted.
 *
 * \param array - The array to test for being sorted.
 * \param pred - A binary sorting predicate which describes the ordering of the elements in the array.
 *
 * \return true if the range is sorted, false otherwise.
 */
template <typename T, int32 N, typename Predicate>
inline bool IsSorted(const T (&array)[N], const Predicate& pred)
{
  return IsSorted_internal::IsSorted((const T*)array, N, pred);
}

/**
 * Tests is a range is sorted.
 *
 * \param array - A pointer to the array to test for being sorted.
 * \param N - The number of elements in the array.
 *
 * \return true if the range is sorted, false otherwise.
 */
template <typename T>
inline bool IsSorted(const T* array, int32 N)
{
  return IsSorted_internal::IsSorted(array, N, Less<T>());
}

/**
 * Tests is a range is sorted.
 *
 * \param array - A pointer to the array to test for being sorted.
 * \param N - The number of elements in the array.
 * \param pred - A binary sorting predicate which describes the ordering of the elements in the array.
 *
 * \return true if the range is sorted, false otherwise.
 */
template <typename T, typename Predicate>
inline bool IsSorted(const T* array, int32 N, const Predicate& pred)
{
  return IsSorted_internal::IsSorted(array, N, pred);
}

/**
 * Tests is a range is sorted.
 *
 * \param container - The container to test for being sorted.
 *
 * \return true if the range is sorted, false otherwise.
 */
template <typename ContainerType>
inline bool IsSorted(const ContainerType& container)
{
  return IsSorted_internal::IsSorted(container.ConstData(), container.Count(), Less<typename ContainerType::ElementType>());
}

/**
 * Tests is a range is sorted.
 *
 * \param container - The container to test for being sorted.
 * \param pred - A binary sorting predicate which describes the ordering of the elements in the array.
 *
 * \return true if the range is sorted, false otherwise.
 */
template <typename ContainerType, typename Predicate>
inline bool IsSorted(const ContainerType& container, const Predicate& pred)
{
  return IsSorted_internal::IsSorted(container.ConstData(), container.Count(), pred);
}

} // namespace algo
} // namespace fun
