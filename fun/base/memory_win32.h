#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_generic.h"

namespace fun {

/**
 *  Windows implementation of the CGenericPlatformMemoryStats.
 *  At this moment it's just the same as the CGenericPlatformMemoryStats.
 *  Can be extended as shown in the following example.
 */
struct CPlatformMemoryStats : public CGenericPlatformMemoryStats {
  /** Default constructor, clears all variables. */
  CPlatformMemoryStats()
    : CGenericPlatformMemoryStats(),
    , WindowsSpecificMemoryStat(0)
  {}

  /** Memory stat specific only for Windows. */
  size_t WindowsSpecificMemoryStat;
};

/**
 * Windows implementation of the memory OS functions
 */
struct FUN_BASE_API CWindowsMemory : public CGenericPlatformMemory
{
  enum EMemoryCounterRegion
  {
    MCR_Invalid, // not memory
    MCR_Physical, // main system memory
    MCR_GPU, // memory directly a GPU (graphics card, etc)
    MCR_GPUSystem, // system memory directly accessible by a GPU
    MCR_TexturePool, // presized texture pools

    MCR_StreamingPool, // amount of texture pool available for streaming.
    MCR_UsedStreamingPool, // amount of texture pool used for streaming.
    MCR_GPUDefragPool, // presized pool of memory that can be defragmented.

    MCR_SamplePlatformSpecifcMemoryRegion,

    MCR_MAX
  };

  /**
   * Windows representation of a shared memory region
   */
  struct CWindowsSharedMemoryRegion : public CSharedMemoryRegion
  {
    /** Returns the handle to file mapping object. */
    HANDLE GetMapping() const { return Mapping; }

    CWindowsSharedMemoryRegion(const string& Name, uint32 AccessMode, void* Address, size_t Size, HANDLE Mapping)
      : CSharedMemoryRegion(Name, AccessMode, Address, Size)
      , Mapping(Mapping)
    {}

  protected:
    /** Handle of a file mapping object */
    HANDLE Mapping;
  };

  // CGenericPlatformMemory Interface
  static void Init();
  static bool SupportBackupMemoryPool() { return true; }
  static class MemoryAllocator* BaseAllocator();
  static CPlatformMemoryStats GetStats();
  static void GetStatsForMallocProfiler(CGenericMemoryStats& OutStats);
  static const CPlatformMemoryConstants& GetConstants();
  static void* BinnedAllocFromOS(size_t Size);
  static void BinnedFreeToOS(void* ptr, size_t Size);
  static bool PageProtect(void* const ptr, const size_t Size, const bool can_read, const bool can_write);
  static CSharedMemoryRegion* MapNamedSharedMemoryRegion(const string& InName, bool bCreate, uint32 AccessMode, size_t Size);
  static bool UnmapNamedSharedMemoryRegion(CSharedMemoryRegion* MemoryRegion);

protected:
  friend struct CGenericStatsUpdater;

  static void InternalUpdateStats(const CPlatformMemoryStats& MemoryStats);
};

typedef CWindowsMemory PlatformMemory;

} // namespace fun
