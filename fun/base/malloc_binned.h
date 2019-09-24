#pragma once

#include "Containers/LockFreeList.h"
#include "Containers/Array.h"

namespace fun {

#define MEM_TIME(st)

//#define USE_LOCKFREE_DELETE
#define USE_INTERNAL_LOCKS
#define CACHE_FREED_OS_ALLOCS

#ifdef USE_INTERNAL_LOCKS
//# define USE_COARSE_GRAIN_LOCKS
#endif

#if defined USE_LOCKFREE_DELETE
#define USE_INTERNAL_LOCKS
#define USE_COARSE_GRAIN_LOCKS
#endif

#if defined CACHE_FREED_OS_ALLOCS
  #define MAX_CACHED_OS_FREES  (64)
  #if FUN_64_BIT
    #define MAX_CACHED_OS_FREES_BYTE_LIMIT  (64*1024*1024)
  #else
    #define MAX_CACHED_OS_FREES_BYTE_LIMIT  (16*1024*1024)
  #endif
#endif

#if defined USE_INTERNAL_LOCKS && !defined USE_COARSE_GRAIN_LOCKS
#   define USE_FINE_GRAIN_LOCKS
#endif

#if FUN_64_BIT
typedef int64 BINNED_STAT_TYPE;
#else
typedef int32 BINNED_STAT_TYPE;
#endif

// when modifying the global allocator stats, if we are using COARSE locks, then all callsites for stat modification are covered by the allocator-wide access guard. Thus the stats can be modified directly.
// If we are using FINE locks, then we must modify the stats through atomics as the locks are either not actually covering the stat callsites, or are locking specific table locks which is not sufficient for stats.
#if STATS
#   if USE_COARSE_GRAIN_LOCKS
#       define BINNED_STAT BINNED_STAT_TYPE
#       define BINNED_INCREMENT_STATCOUNTER(counter)        (++(counter))
#       define BINNED_DECREMENT_STATCOUNTER(counter)        (--(counter))
#       define BINNED_ADD_STATCOUNTER(counter, value)       ((counter) += (value))
#       define BINNED_PEAK_STATCOUNTER(PeakCounter, CompareVal)   ((PeakCounter) = MathBase::Max((PeakCounter), (CompareVal)))
#   else
#       define BINNED_STAT volatile BINNED_STAT_TYPE
#       define BINNED_INCREMENT_STATCOUNTER(counter)  (Atomics::Increment(&(counter)))
#       define BINNED_DECREMENT_STATCOUNTER(counter)  (Atomics::Decrement(&(counter)))
#       define BINNED_ADD_STATCOUNTER(counter, value) (Atomics::Add(&counter, (value)))
#       define BINNED_PEAK_STATCOUNTER(PeakCounter, CompareVal) \
      { \
        BINNED_STAT_TYPE NewCompare; \
        BINNED_STAT_TYPE NewPeak; \
        do \
        { \
          NewCompare = (PeakCounter); \
          NewPeak = MathBase::Max((PeakCounter), (CompareVal)); \
        } \
        while (Atomics::CompareExchange(&(PeakCounter), NewPeak, NewCompare) != NewCompare); \
      }
#   endif
#else
#   define BINNED_STAT BINNED_STAT_TYPE
#   define BINNED_INCREMENT_STATCOUNTER(counter)
#   define BINNED_DECREMENT_STATCOUNTER(counter)
#   define BINNED_ADD_STATCOUNTER(counter, value)
#   define BINNED_PEAK_STATCOUNTER(PeakCounter, CompareVal)
#endif

/** Malloc binned allocator specific stats. */
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Os Current"), STAT_Binned_OsCurrent, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Os Peak"), STAT_Binned_OsPeak, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Waste Current"), STAT_Binned_WasteCurrent, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Waste Peak"), STAT_Binned_WastePeak, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Used Current"), STAT_Binned_UsedCurrent, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Used Peak"), STAT_Binned_UsedPeak, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Binned Current Allocs"), STAT_Binned_CurrentAllocs, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Binned Total Allocs"), STAT_Binned_TotalAllocs, STATGROUP_MemoryAllocator, FUN_BASE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Slack Current"), STAT_Binned_SlackCurrent, STATGROUP_MemoryAllocator, FUN_BASE_API);


/**
 * Optimized virtual memory allocator.
 */
class MemoryAllocatorBinned : public MemoryAllocator
{
  struct Private;

 private:
  // Counts.
  enum { POOL_COUNT = 42 };

  /** Maximum allocation for the pooled allocator */
  enum { EXTENDED_PAGE_POOL_ALLOCATION_COUNT = 2 };
  enum { MAX_POOLED_ALLOCATION_SIZE = 32768 + 1 };

  // Forward declares.
  struct CFreeMem;
  struct CPoolTable;
  struct CPoolInfo;
  struct CPoolHashBucket;

#ifdef CACHE_FREED_OS_ALLOCS
  struct CFreePageBlock
  {
    void* ptr;
    size_t byte_size;

