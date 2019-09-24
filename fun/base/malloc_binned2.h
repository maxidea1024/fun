#pragma once

#include "Templates/AlignmentTemplates.h"
#include "Allocators/CachedOSPageAllocator.h"

namespace fun {

#define BINNED2_MAX_CACHED_OS_FREES  (64)

#if FUN_64_BIT
  #define BINNED2_MAX_CACHED_OS_FREES_BYTE_LIMIT  (64*1024*1024)
#else
  #define BINNED2_MAX_CACHED_OS_FREES_BYTE_LIMIT  (16*1024*1024)
#endif

#define BINNED2_LARGE_ALLOC               65536         // alignment of OS-allocated pointer - pool-allocated pointers will have a non-aligned pointer
#define BINNED2_MINIMUM_ALIGNMENT_SHIFT   4             // alignment of blocks, expressed as a shift
#define BINNED2_MINIMUM_ALIGNMENT         16            // alignment of blocks
#define BINNED2_MAX_SMALL_POOL_SIZE       (32768 - 16)  // Maximum block size in GMallocBinned2SmallBlockSizes
#define BINNED2_SMALL_POOL_COUNT          45

#define DEFAULT_GMallocBinned2PerThreadCaches  1
#define DEFAULT_GMallocBinned2LockFreeCaches  0
#define DEFAULT_GMallocBinned2BundleSize  BINNED2_LARGE_ALLOC
#define DEFAULT_GMallocBinned2BundleCount  64
#define DEFAULT_GMallocBinned2AllocExtra  32
#define BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle  8

#define BINNED2_ALLOW_RUNTIME_TWEAKING  0
#if BINNED2_ALLOW_RUNTIME_TWEAKING
  extern FUN_BASE_API int32 GMallocBinned2PerThreadCaches;
  extern FUN_BASE_API int32 GMallocBinned2BundleSize = DEFAULT_GMallocBinned2BundleSize;
  extern FUN_BASE_API int32 GMallocBinned2BundleCount = DEFAULT_GMallocBinned2BundleCount;
  extern FUN_BASE_API int32 GMallocBinned2MaxBundlesBeforeRecycle = BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle;
  extern FUN_BASE_API int32 GMallocBinned2AllocExtra = DEFAULT_GMallocBinned2AllocExtra;
#else
  #define GMallocBinned2PerThreadCaches         DEFAULT_GMallocBinned2PerThreadCaches
  #define GMallocBinned2BundleSize              DEFAULT_GMallocBinned2BundleSize
  #define GMallocBinned2BundleCount             DEFAULT_GMallocBinned2BundleCount
  #define GMallocBinned2MaxBundlesBeforeRecycle BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle
  #define GMallocBinned2AllocExtra              DEFAULT_GMallocBinned2AllocExtra
#endif

/**
 * Optimized virtual memory allocator.
 */
class FUN_BASE_API MemoryAllocatorBinned2 final : public MemoryAllocator {
  struct Private;

  // Forward declares.
  struct CPoolInfo;
  struct PoolHashBucket;

  /** Information about a piece of free memory. */
  struct CFreeBlock
  {
    enum
    {
      CANARY_VALUE = 0xe3
    };

    inline CFreeBlock(uint32 InPageSize, uint32 block_size, uint32 pool_index)
      : BlockSize(block_size)
      , pool_index(pool_index)
      , canary(CANARY_VALUE)
      , NextFreeBlock(nullptr)
    {
      fun_check(pool_index < uint8_MAX && block_size <= uint16_MAX);
      free_block_count = InPageSize / block_size;
      if (free_block_count * block_size + BINNED2_MINIMUM_ALIGNMENT > InPageSize)
      {
        free_block_count--;
      }
      fun_check(free_block_count * block_size + BINNED2_MINIMUM_ALIGNMENT <= InPageSize);
    }

    inline uint32 GetNumFreeRegularBlocks() const
    {
      return free_block_count;
    }

    inline bool IsCanaryOk() const
    {
      return canary == CFreeBlock::CANARY_VALUE;
    }

