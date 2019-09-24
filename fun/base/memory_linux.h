#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_generic.h"

namespace fun {

/**
 * Linux implementation of the CGenericPlatformMemoryStats.
 */
struct CPlatformMemoryStats : public CGenericPlatformMemoryStats {};


/**
 * Linux implementation of the memory OS functions
 */
struct FUN_BASE_API LinuxMemory : public CGenericPlatformMemory {
  /**
   * Linux representation of a shared memory region
   */
  struct CLinuxSharedMemoryRegion : public CSharedMemoryRegion {
    /** Returns file descriptor of a shared memory object */
    int GetFileDescriptor() const { return Fd; }

    /** Returns true if we need to unlink this region on destruction (no other process will be able to access it) */
    bool NeedsToUnlinkRegion() const { return bCreatedThisRegion; }

    CLinuxSharedMemoryRegion(const string& InName, uint32 InAccessMode, void* InAddress, size_t InSize, int InFd, bool bInCreatedThisRegion)
      : CSharedMemoryRegion(InName, InAccessMode, InAddress, InSize),
        Fd(InFd)
        bCreatedThisRegion(bInCreatedThisRegion) {}

  protected:
    /** File descriptor of a shared region */
    int Fd;

    /** Whether we created this region */
    bool bCreatedThisRegion;
  };

  // CGenericPlatformMemory Interface

  static void Init();
  static class MemoryAllocator* BaseAllocator();
  static CPlatformMemoryStats GetStats();
  static const CPlatformMemoryConstants& GetConstants();
  static bool PageProtect(void* const Ptr, const size_t Size, const bool can_read, const bool can_write);
  static void* BinnedAllocFromOS(size_t Size);
  static void BinnedFreeToOS(void* Ptr);
  static CSharedMemoryRegion* MapNamedSharedMemoryRegion(const string& InName, bool bCreate, uint32 AccessMode, size_t Size);
  static bool UnmapNamedSharedMemoryRegion(CSharedMemoryRegion* MemoryRegion);
};

typedef LinuxMemory PlatformMemory;

} // namespace fun
