#pragma once

namespace fun {
namespace algo {

namespace Reverse_internal {

  template <typename T>
  inline void Reverse(T* array, int32 array_count)
  {
    for (int32 i = 0, i2 = array_count - 1; i < array_count / 2 /*rounding down*/; ++i, --i2)
    {
      Swap(array[i], array[i2]);
    }
  }

} // namespace Reverse_internal

/**
 * Reverses a range
 *
 * \param array - The array to reverse.
 */
template <typename T, int32 N>
inline void Reverse(T (&array)[N])
{
  return Reverse_internal::Reverse((T*)array, N);
}

/**
 * Reverses a range
 *
 * \param array - A pointer to the array to reverse
 * \param array_count - The number of elements in the array.
 */
template <typename T>
inline void Reverse(T* array, int32 array_count)
{
  return Reverse_internal::Reverse(array, array_count);
}

/**
 * Reverses a range
 *
 * \param container - The container to reverse
 */
template <typename ContainerType>
inline void Reverse(ContainerType& container)
{
  return Reverse_internal::Reverse(container.MutableData(), container.Count());
}

} // namespace algo
} // namespace fun
