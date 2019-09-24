#if !defined(__FUN_BASE_BASE_H__)
#error Do not include this file directly. It should be included only in base.h file.
#endif

namespace fun {

/**
 * Aligns a value to the nearest higher multiple of 'alignment',
 * which must be a power of two.
 */
template <typename T>
inline constexpr T Align(const T ptr, const uint32 alignment) {
  return (T)(((intptr_t)ptr + alignment - 1) & ~((intptr_t)alignment - 1));
}

/**
 * Aligns a value to the nearest lower multiple of 'alignment',
 * which must be a power of two.
 */
template <typename T>
inline constexpr T AlignDown(const T ptr, const uint32 alignment) {
  return (T)(((intptr_t)ptr) & ~((intptr_t)alignment - 1));
}

/**
 * Checks if a pointer is aligned to the specified alignment.
 */
template <typename T>
inline static bool IsAligned(const volatile void* ptr, const uint32 alignment) {
  return !(uintptr_t(ptr) & (alignment - 1)) == 0;
}

/**
 * Aligns a value to nearest higher multiple of 'alignment'.
 */
template <typename T>
inline static T AlignArbitrary(const T ptr, const uint32 alignment) {
  return (T)((((uintptr_t)ptr + alignment - 1) / alignment) * alignment);
}

}  // namespace fun
