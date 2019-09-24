#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_generic.h"

namespace fun {

/**
 * Android implementation of the CGenericPlatformMemoryStats.
 */
struct CPlatformMemoryStats : public CGenericPlatformMemoryStats {};

/**
 * Android implementation of the memory OS functions
 */
struct FUN_BASE_API AndroidMemory : public CGenericPlatformMemory {
  static void Init();
  static CPlatformMemoryStats GetStats();
  static const CPlatformMemoryConstants& GetConstants();
  static MemoryAllocator* BaseAllocator();
  static void* BinnedAllocFromOS(size_t Size);
  static void BinnedFreeToOS(void* ptr);
};

typedef AndroidMemory PlatformMemory;

}  // namespace fun