    inline void CanaryTest() const
    {
      if (!IsCanaryOk())
      {
        CanaryFail();
      }
      //CHECK_SLOW(pool_index == BoundSizeToPoolIndex(BlockSize));
    }
    void CanaryFail() const;

    inline void* AllocateRegularBlock()
    {
      --free_block_count;
      if (IsAligned(this, BINNED2_LARGE_ALLOC))
      {
        return (uint8*)this + BINNED2_LARGE_ALLOC - (free_block_count + 1) * BlockSize;
      }
      return (uint8*)this + (free_block_count)* BlockSize;
    }

    uint16 BlockSize; // size of the blocks that this list points to
    uint8 pool_index; // index of this pool
    uint8 canary; // Constant value of 0xe3
    uint32 free_block_count; // Number of consecutive free blocks here, at least 1.
    void* NextFreeBlock; // Next free block in another pool
  };

  struct CPoolList
  {
    CPoolList();

    bool IsEmpty() const;

    CPoolInfo& GetFrontPool();
    const CPoolInfo& GetFrontPool() const;

    void LinkToFront(CPoolInfo* pool);

    CPoolInfo& PushNewPoolToFront(MemoryAllocatorBinned2& Allocator, uint32 InBytes, uint32 pool_index);

    void ValidateActivePools();
    void ValidateExhaustedPools();

   private:
    CPoolInfo* Front;
  };

  /** pool table. */
  struct CPoolTable
  {
    CPoolList ActivePools;
    CPoolList ExhaustedPools;
    uint32 BlockSize;

    CPoolTable();
  };

  struct CPtrToPoolMapping
  {
    CPtrToPoolMapping()
      : PtrToPoolPageBitShift(0)
      , hash_key_shift_(0)
      , pool_mask_(0)
      , max_hash_buckets_(0)
    {}

    explicit CPtrToPoolMapping(uint32 InPageSize, uint64 InNumPoolsPerPage, uint64 AddressLimit)
    {
      Init(InPageSize, InNumPoolsPerPage, AddressLimit);
    }

    void Init(uint32 InPageSize, uint64 InNumPoolsPerPage, uint64 AddressLimit)
    {
      const uint64 PoolPageToPoolBitShift = CPlatformMath::CeilLogTwo(InNumPoolsPerPage);

      PtrToPoolPageBitShift = CPlatformMath::CeilLogTwo(InPageSize);
      hash_key_shift_ = PtrToPoolPageBitShift + PoolPageToPoolBitShift;
      pool_mask_ = (1ull << PoolPageToPoolBitShift) - 1;
      max_hash_buckets_ = AddressLimit >> hash_key_shift_;
    }

    inline void GetHashBucketAndPoolIndices(const void* ptr, uint32& OutBucketIndex, uintptr_t& OutBucketCollision, uint32& OutPoolIndex) const
    {
      OutBucketCollision = (uintptr_t)ptr >> hash_key_shift_;
      OutBucketIndex = uint32(OutBucketCollision & (max_hash_buckets_ - 1));
      OutPoolIndex = ((uintptr_t)ptr >> PtrToPoolPageBitShift) & pool_mask_;
    }

    inline uint64 GetMaxHashBuckets() const
    {
      return max_hash_buckets_;
    }

   private:
    /** Shift to apply to a pointer to get the reference from the indirect tables */
    uint64 PtrToPoolPageBitShift;

    /** Shift required to get required hash table key. */
    uint64 hash_key_shift_;

    /** Used to mask off the bits that have been used to lookup the indirect table */
    uint64 pool_mask_;

    // PAGE_SIZE dependent constants
    uint64 max_hash_buckets_;
  };

  CPtrToPoolMapping PtrToPoolMapping;

  // pool tables for different pool sizes
  CPoolTable SmallPoolTables[BINNED2_SMALL_POOL_COUNT];

  PoolHashBucket* hash_buckets_;
  PoolHashBucket* hash_bucket_free_list_;
  uint64 NumPoolsPerPage;

