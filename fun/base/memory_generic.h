#pragma once

#include "fun/base/base.h"

namespace fun {

class MemoryAllocator;
class Printer;
class String;

/** Holds generic memory stats, internally implemented as a map. */
struct CGenericMemoryStats;

/**
 * Struct used to hold common memory constants for all platforms.
 * These values don't change over the entire life of the executable.
 */
struct CGenericPlatformMemoryConstants {
  /** The amount of actual physical memory, in bytes. */
  size_t TotalPhysical;

  /** The amount of virtual memory, in bytes. */
  size_t TotalVirtual;

  /** The size of a page, in bytes. */
  size_t PageSize;

  /**
   * For platforms that support multiple page sizes this is non-zero and smaller than PageSize.
   * If non-zero, then BinnedAllocFromOS will take allocation requests aligned to this size and return blocks aligned to PageSize
   */
  size_t OsAllocationGranularity;

  /**
   * AddressLimit - Second parameter is estimate of the range of addresses expected to be returns by BinnedAllocFromOS(). Binned
   * Malloc will adjust its internal structures to make lookups for memory allocations O(1) for this range.
   * It is ok to go outside this range, lookups will just be a little slower
   */
  uint64 AddressLimit;

  /** Approximate physical RAM in GB; 1 on everything except PC. Used for "course tuning", like CPlatformMisc::NumberOfCores(). */
  uint32 TotalPhysicalGB;

  /** Default constructor, clears all variables. */
  CGenericPlatformMemoryConstants()
    : TotalPhysical(0)
    , TotalVirtual(0)
    , PageSize(0)
    , OsAllocationGranularity(0)
    , AddressLimit((uint64)0xFFFFFFFF + 1)
    , TotalPhysicalGB(1)
  {}

  /** Copy constructor, used by the generic platform memory stats. */
  CGenericPlatformMemoryConstants(const CGenericPlatformMemoryConstants& Other)
    : TotalPhysical(Other.TotalPhysical)
    , TotalVirtual(Other.TotalVirtual)
    , PageSize(Other.PageSize)
    , OsAllocationGranularity(Other.OsAllocationGranularity)
    , AddressLimit(Other.AddressLimit)
    , TotalPhysicalGB(Other.TotalPhysicalGB)
  {}
};

typedef CGenericPlatformMemoryConstants CPlatformMemoryConstants;

/**
 * Struct used to hold common memory stats for all platforms.
 * These values may change over the entire life of the executable.
 */
struct CGenericPlatformMemoryStats : public CPlatformMemoryConstants
{
  /** The amount of physical memory currently available, in bytes. */
  size_t AvailablePhysical;

  /** The amount of virtual memory currently available, in bytes. */
  size_t AvailableVirtual;

  /** The amount of physical memory used by the process, in bytes. */
  size_t UsedPhysical;

  /** The peak amount of physical memory used by the process, in bytes. */
  size_t PeakUsedPhysical;

  /** Total amount of virtual memory used by the process. */
  size_t UsedVirtual;

  /** The peak amount of virtual memory used by the process. */
  size_t PeakUsedVirtual;

  /** Default constructor, clears all variables. */
  CGenericPlatformMemoryStats();
};

struct CPlatformMemoryStats;

/**
 * ALLOCA/alloca implementation. This can't be a function, even inline'd because there's no guarantee that
 * the memory returned in a function will stick around for the caller to use.
 */
#if PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS
#define __ALLOCA_Func  _alloca
#else
#define __ALLOCA_Func  alloca
#endif

#define ALLOCA(size)  ((size == 0) ? 0 : (void*)(((intptr_t)__ALLOCA_Func(size + 15) + 15) & ~15))

/** Generic implementation for most platforms, these tend to be unused and unimplemented. */
struct FUN_BASE_API CGenericPlatformMemory
{
  /** Set to true if we encounters out of memory. */
  static bool bIsOOM;

  /** Set to size of allocation that triggered out of memory, zero otherwise. */
  static uint64 OOMAllocationSize;

  /** Set to alignment of allocation that triggered out of memory, zero otherwise. */
  static uint32 OOMAllocationAlignment;

  /** Preallocated buffer to delete on out of memory. Used by OOM handling and crash reporting. */
  static void* BackupOOMMemoryPool;

  /** size of BackupOOMMemoryPool in bytes. */
  static uint32 BackupOOMMemoryPoolSize;

