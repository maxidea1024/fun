#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_generic.h"

namespace fun {

/**
 * Max implementation of the CGenericPlatformMemoryStats.
 */
struct CPlatformMemoryStats : public CGenericPlatformMemoryStats {};

/**
 * Mac implementation of the memory OS functions
 */
struct FUN_BASE_API MacMemory : public CGenericPlatformMemory {
  static void Init();
  static CPlatformMemoryStats GetStats();
  static const CPlatformMemoryConstants& GetConstants();
  static MemoryAllocator* BaseAllocator();
  static bool PageProtect(void* const ptr, const size_t size, const bool can_read, const bool can_write);
  static void* BinnedAllocFromOS(size_t size);
  static void BinnedFreeToOS(void* ptr, size_t size);
};

typedef MacMemory PlatformMemory;

} // namespace fun
