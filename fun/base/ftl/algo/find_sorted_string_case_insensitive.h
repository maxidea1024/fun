// TODO 제거하자
// TODO 제거하자
// TODO 제거하자
// TODO 제거하자
// TODO 제거하자
// TODO 제거하자
// TODO 제거하자
// TODO 제거하자
#pragma once

#include "fun/base/base.h"

namespace fun {
namespace algo {

/**
 * Finds a string in an array of sorted strings, by case-insensitive search, by
 * using binary subdivision of the array.
 *
 * \param str - The string to look for.
 * \param sorted_array - The array of strings to search.  The strings must be
 * sorted lexicographically, case-insensitively. \param ArrayCount - The number
 * of strings in the array.
 *
 * \return The index of the found string in the array, or -1 if the string was
 * not found.
 */
FUN_BASE_API int32 FindSortedStringCaseInsensitive(
    const char* str, const char* const* sorted_array, int32 array_count);

/**
 * Finds a string in an array of sorted strings, by case-insensitive search, by
 * using binary subdivision of the array.
 *
 * \param str - The string to look for.
 * \param sorted_array - The array of strings to search.  The strings must be
 * sorted lexicographically, case-insensitively.
 *
 * \return The index of the found string in the array, or -1 if the string was
 * not found.
 */
template <int32 array_count>
FUN_ALWAYS_INLINE int32 FindSortedStringCaseInsensitive(
    const char* str, const char* const (&sorted_array)[array_count]) {
  return FindSortedStringCaseInsensitive(str, sorted_array, array_count);
}

}  // namespace algo
}  // namespace fun