    CFreePageBlock()
      : ptr(nullptr)
      , byte_size(0)
    {}
  };
#endif

  /** pool table. */
  struct CPoolTable
  {
    CPoolInfo* first_pool;
    CPoolInfo* exhausted_pool;
    uint32 block_size;
#ifdef USE_FINE_GRAIN_LOCKS
    CCriticalSection mutex;
#endif
#if STATS
    /** Number of currently active pools */
    uint32 active_pool_count;

    /** Largest number of pools simultaneously active */
    uint32 max_active_pools;

    /** Number of requests currently active */
    uint32 active_request_count;

    /** High watermark of requests simultaneously active */
    uint32 max_active_requests;

    /** Minimum request size (in bytes) */
    uint32 min_request;

    /** Maximum request size (in bytes) */
    uint32 max_request;

    /** Total number of requests ever */
    uint64 total_request_count;

    /** Total waste from all allocs in this table */
    uint64 total_waste;
#endif

    CPoolTable()
      : first_pool(nullptr)
      , exhausted_pool(nullptr)
      , block_size(0)
#if STATS
      , active_pool_count(0)
      , max_active_pools(0)
      , active_request_count(0)
      , max_active_requests(0)
      , min_request(0)
      , max_request(0)
      , total_request_count(0)
      , total_waste(0)
#endif
    {}
  };

  uint64 table_address_limit_;

#ifdef USE_LOCKFREE_DELETE
  /**
   * We can't call the constructor to TLockFreePointerList in the BinnedMalloc constructor
   * as it attempts to allocate memory. We push this back and initialize it later but we
   * set aside the memory before hand
   */
  uint8 pending_free_list_memory_[sizeof(TLockFreePointerList<void>)];
  TLockFreePointerList<void>* pending_free_list_;
  TArray<void*> flushed_frees_;
  bool flushing_frees_;
  bool done_flush_list_init_;
#endif

  CCriticalSection access_mutex_;

  // PAGE_SIZE dependent constants
  uint64 max_hash_buckets_;
  uint64 max_hash_bucket_bits_;
  uint64 max_hash_bucket_waste_;
  uint64 max_book_keeping_overhead_;
  /** Shift to get the reference from the indirect tables */
  uint64 pool_bit_shift_;
  uint64 indect_pool_block_shift_;
  uint64 indect_pool_block_size_;
  /** Shift required to get required hash table key. */
  uint64 hash_key_shift_;
  /** Used to mask off the bits that have been used to lookup the indirect table */
  uint64 pool_mask_;
  uint64 binned_size_limit_;
  uint64 binned_os_table_index_;

  // Variables.
  CPoolTable pool_table_[POOL_COUNT];
  CPoolTable os_table_;
  CPoolTable page_pool_table_[EXTENDED_PAGE_POOL_ALLOCATION_COUNT];
  CPoolTable* mem_size_to_pool_table_[MAX_POOLED_ALLOCATION_SIZE + EXTENDED_PAGE_POOL_ALLOCATION_COUNT];

  CPoolHashBucket* hash_buckets_;
  CPoolHashBucket* hash_bucket_free_list_;

  uint32 page_size_;

#ifdef CACHE_FREED_OS_ALLOCS
  CFreePageBlock freed_page_blocks_[MAX_CACHED_OS_FREES];
  uint32 freed_page_block_count_;
  uint32 cached_total_;
#endif

#if STATS
  BINNED_STAT os_current_;
  BINNED_STAT os_peak_;
  BINNED_STAT waste_current_;
  BINNED_STAT waste_peak_;
  BINNED_STAT used_current_;
  BINNED_STAT used_peak_;
  BINNED_STAT current_alloc_count_;
  BINNED_STAT total_alloc_count_;
  /** os_current_ - waste_current_ - used_current_. */
  BINNED_STAT slack_current_;
  double mem_time_;
#endif

 public:
  // MemoryAllocator interface.

  // page_size - First parameter is page size, all allocs from BinnedAllocFromOS() MUST be aligned to this size
  // AddressLimit - Second parameter is estimate of the range of addresses expected to be returns by BinnedAllocFromOS(). Binned
  // Malloc will adjust its internal structures to make lookups for memory allocations O(1) for this range.
  // It is ok to go outside this range, lookups will just be a little slower
  MemoryAllocatorBinned(uint32 page_size, uint64 address_limit);
  virtual ~MemoryAllocatorBinned();

  void InitializeStatsMetadata() override;
  bool IsInternallyThreadSafe() const override;
  void* Malloc(size_t size, uint32 alignment) override;
  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override;
  void Free(void* ptr) override;
  bool GetAllocationSize(void* original, size_t& out_allocation_size) override;
  bool ValidateHeap() override;

  /** Called once per frame, gathers and sets all memory allocator statistics into the corresponding stats. MUST BE THREAD SAFE. */
  void UpdateStats() override;
  void GetAllocatorStats(GenericMemoryStats& out_stats) override;
  void DumpAllocatorStats(class Printer& out) override;
  const char* GetDescriptiveName() override;
};

} // namespace fun