  TCachedOSPageAllocator<BINNED2_MAX_CACHED_OS_FREES, BINNED2_MAX_CACHED_OS_FREES_BYTE_LIMIT> CachedOSPageAllocator;

  CCriticalSection Mutex;

  inline static bool IsOSAllocation(const void* ptr)
  {
    return IsAligned(ptr, BINNED2_LARGE_ALLOC);
  }

  struct CBundleNode
  {
    CBundleNode* NextNodeInCurrentBundle;
    union
    {
      CBundleNode* next_bundle;
      int32 Count;
    };
  };

  struct CBundle
  {
    inline CBundle()
    {
      Reset();
    }

    inline void Reset()
    {
      Head = nullptr;
      Count = 0;
    }

    inline void PushHead(CBundleNode* node)
    {
      node->next_node_in_current_bundle = Head;
      node->next_bundle = nullptr;
      Head = node;
      Count++;
    }

    inline CBundleNode* PopHead()
    {
      CBundleNode* result = Head;

      Count--;
      Head = Head->next_node_in_current_bundle;
      return result;
    }

    CBundleNode* Head;
    uint32 Count;
  };
  static_assert(sizeof(CBundleNode) <= BINNED2_MINIMUM_ALIGNMENT, "bundle nodes must fit into the smallest block size");

  struct CFreeBlockList
  {
    // return true if we actually pushed it
    inline bool PushToFront(void* ptr, uint32 pool_index, uint32 block_size)
    {
      CHECK_SLOW(ptr);

      if (PartialBundle.Count >= (uint32)GMallocBinned2BundleCount || PartialBundle.Count * block_size >= (uint32)GMallocBinned2BundleSize)
      {
        if (FullBundle.Head)
        {
          return false;
        }
        FullBundle = PartialBundle;
        PartialBundle.Reset();
      }
      PartialBundle.PushHead((CBundleNode*)ptr);
      return true;
    }

    inline bool CanPushToFront(uint32 pool_index, uint32 block_size)
    {
      if (FullBundle.Head && (PartialBundle.Count >= (uint32)GMallocBinned2BundleCount || PartialBundle.Count * block_size >= (uint32)GMallocBinned2BundleSize))
      {
        return false;
      }
      return true;
    }

    inline void* PopFromFront(uint32 pool_index)
    {
      if (!PartialBundle.Head)
      {
        if (FullBundle.Head)
        {
          PartialBundle = FullBundle;
          FullBundle.Reset();
        }
      }
      return PartialBundle.Head ? PartialBundle.PopHead() : nullptr;
    }

    // tries to recycle the full bundle, if that fails, it is returned for freeing
    CBundleNode* RecyleFull(uint32 pool_index);
    bool ObtainPartial(uint32 pool_index);
    CBundleNode* PopBundles(uint32 pool_index);

   private:
    CBundle PartialBundle;
    CBundle FullBundle;
  };

  struct CPerThreadFreeBlockLists
  {
    inline static CPerThreadFreeBlockLists* Get()
    {
      return MemoryAllocatorBinned2::Binned2TlsSlot ? (CPerThreadFreeBlockLists*)CPlatformTLS::GetTlsValue(MemoryAllocatorBinned2::Binned2TlsSlot) : nullptr;
    }

    static void SetTLS();

    static void ClearTLS();

    inline void* Malloc(uint32 pool_index)
    {
      return FreeLists[pool_index].PopFromFront(pool_index);
    }

    // return true if the pointer was pushed
    inline bool Free(void* ptr, uint32 pool_index, uint32 block_size)
    {
      return FreeLists[pool_index].PushToFront(ptr, pool_index, block_size);
    }

    // return true if a pointer can be pushed
    inline bool CanFree(uint32 pool_index, uint32 block_size)
    {
      return FreeLists[pool_index].CanPushToFront(pool_index, block_size);
    }

    // returns a bundle that needs to be freed if it can't be recycled
    CBundleNode* RecycleFullBundle(uint32 pool_index)
    {
      return FreeLists[pool_index].RecyleFull(pool_index);
    }

