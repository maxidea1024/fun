//TODO 제거하자
//TODO 제거하자
//TODO 제거하자
//TODO 제거하자
//TODO 제거하자
//TODO 제거하자
//TODO 제거하자
//TODO 제거하자
#pragma once

namespace fun {
namespace algo {

/**
 * Finds a string in an array of sorted strings, by case-insensitive search, by using binary subdivision of the array.
 *
 * \param str - The string to look for.
 * \param sorted_array - The array of strings to search.  The strings must be sorted lexicographically, case-insensitively.
 * \param ArrayCount - The number of strings in the array.
 *
 * \return The index of the found string in the array, or -1 if the string was not found.
 */
FUN_BASE_API int32 FindSortedStringCaseInsensitive(const char* str, const char* const* sorted_array, int32 ArrayCount);

/**
 * Finds a string in an array of sorted strings, by case-insensitive search, by using binary subdivision of the array.
 *
 * \param str - The string to look for.
 * \param sorted_array - The array of strings to search.  The strings must be sorted lexicographically, case-insensitively.
 *
 * \return The index of the found string in the array, or -1 if the string was not found.
 */
template <int32 ArraySize>
inline int32 FindSortedStringCaseInsensitive(const char* str, const char* const (&sorted_array)[ArraySize])
{
  return FindSortedStringCaseInsensitive(str, sorted_array, ArraySize);
}

} // namespace algo
} // namespace fun
