#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"

namespace fun {
namespace algo {

namespace impl {

template <typename T>
FUN_ALWAYS_INLINE void ReverseInternal(T* array, size_t array_count) {
  for (size_t i = 0, i2 = array_count - 1;
       i < array_count / 2 /*rounding down*/; ++i, --i2) {
    fun::Swap(array[i], array[i2]);
  }
}

}  // namespace impl

/**
 * Reverses a range
 *
 * \param array - The array to reverse.
 */
template <typename T, size_t N>
FUN_ALWAYS_INLINE void Reverse(T (&array)[N]) {
  return impl::ReverseInternal((T*)array, N);
}

/**
 * Reverses a range
 *
 * \param array - A pointer to the array to reverse
 * \param array_count - The number of elements in the array.
 */
template <typename T>
FUN_ALWAYS_INLINE void Reverse(T* array, size_t array_count) {
  return impl::ReverseInternal(array, array_count);
}

/**
 * Reverses a range
 *
 * \param container - The container to reverse
 */
template <typename ContainerType>
FUN_ALWAYS_INLINE void Reverse(ContainerType& container) {
  return impl::ReverseInternal(container.MutableData(), container.Count());
}

}  // namespace algo
}  // namespace fun