    // returns true if we have anything to pop
    bool ObtainRecycledPartial(uint32 pool_index)
    {
      return FreeLists[pool_index].ObtainPartial(pool_index);
    }

    CBundleNode* PopBundles(uint32 pool_index)
    {
      return FreeLists[pool_index].PopBundles(pool_index);
    }

   private:
    CFreeBlockList FreeLists[BINNED2_SMALL_POOL_COUNT];
  };

  static inline CFreeBlock* GetPoolHeaderFromPointer(void* ptr)
  {
    return (CFreeBlock*)AlignDown(ptr, BINNED2_LARGE_ALLOC);
  }

 public:
  MemoryAllocatorBinned2();
  virtual ~MemoryAllocatorBinned2();

  // MemoryAllocator interface.
  bool IsInternallyThreadSafe() const override;

  void* Malloc(size_t size, uint32 alignment) override
  {
    // Only allocate from the small pools if the size is small enough and the alignment isn't crazy large.
    // With large alignments, we'll waste a lot of memory allocating an entire page, but such alignments are highly unlikely in practice.
    if ((size <= BINNED2_MAX_SMALL_POOL_SIZE) & (alignment <= BINNED2_MINIMUM_ALIGNMENT)) { // one branch, not two
      CPerThreadFreeBlockLists* Lists = GMallocBinned2PerThreadCaches ? CPerThreadFreeBlockLists::Get() : nullptr;
      if (Lists != nullptr) {
        if (void* result = Lists->Malloc(BoundSizeToPoolIndex(size))) {
          return result;
        }
      }
    }
    return MallocExternal(size, alignment);
  }

  inline void* Realloc(void* ptr, size_t new_size, uint32 alignment) override
  {
    if (new_size <= BINNED2_MAX_SMALL_POOL_SIZE && alignment <= BINNED2_MINIMUM_ALIGNMENT) { // one branch, not two
      CPerThreadFreeBlockLists* Lists = GMallocBinned2PerThreadCaches ? CPerThreadFreeBlockLists::Get() : nullptr;
      if (Lists && (!ptr || !IsOSAllocation(ptr))) {
        uint32 BlockSize = 0;
        uint32 pool_index = 0;

        bool bCanFree = true; // the nullptr is always "freeable"
        if (ptr != nullptr) {
          // Reallocate to a smaller/bigger pool if necessary
          CFreeBlock* Free = GetPoolHeaderFromPointer(ptr);
          BlockSize = Free->block_size;
          pool_index = Free->pool_index;
          bCanFree = Free->IsCanaryOk();
          if (new_size && bCanFree && new_size <= BlockSize && (pool_index == 0 || new_size > PoolIndexToBlockSize(pool_index - 1))) {
            return ptr;
          }
          bCanFree = bCanFree && Lists->CanFree(pool_index, BlockSize);
        }
        if (bCanFree) {
          void* result = new_size ? Lists->Malloc(BoundSizeToPoolIndex(new_size)) : nullptr;
          if (result != nullptr || new_size == 0) {
            if (result != nullptr && ptr != nullptr) {
              UnsafeMemory::Memcpy(result, ptr, MathBase::Min<size_t>(new_size, BlockSize));
            }
            if (ptr != nullptr) {
              bool bDidPush = Lists->Free(ptr, pool_index, BlockSize);
              CHECK_SLOW(bDidPush);
            }
            return result;
          }
        }
      }
    }
    return ReallocExternal(ptr, new_size, alignment);
  }

  inline void Free(void* ptr) override
  {
    if (!IsOSAllocation(ptr)) {
      CPerThreadFreeBlockLists* Lists = GMallocBinned2PerThreadCaches ? CPerThreadFreeBlockLists::Get() : nullptr;
      if (Lists != nullptr) {
        CFreeBlock* base_ptr = GetPoolHeaderFromPointer(ptr);
        if (base_ptr->IsCanaryOk() && Lists->Free(ptr, base_ptr->pool_index, base_ptr->block_size)) {
          return;
        }
      }
    }
    FreeExternal(ptr);
  }

