#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Used to declare an untyped array of data with compile-time alignment.
 */
template <size_t size, uint32 alignment>
struct AlignedStorage;

/**
 * Unaligned storage.
 */
template <size_t size>
struct AlignedStorage<size, 1> {
  uint8 pad[size];
};

// C++/CLI doesn't support alignment of native types in managed code, so
// we enforce that the element size is a multiple of the desired alignment.
#ifdef __cplusplus_cli

  #define __DEFINE_ALIGNED_STORAGE(align) \
    template <size_t size> \
    struct AlignedStorage<size, align> { \
      uint8 pad[size]; \
      static_assert((size % align) == 0, "CLR interop types must not be aligned."); \
    };

#else // __cplusplus_cli

  /**
   * A macro that implements AlignedStorage for a specific alignment.
   */
  #define __DEFINE_ALIGNED_STORAGE(align) \
    template <size_t size> \
    struct AlignedStorage<size, align> { \
      struct alignas(align) Padding { \
        uint8 pad[size]; \
      }; \
      Padding padding; \
    };

#endif // !__cplusplus_cli

// 16, 8, 4, 2 alignments.
__DEFINE_ALIGNED_STORAGE(16);
__DEFINE_ALIGNED_STORAGE(8);
__DEFINE_ALIGNED_STORAGE(4);
__DEFINE_ALIGNED_STORAGE(2);

#undef __DEFINE_ALIGNED_STORAGE


/**
 * An untyped array of data with compile-time alignment and
 * size derived from another type.
 */
template <typename T>
struct TypeCompatibleStorage : public AlignedStorage<sizeof(T), alignof(T)> {};

} // namespace fun
