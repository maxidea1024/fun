#pragma once

namespace fun {

/**
 * Aligns a value to the nearest higher multiple of 'alignment', which must be a power of two.
 *
 * \param ptr - Value to align
 * \param alignment - alignment, must be a power of two
 *
 * \return Aligned value
 */
template <typename T>
inline constexpr T Align(const T ptr, int32 alignment)
{
  return (T)(((intptr_t)ptr + alignment - 1) & ~(alignment - 1));
}

/**
 * Aligns a value to the nearest lower multiple of 'alignment', which must be a power of two.
 *
 * \param ptr - Value to align
 * \param alignment - alignment, must be a power of two
 *
 * \return Aligned value
 */
template <typename T>
inline constexpr T AlignDown(const T ptr, int32 alignment)
{
  return (T)(((intptr_t)ptr) & ~(alignment - 1));
}

/**
 * Checks if a pointer is aligned to the specified alignment.
 *
 * \param ptr - The pointer to check.
 *
 * \return true if the pointer is aligned, false otherwise.
 */
static inline bool IsAligned(const volatile void* ptr, const uint32 alignment)
{
  return !(uintptr_t(ptr) & (alignment - 1));
}

/**
 * Aligns a value to the nearest higher multiple of 'alignment'.
 *
 * \param ptr - Value to align
 * \param alignment - alignment, can be any arbitrary uint32
 *
 * \return Aligned value
 */
template <typename T> inline T AlignArbitrary(const T ptr, uint32 alignment)
{
  return (T)((((uintptr_t)ptr + alignment - 1) / alignment) * alignment);
}

} // namespace fun