  /**
   * Various memory regions that can be used with memory stats. The exact meaning of
   * the enums are relatively platform-dependent, although the general ones (Physical, GPU)
   * are straightforward. A platform can add more of these, and it won't affect other
   * platforms, other than a minuscule amount of memory for the StatManager to track the
   * max available memory for each region (uses an array PlatformMemory::MCR_MAX big)
   */
  enum EMemoryCounterRegion {
    MCR_Invalid, // not memory
    MCR_Physical, // main system memory
    MCR_GPU, // memory directly a GPU (graphics card, etc)
    MCR_GPUSystem, // system memory directly accessible by a GPU
    MCR_TexturePool, // presized texture pools

    MCR_StreamingPool, // amount of texture pool available for streaming.
    MCR_UsedStreamingPool, // amount of texture pool used for streaming.
    MCR_GPUDefragPool, // presized pool of memory that can be defragmented.

    MCR_MAX
  };

  /**
   * Flags used for shared memory creation/open
   */
  enum ESharedMemoryAccess {
    Read = 0x02,
    Write = 0x04
  };

  /**
   * Generic representation of a shared memory region
   */
  struct CSharedMemoryRegion {
    /** Returns the name of the region */
    const TCHAR* GetName() const { return Name; }

    /** Returns the beginning of the region in process address space */
    void* GetAddress() { return Address; }

    /** Returns the beginning of the region in process address space */
    const void* GetAddress() const { return Address; }

    /** Returns size of the region in bytes */
    size_t GetSize() const { return size; }

    CSharedMemoryRegion(const String& InName, uint32 InAccessMode, void* InAddress, size_t InSize);

  protected:
    enum Limits {
      MaxSharedMemoryName = 128
    };

    /** Name of the region */
    TCHAR Name[MaxSharedMemoryName];

    /** Access mode for the region */
    uint32 AccessMode;

    /** The actual buffer */
    void* Address;

    /** size of the buffer */
    size_t size;
  };

  /** Initializes platform memory specific constants. */
  static void Init();

  static CA_NO_RETURN void OnOutOfMemory(uint64 size, uint32 Alignment);

  /** Initializes the memory pools, should be called by the init function. */
  static void SetupMemoryPools();

  /**
   * @return whether platform supports memory pools for crash reporting.
   */
  static bool SupportBackupMemoryPool()
  {
    return false;
  }

  /**
   * @return the default allocator.
   */
  static MemoryAllocator* BaseAllocator();

  /**
   * @return platform specific current memory statistics.
   */
  static CPlatformMemoryStats GetStats();

  /**
   * Writes all platform specific current memory statistics in the format usable by the malloc profiler.
   */
  static void GetStatsForMallocProfiler(CGenericMemoryStats& OutStats);

  /**
   * @return platform specific memory constants.
   */
  static const CPlatformMemoryConstants& GetConstants();

  /**
   * @return approximate physical RAM in GB.
   */
  static uint32 GetPhysicalGBRam();

  /**
   * Changes the protection on a region of committed pages in the virtual address space.
   *
   * @param Ptr - Address to the starting page of the region of pages whose access protection attributes are to be changed.
   * @param size - The size of the region whose access protection attributes are to be changed, in bytes.
   * @param can_read - Can the memory be read.
   * @param can_write - Can the memory be written to.
   *
   * @return True if the specified pages' protection mode was changed.
   */
  static bool PageProtect(void* const Ptr, const size_t size, const bool can_read, const bool can_write);

  /**
   * Allocates pages from the OS.
   *
   * @param size - size to allocate, not necessarily aligned
   *
   * @return OS allocated pointer for use by binned allocator
   */
  static void* BinnedAllocFromOS(size_t size);

  /**
   * Returns pages allocated by BinnedAllocFromOS to the OS.
   *
   * @param Ptr - A pointer previously returned from BinnedAllocFromOS
   * @param size - size of the allocation previously passed to BinnedAllocFromOS
   */
  static void BinnedFreeToOS(void* Ptr, size_t size);

  // These alloc/free memory that is mapped to the GPU
  // Only for platforms with UMA (XB1/PS4/etc)
  static void* GPUMalloc(size_t Count, uint32 Alignment = 0) { return nullptr; };
  static void* GPURealloc(void* Original, size_t Count, uint32 Alignment = 0) { return nullptr; };
  static void GPUFree(void* Original) {};