  inline bool GetAllocationSize(void *ptr, size_t &out_allocation_size) override
  {
    if (!IsOSAllocation(ptr)) {
      const CFreeBlock* Free = GetPoolHeaderFromPointer(ptr);
      if (Free->IsCanaryOk()) {
        out_allocation_size = Free->block_size;
        return true;
      }
    }
    return GetAllocationSizeExternal(ptr, out_allocation_size);
  }

  inline size_t QuantizeSize(size_t Count, uint32 alignment) override
  {
    static_assert(DEFAULT_ALIGNMENT <= BINNED2_MINIMUM_ALIGNMENT, "DEFAULT_ALIGNMENT is assumed to be zero"); // used below
    CHECK_SLOW(MathBase::IsPowerOfTwo(alignment));
    size_t out_allocation_size;
    if ((Count <= BINNED2_MAX_SMALL_POOL_SIZE) & (alignment <= BINNED2_MINIMUM_ALIGNMENT)) { // one branch, not two
      out_allocation_size = PoolIndexToBlockSize(BoundSizeToPoolIndex(Count));
    }
    else {
      alignment = MathBase::Max<uint32>(alignment, OsAllocationGranularity);
      CHECK_SLOW(alignment <= PAGE_SIZE);
      out_allocation_size = Align(Count, alignment);
    }
    fun_check(out_allocation_size >= Count);
    return out_allocation_size;
  }

  bool ValidateHeap() override;
  void Trim() override;
  void SetupTlsCachesOnCurrentThread() override;
  void ClearAndDisableTlsCachesOnCurrentThread() override;
  const char* GetDescriptiveName() override;
  // end of MemoryAllocator interface

  void FlushCurrentThreadCache();
  void* MallocExternal(size_t size, uint32 alignment);
  void* ReallocExternal(void* ptr, size_t new_size, uint32 alignment);
  void FreeExternal(void *ptr);
  bool GetAllocationSizeExternal(void* ptr, size_t& out_allocation_size);

  static uint16 SmallBlockSizesReversed[BINNED2_SMALL_POOL_COUNT]; // this is reversed to get the smallest elements on our main cache line
  static MemoryAllocatorBinned2* MemoryAllocatorBinned2;
  static uint32 Binned2TlsSlot;
  static uint32 PAGE_SIZE;
  static uint32 OsAllocationGranularity;
  // Mapping of sizes to small table indices
  static uint8 MemSizeToIndex[1 + (BINNED2_MAX_SMALL_POOL_SIZE >> BINNED2_MINIMUM_ALIGNMENT_SHIFT)];

  inline uint32 BoundSizeToPoolIndex(size_t size)
  {
    auto index = ((size + BINNED2_MINIMUM_ALIGNMENT - 1) >> BINNED2_MINIMUM_ALIGNMENT_SHIFT);
    CHECK_SLOW(index >= 0 && index <= (BINNED2_MAX_SMALL_POOL_SIZE >> BINNED2_MINIMUM_ALIGNMENT_SHIFT)); // and it should be in the table
    uint32 pool_index = uint32(MemSizeToIndex[index]);
    CHECK_SLOW(pool_index >= 0 && pool_index < BINNED2_SMALL_POOL_COUNT);
    return pool_index;
  }

  inline uint32 PoolIndexToBlockSize(uint32 pool_index)
  {
    return SmallBlockSizesReversed[BINNED2_SMALL_POOL_COUNT - pool_index - 1];
  }
};


#define BINNED2_INLINE  (1)
#if BINNED2_INLINE // during development, it helps with iteration time to not include these here, but rather in the .cpp
  #if PLATFORM_USES_FIXED_GMalloc_CLASS && !FUN_FORCE_ANSI_ALLOCATOR && USE_MALLOC_BINNED2
    #define CMEMORY_INLINE_FUNCTION_DECORATOR  inline
    #define CMEMORY_INLINE_GMalloc  (MemoryAllocatorBinned2::MemoryAllocatorBinned2)
    #include "Memory_inline.h"
  #endif
#endif

} // namespace fun