  /** Dumps basic platform memory statistics into the specified printer. */
  static void DumpStats(Printer& Ar);

  /** Dumps basic platform memory statistics and allocator specific statistics into the specified printer. */
  static void DumpPlatformAndAllocatorStats(Printer& Ar);

  /** @name Memory functions */

  /**
   * Copies count bytes of characters from Src to Dest. If some regions of the source
   * area and the destination overlap, memmove ensures that the original source bytes
   * in the overlapping region are copied before being overwritten.  NOTE: make sure
   * that the destination buffer is the same size or larger than the source buffer!
   */
  static inline void* Memmove(void* Dest, const void* Src, size_t Count)
  {
    return memmove(Dest, Src, Count);
  }

  static inline int32 Memcmp(const void* Buf1, const void* Buf2, size_t Count)
  {
    return memcmp(Buf1, Buf2, Count);
  }

  static inline void* Memset(void* Dest, uint8 Char, size_t Count)
  {
    return memset(Dest, Char, Count);
  }

  static inline void* Memzero(void* Dest, size_t Count)
  {
    return memset(Dest, 0, Count);
  }

  static inline void* Memcpy(void* Dest, const void* Src, size_t Count)
  {
    return memcpy(Dest, Src, Count);
  }

  /** Memcpy optimized for big blocks. */
  static inline void* BigBlockMemcpy(void* Dest, const void* Src, size_t Count)
  {
    return memcpy(Dest, Src, Count);
  }

  /** On some platforms memcpy optimized for big blocks that avoid L2 cache pollution are available */
  static inline void* StreamingMemcpy(void* Dest, const void* Src, size_t Count)
  {
    return memcpy(Dest, Src, Count);
  }

 private:
  template <typename T>
  static inline void Valswap(T& a, T& b)
  {
    // Usually such an implementation would use move semantics, but
    // we're only ever going to call it on fundamental types and MoveTemp
    // is not necessarily in scope here anyway, so we don't want to
    // #include it if we don't need to.
    T tmp = a;
    a = b;
    b = tmp;
  }

  static void MemswapImpl(void* ptr1, void* ptr2, size_t size);

 public:
  static inline void Memswap(void* ptr1, void* ptr2, size_t size)
  {
    switch (size) {
    case 1:
      Valswap(*(uint8*)ptr1, *(uint8*)ptr2);
      break;

    case 2:
      Valswap(*(uint16*)ptr1, *(uint16*)ptr2);
      break;

    case 4:
      Valswap(*(uint32*)ptr1, *(uint32*)ptr2);
      break;

    case 8:
      Valswap(*(uint64*)ptr1, *(uint64*)ptr2);
      break;

    case 16:
      Valswap(((uint64*)ptr1)[0], ((uint64*)ptr2)[0]);
      Valswap(((uint64*)ptr1)[1], ((uint64*)ptr2)[1]);
      break;

    default:
      MemswapImpl(ptr1, ptr2, size);
      break;
    }
  }

  /**
   * Maps a named shared memory region into process address space (creates or opens it)
   *
   * @param Name - unique name of the shared memory region (should not contain [back]slashes to remain cross-platform).
   * @param bCreate - whether we're creating it or just opening existing (created by some other process).
   * @param AccessMode - mode which we will be accessing it (use values from ESharedMemoryAccess)
   * @param size - size of the buffer (should be >0. Also, the real size is subject to platform limitations and may be increased to match page size)
   *
   * @return pointer to CSharedMemoryRegion (or its descendants) if successful, nullptr if not.
   */
  static CSharedMemoryRegion* MapNamedSharedMemoryRegion(const String& Name, bool bCreate, uint32 AccessMode, size_t size);

  /**
   * Unmaps a name shared memory region
   *
   * @param MemoryRegion - an object that encapsulates a shared memory region (will be destroyed even if function fails!)
   *
   * @return true if successful
   */
  static bool UnmapNamedSharedMemoryRegion(CSharedMemoryRegion* MemoryRegion);

 protected:
  friend struct CGenericStatsUpdater;

  /** Updates platform specific stats. This method is called through CGenericStatsUpdater from the task graph thread. */
  static void InternalUpdateStats(const CPlatformMemoryStats& MemoryStats);
};

} // namespace fun
