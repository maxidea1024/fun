#include "fun/base/malloc_binned2.h"
#include "HAL/MemoryMisc.h"
//#include "HAL/PlatformAtomics.h"

// TODO?
//#include "Async/TaskGraphInterfaces.h"

namespace fun {

#if BINNED2_ALLOW_RUNTIME_TWEAKING

int32 GMallocBinned2PerThreadCaches = DEFAULT_GMallocBinned2PerThreadCaches;
static CAutoConsoleVariableRef GMallocBinned2PerThreadCachesCVar(
    TEXT("MemoryAllocatorBinned2.PerThreadCaches"),
    GMallocBinned2PerThreadCaches,
    TEXT("Enables per-thread caches of small (<= 32768 byte) allocations from "
         "MemoryAllocatorBinned2"));

int32 GMallocBinned2BundleSize = DEFAULT_GMallocBinned2BundleSize;
static CAutoConsoleVariableRef GMallocBinned2BundleSizeCVar(
    TEXT("MemoryAllocatorBinned2.BundleSize"), GMallocBinned2BundleSize,
    TEXT("Max size in bytes of per-block bundles used in the recycling "
         "process"));

int32 GMallocBinned2BundleCount = DEFAULT_GMallocBinned2BundleCount;
static CAutoConsoleVariableRef GMallocBinned2BundleCountCVar(
    TEXT("MemoryAllocatorBinned2.BundleCount"), GMallocBinned2BundleCount,
    TEXT(
        "Max count in blocks per-block bundles used in the recycling process"));

int32 GMallocBinned2MaxBundlesBeforeRecycle =
    BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle;
static CAutoConsoleVariableRef GMallocBinned2MaxBundlesBeforeRecycleCVar(
    TEXT("MemoryAllocatorBinned2.BundleRecycleCount"),
    GMallocBinned2MaxBundlesBeforeRecycle,
    TEXT("Number of freed bundles in the global recycler before it returns "
         "them to the system, per-block size. Limited by "
         "BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle (currently 4)"));

int32 GMallocBinned2AllocExtra = DEFAULT_GMallocBinned2AllocExtra;
static CAutoConsoleVariableRef GMallocBinned2AllocExtraCVar(
    TEXT("MemoryAllocatorBinned2.AllocExtra"), GMallocBinned2AllocExtra,
    TEXT("When we do acquire the lock, how many blocks cached in TLS caches. "
         "In no case will we grab more than a page."));

#endif  // BINNED2_ALLOW_RUNTIME_TWEAKING

// Block sizes are based around getting the maximum amount of allocations per
// pool, with as little alignment waste as possible. Block sizes should be close
// to even divisors of the system page size, and well distributed. They must be
// 16-byte aligned as well.
static uint16 SmallBlockSizes[] = {
    16,    32,    48,         64,    80,        96,   112,       128,
    160,   192,   224,        256,   288,       320,  384,       448,
    512,   576,   640,        704,   768,       896,  1024 - 16, 1168,
    1360,  1632,  2048 - 16,  2336,  2720,      3264, 4096 - 16, 4368,
    4672,  5040,  5456,       5952,  6544 - 16, 7280, 8192 - 16, 9360,
    10912, 13104, 16384 - 16, 21840, 32768 - 16};

MS_ALIGN(PLATFORM_CACHE_LINE_SIZE)
static uint8 UnusedAlignPadding[PLATFORM_CACHE_LINE_SIZE] GCC_ALIGN(
    PLATFORM_CACHE_LINE_SIZE) = {0};
uint16
    MemoryAllocatorBinned2::SmallBlockSizesReversed[BINNED2_SMALL_POOL_COUNT] =
        {0};
uint32 MemoryAllocatorBinned2::Binned2TlsSlot = 0;
uint32 MemoryAllocatorBinned2::OsAllocationGranularity = 0;
uint32 MemoryAllocatorBinned2::PAGE_SIZE = 0;
MemoryAllocatorBinned2* MemoryAllocatorBinned2::MemoryAllocatorBinned2 =
    nullptr;
// Mapping of sizes to small table indices
uint8 MemoryAllocatorBinned2::MemSizeToIndex
    [1 + (BINNED2_MAX_SMALL_POOL_SIZE >> BINNED2_MINIMUM_ALIGNMENT_SHIFT)] = {
        0};

MemoryAllocatorBinned2::CPoolList::CPoolList() : front(nullptr) {}

MemoryAllocatorBinned2::CPoolTable::CPoolTable() : block_size(0) {}

struct MemoryAllocatorBinned2::CPoolInfo {
  enum class ECanary : uint16 {
    Unassigned = 0x3941,
    FirstFreeBlockIsOSAllocSize = 0x17ea,
    FirstFreeBlockIsPtr = 0xf317
  };

  uint16 taken;  // Number of allocated elements in this pool, when counts down
                 // to zero can free the entire pool
  ECanary canary;                // See ECanary
  uint32 alloc_size;             // Number of bytes allocated
  CFreeBlock* first_free_block;  // Pointer to first free memory in this pool or
                                 // the OS Allocation size in bytes if this
                                 // allocation is not binned
  CPoolInfo* next;               // Pointer to next pool
  CPoolInfo**
      ptr_to_prev_next;  // Pointer to whichever pointer points to this pool

 public:
  CPoolInfo()
      : taken(0),
        canary(ECanary::Unassigned),
        alloc_size(0),
        first_free_block(nullptr),
        next(nullptr),
        ptr_to_prev_next(nullptr) {}

  void CheckCanary(ECanary should_be) const {
    if (canary != should_be) {
      fun_log(LogMemory, Fatal,
              TEXT("MemoryAllocatorBinned2 Corruption canary was 0x%x, should "
                   "be 0x%x"),
              int32(canary), int32(should_be));
    }
  }

  void SetCanary(ECanary should_be, bool pre_existing, bool bGuarnteedToBeNew) {
    if (pre_existing) {
      if (bGuarnteedToBeNew) {
        fun_log(LogMemory, Fatal,
                TEXT("MemoryAllocatorBinned2 Corruption canary was 0x%x, "
                     "should be 0x%x. This block is both preexisting and "
                     "guaranteed to be new; which makes no sense."),
                int32(canary), int32(should_be));
      }

      if (should_be == ECanary::Unassigned) {
        if (canary != ECanary::FirstFreeBlockIsOSAllocSize &&
            canary != ECanary::FirstFreeBlockIsPtr) {
          fun_log(
              LogMemory, Fatal,
              TEXT("MemoryAllocatorBinned2 Corruption canary was 0x%x, will be "
                   "0x%x because this block should be preexisting and in use."),
              int32(canary), int32(should_be));
        }
      } else if (canary != should_be) {
        fun_log(
            LogMemory, Fatal,
            TEXT("MemoryAllocatorBinned2 Corruption canary was 0x%x, should be "
                 "0x%x because this block should be preexisting."),
            int32(canary), int32(should_be));
      }
    } else {
      if (bGuarnteedToBeNew) {
        if (canary != ECanary::Unassigned) {
          fun_log(LogMemory, Fatal,
                  TEXT("MemoryAllocatorBinned2 Corruption canary was 0x%x, "
                       "will be 0x%x. This block is guaranteed to be new yet "
                       "is it already assigned."),
                  int32(canary), int32(should_be));
        }
      } else if (canary != should_be && canary != ECanary::Unassigned) {
        fun_log(LogMemory, Fatal,
                TEXT("MemoryAllocatorBinned2 Corruption canary was 0x%x, will "
                     "be 0x%x does not have an expected value."),
                int32(canary), int32(should_be));
      }
    }
    canary = should_be;
  }

  bool HasFreeRegularBlock() const {
    CheckCanary(ECanary::FirstFreeBlockIsPtr);
    return first_free_block && first_free_block->GetNumFreeRegularBlocks() != 0;
  }

  void* AllocateRegularBlock() {
    fun_check(HasFreeRegularBlock());
    ++taken;
    void* result = first_free_block->AllocateRegularBlock();
    ExhaustPoolIfNecessary();
    return result;
  }

  uint32 GetOSRequestedBytes() const { return alloc_size; }

  uintptr_t GetOsAllocatedBytes() const {
    CheckCanary(ECanary::FirstFreeBlockIsOSAllocSize);
    return (uintptr_t)first_free_block;
  }

  void SetOSAllocationSizes(uint32 InRequestedBytes,
                            uintptr_t InAllocatedBytes) {
    CheckCanary(ECanary::FirstFreeBlockIsOSAllocSize);
    fun_check(InRequestedBytes !=
              0);  // Shouldn't be pooling zero byte allocations
    fun_check(InAllocatedBytes >=
              InRequestedBytes);  // We must be allocating at least as much as
                                  // we requested

    alloc_size = InRequestedBytes;
    first_free_block = (CFreeBlock*)InAllocatedBytes;
  }

  void Link(CPoolInfo*& prev_next) {
    if (prev_next) {
      prev_next->ptr_to_prev_next = &next;
    }
    next = prev_next;
    ptr_to_prev_next = &prev_next;
    prev_next = this;
  }

  void Unlink() {
    if (next) {
      next->ptr_to_prev_next = ptr_to_prev_next;
    }
    *ptr_to_prev_next = next;
  }

 private:
  void ExhaustPoolIfNecessary() {
    if (first_free_block->GetNumFreeRegularBlocks() ==
        0) {  // && (first_free_block->GetShortBlockSize() == 0 ||
              // !first_free_block->HasFreeShortBlock()))
      first_free_block = (CFreeBlock*)first_free_block->nextFreeBlock;
    }
    fun_check(!first_free_block ||
              first_free_block->GetNumFreeRegularBlocks() != 0);
  }
};

/** Hash table struct for retrieving allocation book keeping information */
struct MemoryAllocatorBinned2::PoolHashBucket {
  uintptr_t bucket_index;
  CPoolInfo* first_pool;
  PoolHashBucket* prev;
  PoolHashBucket* next;

  PoolHashBucket() {
    bucket_index = 0;
    first_pool = nullptr;
    prev = this;
    next = this;
  }

  void Link(PoolHashBucket* after) {
    after->prev = prev;
    after->next = this;
    prev->next = after;
    this->prev = after;
  }

  void Unlink() {
    next->prev = prev;
    prev->next = next;
    prev = this;
    next = this;
  }
};

struct MemoryAllocatorBinned2::Private {
  // Implementation.
  static CA_NO_RETURN void OutOCMemory(uint64 size, uint32 alignment = 0) {
    // this is expected not to return
    PlatformMemory::OnOutOfMemory(size, alignment);
  }

  /**
   * Gets the CPoolInfo for a memory address. If no valid info exists one is
   * created.
   */
  static CPoolInfo* GetOrCreatePoolInfo(
      MemoryAllocatorBinned2& allocator, void* ptr,
      MemoryAllocatorBinned2::CPoolInfo::ECanary Kind, bool pre_existing) {
    /**
    Creates an array of CPoolInfo structures for tracking allocations.
    */
    auto CreatePoolArray = [](uint64 NumPools) {
      uint64 PoolArraySize = NumPools * sizeof(CPoolInfo);

      void* result = PlatformMemory::BinnedAllocFromOS(PoolArraySize);
      if (!result) {
        OutOCMemory(PoolArraySize);
      }

      DefaultConstructItems<CPoolInfo>(result, NumPools);
      return (CPoolInfo*)result;
    };

    uint32 bucket_index;
    uintptr_t bucket_index_collision;
    uint32 pool_index;
    allocator.PtrToPoolMapping.GetHashBucketAndPoolIndices(
        ptr, bucket_index, bucket_index_collision, pool_index);

    PoolHashBucket* first_bucket = &allocator.hash_buckets_[bucket_index];
    PoolHashBucket* collision = first_bucket;
    do {
      if (!collision->first_pool) {
        collision->bucket_index = bucket_index_collision;
        collision->first_pool = CreatePoolArray(allocator.NumPoolsPerPage);
        collision->first_pool[pool_index].SetCanary(Kind, pre_existing, true);
        return &collision->first_pool[pool_index];
      }

      if (collision->bucket_index == bucket_index_collision) {
        collision->first_pool[pool_index].SetCanary(Kind, pre_existing, false);
        return &collision->first_pool[pool_index];
      }

      collision = collision->next;
    } while (collision != first_bucket);

    // Create a new hash bucket entry
    if (!allocator.hash_bucket_free_list_) {
      allocator.hash_bucket_free_list_ =
          (PoolHashBucket*)PlatformMemory::BinnedAllocFromOS(
              MemoryAllocatorBinned2::PAGE_SIZE);

      for (uintptr_t i = 0, n = PAGE_SIZE / sizeof(PoolHashBucket); i < n;
           ++i) {
        allocator.hash_bucket_free_list_->Link(
            new (allocator.hash_bucket_free_list_ + i) PoolHashBucket());
      }
    }

    PoolHashBucket* next_free = allocator.hash_bucket_free_list_->next;
    PoolHashBucket* new_bucket = allocator.hash_bucket_free_list_;

    new_bucket->Unlink();

    if (next_free == new_bucket) {
      next_free = nullptr;
    }
    allocator.hash_bucket_free_list_ = next_free;

    if (!new_bucket->first_pool) {
      new_bucket->first_pool = CreatePoolArray(allocator.NumPoolsPerPage);
      new_bucket->first_pool[pool_index].SetCanary(Kind, pre_existing, true);
    } else {
      new_bucket->first_pool[pool_index].SetCanary(Kind, pre_existing, false);
    }

    new_bucket->bucket_index = bucket_index_collision;

    first_bucket->Link(new_bucket);

    return &new_bucket->first_pool[pool_index];
  }

  static CPoolInfo* FindPoolInfo(MemoryAllocatorBinned2& allocator, void* ptr) {
    uint32 bucket_index;
    uintptr_t bucket_index_collision;
    uint32 pool_index;
    allocator.PtrToPoolMapping.GetHashBucketAndPoolIndices(
        ptr, bucket_index, bucket_index_collision, pool_index);

    PoolHashBucket* first_bucket = &allocator.hash_buckets_[bucket_index];
    PoolHashBucket* collision = first_bucket;
    do {
      if (collision->bucket_index == bucket_index_collision) {
        return &collision->first_pool[pool_index];
      }

      collision = collision->next;
    } while (collision != first_bucket);

    return nullptr;
  }

  struct CGlobalRecycler {
    bool PushBundle(uint32 pool_index, CBundleNode* bundle) {
      uint32 cahced_bundle_count = MathBase::Min<uint32>(
          GMallocBinned2MaxBundlesBeforeRecycle,
          BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle);
      for (uint32 slot = 0; slot < cahced_bundle_count; slot++) {
        if (!Bundles[pool_index].free_bundles[slot]) {
          if (!Atomics::CompareExchangePointer(
                  (void**)&Bundles[pool_index].free_bundles[slot], bundle,
                  nullptr)) {
            return true;
          }
        }
      }
      return false;
    }

    CBundleNode* PopBundle(uint32 pool_index) {
      uint32 cahced_bundle_count = MathBase::Min<uint32>(
          GMallocBinned2MaxBundlesBeforeRecycle,
          BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle);
      for (uint32 slot = 0; slot < cahced_bundle_count; slot++) {
        CBundleNode* result = Bundles[pool_index].free_bundles[slot];
        if (result) {
          if (Atomics::CompareExchangePointer(
                  (void**)&Bundles[pool_index].free_bundles[slot], nullptr,
                  result) == result) {
            return result;
          }
        }
      }
      return nullptr;
    }

   private:
    struct CPaddedBundlePointer {
      CBundleNode*
          free_bundles[BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle];
#define BUNDLE_PADDING        \
  (PLATFORM_CACHE_LINE_SIZE - \
   sizeof(CBundleNode*) * BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle)
#if (4 + (4 * FUN_64_BIT)) *                                \
        BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle < \
    PLATFORM_CACHE_LINE_SIZE
      uint8 Padding[BUNDLE_PADDING];
#endif
      CPaddedBundlePointer() {
        DefaultConstructItems<CBundleNode*>(
            free_bundles, BINNED2_MAX_GMallocBinned2MaxBundlesBeforeRecycle);
      }
    };
    static_assert(
        sizeof(CPaddedBundlePointer) == PLATFORM_CACHE_LINE_SIZE,
        "CPaddedBundlePointer should be the same size as a cache line");
    MS_ALIGN(PLATFORM_CACHE_LINE_SIZE)
    CPaddedBundlePointer Bundles[BINNED2_SMALL_POOL_COUNT] GCC_ALIGN(
        PLATFORM_CACHE_LINE_SIZE);
  };

  static CGlobalRecycler GGlobalRecycler;

  static void free_bundles(MemoryAllocatorBinned2& allocator,
                           CBundleNode* bundles_to_recycle, uint32 block_size,
                           uint32 pool_index) {
    CPoolTable& table = allocator.SmallPoolTables[pool_index];

    CBundleNode* bundle = bundles_to_recycle;
    while (bundle) {
      CBundleNode* next_bundle = bundle->next_bundle;

      CBundleNode* node = bundle;
      do {
        CBundleNode* next_node = node->next_node_in_current_bundle;
        CPoolInfo* node_pool = FindPoolInfo(allocator, node);
        if (!node_pool) {
          fun_log(LogMemory, Fatal,
                  TEXT("MemoryAllocatorBinned2 Attempt to free an unrecognized "
                       "small block %p"),
                  node);
        }
        node_pool->CheckCanary(CPoolInfo::ECanary::FirstFreeBlockIsPtr);

        // If this pool was exhausted, move to available list.
        if (!node_pool->first_free_block) {
          table.ActivePools.LinkToFront(node_pool);
        }

        // Free a pooled allocation.
        CFreeBlock* free = (CFreeBlock*)node;
        free->free_block_count = 1;
        free->nextFreeBlock = node_pool->first_free_block;
        free->block_size = block_size;
        node_pool->first_free_block = free;

        // Free this pool.
        fun_check(node_pool->taken >= 1);
        if (--node_pool->taken == 0) {
          node_pool->SetCanary(CPoolInfo::ECanary::Unassigned, true, false);
          CFreeBlock* base_ptr_of_node = GetPoolHeaderFromPointer(node);

          // Free the OS memory.
          node_pool->Unlink();
          allocator.CachedOSPageAllocator.Free(base_ptr_of_node,
                                               allocator.PAGE_SIZE);
        }

        node = next_node;
      } while (node);

      bundle = next_bundle;
    }
  }
};

MemoryAllocatorBinned2::Private::CGlobalRecycler
    MemoryAllocatorBinned2::Private::GGlobalRecycler;

inline bool MemoryAllocatorBinned2::CPoolList::IsEmpty() const {
  return front == nullptr;
}

inline MemoryAllocatorBinned2::CPoolInfo&
MemoryAllocatorBinned2::CPoolList::GetFrontPool() {
  fun_check(!IsEmpty());
  return *front;
}

inline const MemoryAllocatorBinned2::CPoolInfo&
MemoryAllocatorBinned2::CPoolList::GetFrontPool() const {
  fun_check(!IsEmpty());
  return *front;
}

void MemoryAllocatorBinned2::CPoolList::LinkToFront(CPoolInfo* pool) {
  pool->Unlink();
  pool->Link(front);
}

MemoryAllocatorBinned2::CPoolInfo&
MemoryAllocatorBinned2::CPoolList::PushNewPoolToFront(
    MemoryAllocatorBinned2& allocator, uint32 block_size, uint32 pool_index) {
  const uint32 LocalPageSize = allocator.PAGE_SIZE;

  // Allocate memory.
  CFreeBlock* free =
      new (allocator.CachedOSPageAllocator.Allocate(LocalPageSize))
          CFreeBlock(LocalPageSize, block_size, pool_index);
  if (!free) {
    Private::OutOCMemory(LocalPageSize);
  }
  fun_check(IsAligned(free, LocalPageSize));
  // Create pool
  CPoolInfo* result = Private::GetOrCreatePoolInfo(
      allocator, free, CPoolInfo::ECanary::FirstFreeBlockIsPtr, false);
  result->Link(front);
  result->taken = 0;
  result->first_free_block = free;

  return *result;
}

MemoryAllocatorBinned2::MemoryAllocatorBinned2()
    : hash_bucket_free_list_(nullptr) {
  static bool bOnce = false;
  fun_check(!bOnce);  // this is now a singleton-like thing and you cannot make
                      // multiple copies
  bOnce = true;

  for (uint32 index = 0; index != BINNED2_SMALL_POOL_COUNT; ++index) {
    uint32 Partner = BINNED2_SMALL_POOL_COUNT - index - 1;
    SmallBlockSizesReversed[index] = SmallBlockSizes[Partner];
  }
  CGenericPlatformMemoryConstants Constants = PlatformMemory::GetConstants();
  PAGE_SIZE = Constants.PAGE_SIZE;
  OsAllocationGranularity = Constants.OsAllocationGranularity
                                ? Constants.OsAllocationGranularity
                                : PAGE_SIZE;
  NumPoolsPerPage = PAGE_SIZE / sizeof(CPoolInfo);
  PtrToPoolMapping.Init(PAGE_SIZE, NumPoolsPerPage, Constants.AddressLimit);

  CHECKF(MathBase::IsPowerOfTwo(PAGE_SIZE),
         TEXT("OS page size must be a power of two"));
  CHECKF(MathBase::IsPowerOfTwo(Constants.AddressLimit),
         TEXT("OS address limit must be a power of two"));
  CHECKF(Constants.AddressLimit > PAGE_SIZE, TEXT("OS address limit must be "
                                                  "greater than the page "
                                                  "size"));  // Check to catch
                                                             // 32 bit overflow
                                                             // in AddressLimit
  CHECKF(
      SmallBlockSizes[BINNED2_SMALL_POOL_COUNT - 1] ==
          BINNED2_MAX_SMALL_POOL_SIZE,
      TEXT("BINNED2_MAX_SMALL_POOL_SIZE must equal the smallest block size"));
  CHECKF(PAGE_SIZE % BINNED2_LARGE_ALLOC == 0,
         TEXT("OS page size must be a multiple of BINNED2_LARGE_ALLOC"));
  CHECKF(sizeof(MemoryAllocatorBinned2::CFreeBlock) <= SmallBlockSizes[0],
         TEXT("pool header must be able to fit into the smallest block"));
  static_assert(
      countof(SmallBlockSizes) == BINNED2_SMALL_POOL_COUNT,
      "Small block size array size must match BINNED2_SMALL_POOL_COUNT");
  static_assert(countof(SmallBlockSizes) <= 256,
                "Small block size array size must fit in a byte");
  static_assert(sizeof(CFreeBlock) <= BINNED2_MINIMUM_ALIGNMENT,
                "Free block struct must be small enough to fit into a block.");

  // Init pool tables.
  for (uint32 index = 0; index != BINNED2_SMALL_POOL_COUNT; ++index) {
    CHECKF(index == 0 || SmallBlockSizes[index - 1] < SmallBlockSizes[index],
           TEXT("Small block sizes must be strictly increasing"));
    CHECKF(SmallBlockSizes[index] <= PAGE_SIZE,
           TEXT("Small block size must be small enough to fit into a page"));
    CHECKF(SmallBlockSizes[index] % BINNED2_MINIMUM_ALIGNMENT == 0,
           TEXT("Small block size must be a multiple of "
                "BINNED2_MINIMUM_ALIGNMENT"));

    SmallPoolTables[index].BlockSize = SmallBlockSizes[index];
  }

  // Set up pool mappings
  uint8* IndexEntry = MemSizeToIndex;
  uint32 pool_index = 0;
  for (uint32 index = 0; index != 1 + (BINNED2_MAX_SMALL_POOL_SIZE >>
                                       BINNED2_MINIMUM_ALIGNMENT_SHIFT);
       ++index) {
    uint32 BlockSize =
        index
        << BINNED2_MINIMUM_ALIGNMENT_SHIFT;  // inverse of int32 index =
                                             // int32((size >>
                                             // BINNED2_MINIMUM_ALIGNMENT_SHIFT));
    while (SmallBlockSizes[pool_index] < BlockSize) {
      ++pool_index;
      fun_check(pool_index != BINNED2_SMALL_POOL_COUNT);
    }
    fun_check(pool_index < 256);
    *IndexEntry++ = uint8(pool_index);
  }
  // now reverse the pool sizes for cache coherency

  for (uint32 index = 0; index != BINNED2_SMALL_POOL_COUNT; ++index) {
    uint32 Partner = BINNED2_SMALL_POOL_COUNT - index - 1;
    SmallBlockSizesReversed[index] = SmallBlockSizes[Partner];
  }

  uint64 max_hash_buckets_ = PtrToPoolMapping.GetMaxHashBuckets();

  hash_buckets_ = (PoolHashBucket*)PlatformMemory::BinnedAllocFromOS(Align(
      max_hash_buckets_ * sizeof(PoolHashBucket), OsAllocationGranularity));
  DefaultConstructItems<PoolHashBucket>(hash_buckets_, max_hash_buckets_);
  MemoryAllocatorBinned2 = this;
  GFixedMallocLocationPtr = (MemoryAllocator**)(&MemoryAllocatorBinned2);
}

MemoryAllocatorBinned2::~MemoryAllocatorBinned2() {}

bool MemoryAllocatorBinned2::IsInternallyThreadSafe() const { return true; }

void* MemoryAllocatorBinned2::MallocExternal(size_t size, uint32 alignment) {
  static_assert(DEFAULT_ALIGNMENT <= BINNED2_MINIMUM_ALIGNMENT,
                "DEFAULT_ALIGNMENT is assumed to be zero");  // used below

  // Only allocate from the small pools if the size is small enough and the
  // alignment isn't crazy large. With large alignments, we'll waste a lot of
  // memory allocating an entire page, but such alignments are highly unlikely
  // in practice.
  if ((size <= BINNED2_MAX_SMALL_POOL_SIZE) &
      (alignment <= BINNED2_MINIMUM_ALIGNMENT)) {  // one branch, not two
    uint32 pool_index = BoundSizeToPoolIndex(size);

    CPerThreadFreeBlockLists* Lists = GMallocBinned2PerThreadCaches
                                          ? CPerThreadFreeBlockLists::Get()
                                          : nullptr;
    if (Lists) {
      if (Lists->ObtainRecycledPartial(pool_index)) {
        if (void* result = Lists->Malloc(pool_index)) {
          return result;
        }
      }
    }

    CScopedLock guard(Mutex);

    // Allocate from small object pool.
    CPoolTable& table = SmallPoolTables[pool_index];

    CPoolInfo* pool;
    if (!table.ActivePools.IsEmpty()) {
      pool = &table.ActivePools.GetFrontPool();
    } else {
      pool = &table.ActivePools.PushNewPoolToFront(*this, table.BlockSize,
                                                   pool_index);
    }

    void* result = pool->AllocateRegularBlock();
    if (GMallocBinned2AllocExtra) {
      if (Lists) {
        for (int32 index = 0;
             index < GMallocBinned2AllocExtra && pool->HasFreeRegularBlock();
             index++) {
          if (!Lists->Free(result, pool_index, table.BlockSize)) {
            break;
          }
          result = pool->AllocateRegularBlock();
        }
      }
    }
    if (!pool->HasFreeRegularBlock()) {
      table.ExhaustedPools.LinkToFront(pool);
    }

    return result;
  }
  alignment = MathBase::Max<uint32>(alignment, BINNED2_MINIMUM_ALIGNMENT);
  size = Align(MathBase::Max((size_t)1, size), alignment);

  fun_check(MathBase::IsPowerOfTwo(alignment));
  fun_check(alignment <= PAGE_SIZE);

  CScopedLock guard(Mutex);

  // Use OS for non-pooled allocations.
  uintptr_t AlignedSize = Align(size, OsAllocationGranularity);
  void* result = CachedOSPageAllocator.Allocate(AlignedSize);
  if (!result) {
    Private::OutOCMemory(AlignedSize);
  }
  fun_check(IsAligned(result, PAGE_SIZE) && IsOSAllocation(result));

  // Create pool.
  CPoolInfo* pool = Private::GetOrCreatePoolInfo(
      *this, result, CPoolInfo::ECanary::FirstFreeBlockIsOSAllocSize, false);
  fun_check(size > 0 && size <= AlignedSize &&
            AlignedSize >= OsAllocationGranularity);
  pool->SetOSAllocationSizes(size, AlignedSize);

  return result;
}

void* MemoryAllocatorBinned2::ReallocExternal(void* ptr, size_t new_size,
                                              uint32 alignment) {
  if (new_size == 0) {
    MemoryAllocatorBinned2::FreeExternal(ptr);
    return nullptr;
  }
  static_assert(DEFAULT_ALIGNMENT <= BINNED2_MINIMUM_ALIGNMENT,
                "DEFAULT_ALIGNMENT is assumed to be zero");  // used below
  fun_check(MathBase::IsPowerOfTwo(alignment));
  fun_check(alignment <= PAGE_SIZE);

  if (!IsOSAllocation(ptr)) {
    fun_check(ptr);  // null is 64k aligned so we should not be here
    // Reallocate to a smaller/bigger pool if necessary
    CFreeBlock* Free = GetPoolHeaderFromPointer(ptr);
    Free->CanaryTest();
    uint32 BlockSize = Free->block_size;
    uint32 pool_index = Free->pool_index;
    if (((new_size <= BlockSize) &
         (alignment <= BINNED2_MINIMUM_ALIGNMENT)) &&  // one branch, not two
        (pool_index == 0 || new_size > PoolIndexToBlockSize(pool_index - 1))) {
      return ptr;
    }

    // Reallocate and copy the data across
    void* result = MemoryAllocatorBinned2::MallocExternal(new_size, alignment);
    UnsafeMemory::Memcpy(result, ptr,
                         MathBase::Min<size_t>(new_size, BlockSize));
    MemoryAllocatorBinned2::FreeExternal(ptr);
    return result;
  }
  if (!ptr) {
    void* result = MemoryAllocatorBinned2::MallocExternal(new_size, alignment);
    return result;
  }

  CScopedLock guard(Mutex);

  // Allocated from OS.
  CPoolInfo* pool = Private::FindPoolInfo(*this, ptr);
  if (!pool) {
    fun_log(LogMemory, Fatal,
            TEXT("MemoryAllocatorBinned2 Attempt to realloc an unrecognized "
                 "block %p"),
            ptr);
  }
  uintptr_t PoolOsBytes = pool->GetOsAllocatedBytes();
  uint32 PoolOSRequestedBytes = pool->GetOSRequestedBytes();
  CHECKF(PoolOSRequestedBytes <= PoolOsBytes,
         TEXT("MemoryAllocatorBinned2::ReallocExternal %d %d"),
         int32(PoolOSRequestedBytes), int32(PoolOsBytes));
  if (new_size > PoolOsBytes ||  // can't fit in the old block
      (new_size <= BINNED2_MAX_SMALL_POOL_SIZE &&
       alignment <= BINNED2_MINIMUM_ALIGNMENT) ||  // can switch to the small
                                                   // block allocator
      Align(new_size, OsAllocationGranularity) <
          PoolOsBytes) {  // we can get some pages back
    // Grow or shrink.
    void* result = MemoryAllocatorBinned2::MallocExternal(new_size, alignment);
    UnsafeMemory::Memcpy(result, ptr,
                         MathBase::Min<size_t>(new_size, PoolOSRequestedBytes));
    MemoryAllocatorBinned2::FreeExternal(ptr);
    return result;
  }

  pool->SetOSAllocationSizes(new_size, PoolOsBytes);

  return ptr;
}

void MemoryAllocatorBinned2::FreeExternal(void* ptr) {
  if (!IsOSAllocation(ptr)) {
    fun_check(ptr);  // null is 64k aligned so we should not be here
    CFreeBlock* base_ptr = GetPoolHeaderFromPointer(ptr);
    base_ptr->CanaryTest();
    uint32 BlockSize = base_ptr->block_size;
    uint32 pool_index = base_ptr->pool_index;
    CBundleNode* bundles_to_recycle = nullptr;
    CPerThreadFreeBlockLists* Lists = GMallocBinned2PerThreadCaches
                                          ? CPerThreadFreeBlockLists::Get()
                                          : nullptr;
    if (Lists) {
      bundles_to_recycle = Lists->RecycleFullBundle(base_ptr->pool_index);
      bool bPushed = Lists->Free(ptr, pool_index, BlockSize);
      fun_check(bPushed);
    } else {
      bundles_to_recycle = (CBundleNode*)ptr;
      bundles_to_recycle->next_node_in_current_bundle = nullptr;
    }
    if (bundles_to_recycle) {
      bundles_to_recycle->next_bundle = nullptr;
      CScopedLock guard(Mutex);
      Private::free_bundles(*this, bundles_to_recycle, BlockSize, pool_index);
    }
  } else if (ptr) {
    CScopedLock guard(Mutex);
    CPoolInfo* pool = Private::FindPoolInfo(*this, ptr);
    if (!pool) {
      fun_log(LogMemory, Fatal,
              TEXT("MemoryAllocatorBinned2 Attempt to free an unrecognized "
                   "block %p"),
              ptr);
    }
    uintptr_t PoolOsBytes = pool->GetOsAllocatedBytes();
    uint32 PoolOSRequestedBytes = pool->GetOSRequestedBytes();
    CHECKF(PoolOSRequestedBytes <= PoolOsBytes,
           TEXT("MemoryAllocatorBinned2::FreeExternal %d %d"),
           int32(PoolOSRequestedBytes), int32(PoolOsBytes));
    pool->SetCanary(CPoolInfo::ECanary::Unassigned, true, false);
    // Free an OS allocation.
    CachedOSPageAllocator.Free(ptr, PoolOsBytes);
  }
}

bool MemoryAllocatorBinned2::GetAllocationSizeExternal(
    void* ptr, size_t& out_allocation_size) {
  if (!IsOSAllocation(ptr)) {
    fun_check(ptr);  // null is 64k aligned so we should not be here
    const CFreeBlock* Free = GetPoolHeaderFromPointer(ptr);
    Free->CanaryTest();
    uint32 BlockSize = Free->block_size;
    out_allocation_size = BlockSize;
    return true;
  }
  if (!ptr) {
    return false;
  }
  CScopedLock guard(mutex_);
  CPoolInfo* pool = Private::FindPoolInfo(*this, ptr);
  if (!pool) {
    fun_log(LogMemory, Fatal,
            TEXT("MemoryAllocatorBinned2 Attempt to GetAllocationSizeExternal "
                 "an unrecognized block %p"),
            ptr);
  }
  uintptr_t PoolOsBytes = pool->GetOsAllocatedBytes();
  uint32 PoolOSRequestedBytes = pool->GetOSRequestedBytes();
  CHECKF(PoolOSRequestedBytes <= PoolOsBytes,
         TEXT("MemoryAllocatorBinned2::GetAllocationSizeExternal %d %d"),
         int32(PoolOSRequestedBytes), int32(PoolOsBytes));
  out_allocation_size = PoolOSRequestedBytes;
  return true;
}

void MemoryAllocatorBinned2::CPoolList::ValidateActivePools() {
  for (CPoolInfo** pool_ptr = &Front; *pool_ptr;
       pool_ptr = &(*pool_ptr)->next) {
    CPoolInfo* pool = *pool_ptr;
    fun_check(pool->ptr_to_prev_next == pool_ptr);
    fun_check(pool->first_free_block);
    for (CFreeBlock* Free = pool->first_free_block; Free;
         Free = (CFreeBlock*)Free->nextFreeBlock) {
      fun_check(Free->GetNumFreeRegularBlocks() > 0);
    }
  }
}

void MemoryAllocatorBinned2::CPoolList::ValidateExhaustedPools() {
  for (CPoolInfo** pool_ptr = &Front; *pool_ptr;
       pool_ptr = &(*pool_ptr)->next) {
    CPoolInfo* pool = *pool_ptr;
    fun_check(pool->ptr_to_prev_next == pool_ptr);
    fun_check(!pool->first_free_block);
  }
}

bool MemoryAllocatorBinned2::ValidateHeap() {
  CScopedLock guard(mutex_);

  for (CPoolTable& table : SmallPoolTables) {
    table.ActivePools.ValidateActivePools();
    table.ExhaustedPools.ValidateExhaustedPools();
  }

  return true;
}

const char* MemoryAllocatorBinned2::GetDescriptiveName() {
  return TEXT("binned2");
}

void MemoryAllocatorBinned2::FlushCurrentThreadCache() {
  CPerThreadFreeBlockLists* Lists = CPerThreadFreeBlockLists::Get();
  if (Lists) {
    CScopedLock guard(Mutex);
    for (int32 pool_index = 0; pool_index != BINNED2_SMALL_POOL_COUNT;
         ++pool_index) {
      CBundleNode* Bundles = Lists->PopBundles(pool_index);
      if (Bundles) {
        Private::free_bundles(*this, Bundles, PoolIndexToBlockSize(pool_index),
                              pool_index);
      }
    }
  }
}

void MemoryAllocatorBinned2::Trim() {
  // TODO?
  // QUICK_SCOPED_CYCLE_COUNTER(STAT_CMallocBinned2_Trim);
  //
  // if (GMallocBinned2PerThreadCaches) {
  //  //double StartTime = SystemTime::Seconds();
  //  TFunction<void(ENamedThreads::Type CurrentThread)> Broadcast =
  //    [this](ENamedThreads::Type MyThread)
  //  {
  //    FlushCurrentThreadCache();
  //  };
  //  CTaskGraphInterface::BroadcastSlow_OnlyUseForSpecialPurposes(false,
  //  Broadcast);
  //  //fun_log(LogTemp, Info, TEXT("Trim Broadcast = %6.2fms"), 1000.0f *
  //  float(SystemTime::Seconds() - StartTime));
  //}
  {
    // double StartTime = SystemTime::Seconds();
    CScopedLock guard(Mutex);
    CachedOSPageAllocator.FreeAll();
    // fun_log(LogTemp, Info, TEXT("Trim CachedOSPageAllocator = %6.2fms"),
    // 1000.0f * float(SystemTime::Seconds() - StartTime));
  }
}

void MemoryAllocatorBinned2::SetupTlsCachesOnCurrentThread() {
  if (!BINNED2_ALLOW_RUNTIME_TWEAKING && !GMallocBinned2PerThreadCaches) {
    return;
  }
  if (!MemoryAllocatorBinned2::Binned2TlsSlot) {
    MemoryAllocatorBinned2::Binned2TlsSlot = CPlatformTLS::AllocTlsSlot();
  }
  fun_check(MemoryAllocatorBinned2::Binned2TlsSlot);
  CPerThreadFreeBlockLists::SetTLS();
}

void MemoryAllocatorBinned2::ClearAndDisableTlsCachesOnCurrentThread() {
  FlushCurrentThreadCache();
  CPerThreadFreeBlockLists::ClearTLS();
}

bool MemoryAllocatorBinned2::CFreeBlockList::ObtainPartial(uint32 pool_index) {
  if (!PartialBundle.Head) {
    PartialBundle.Count = 0;
    PartialBundle.Head =
        MemoryAllocatorBinned2::Private::GGlobalRecycler.PopBundle(pool_index);
    if (PartialBundle.Head) {
      PartialBundle.Count = PartialBundle.Head->Count;
      PartialBundle.Head->next_bundle = nullptr;
      return true;
    }
    return false;
  }
  return true;
}

MemoryAllocatorBinned2::CBundleNode*
MemoryAllocatorBinned2::CFreeBlockList::RecyleFull(uint32 pool_index) {
  MemoryAllocatorBinned2::CBundleNode* result = nullptr;
  if (FullBundle.Head) {
    FullBundle.Head->Count = FullBundle.Count;
    if (!MemoryAllocatorBinned2::Private::GGlobalRecycler.PushBundle(
            pool_index, FullBundle.Head)) {
      result = FullBundle.Head;
      result->next_bundle = nullptr;
    }
    FullBundle.Reset();
  }
  return result;
}

MemoryAllocatorBinned2::CBundleNode*
MemoryAllocatorBinned2::CFreeBlockList::PopBundles(uint32 pool_index) {
  CBundleNode* Partial = PartialBundle.Head;
  if (Partial) {
    PartialBundle.Reset();
    Partial->next_bundle = nullptr;
  }

  CBundleNode* Full = FullBundle.Head;
  if (Full) {
    FullBundle.Reset();
    Full->next_bundle = nullptr;
  }

  CBundleNode* result = Partial;
  if (result) {
    result->next_bundle = Full;
  } else {
    result = Full;
  }

  return result;
}

void MemoryAllocatorBinned2::CPerThreadFreeBlockLists::SetTLS() {
  fun_check(MemoryAllocatorBinned2::Binned2TlsSlot);
  CPerThreadFreeBlockLists* ThreadSingleton =
      (CPerThreadFreeBlockLists*)CPlatformTLS::GetTlsValue(
          MemoryAllocatorBinned2::Binned2TlsSlot);
  if (!ThreadSingleton) {
    ThreadSingleton = new (PlatformMemory::BinnedAllocFromOS(
        Align(sizeof(CPerThreadFreeBlockLists),
              MemoryAllocatorBinned2::OsAllocationGranularity)))
        CPerThreadFreeBlockLists();
    CPlatformTLS::SetTlsValue(MemoryAllocatorBinned2::Binned2TlsSlot,
                              ThreadSingleton);
  }
}

void MemoryAllocatorBinned2::CPerThreadFreeBlockLists::ClearTLS() {
  fun_check(MemoryAllocatorBinned2::Binned2TlsSlot);
  CPlatformTLS::SetTlsValue(MemoryAllocatorBinned2::Binned2TlsSlot, nullptr);
}

void MemoryAllocatorBinned2::CFreeBlock::CanaryFail() const {
  fun_log(LogMemory, Fatal,
          TEXT("MemoryAllocatorBinned2 Attempt to realloc an unrecognized "
               "block %p   canary == 0x%x != 0x%x"),
          (void*)this, (int32)canary,
          (int32)MemoryAllocatorBinned2::CFreeBlock::CANARY_VALUE);
}

#if !BINNED2_INLINE
#if PLATFORM_USES_FIXED_GMalloc_CLASS && !FUN_FORCE_ANSI_ALLOCATOR && \
    USE_MALLOC_BINNED2
//#define CMemory_INLINE_FUNCTION_DECORATOR  inline
#define CMEMORY_INLINE_GMalloc (MemoryAllocatorBinned2::MemoryAllocatorBinned2)
#include "CMemory_inline.h"
#endif
#endif

}  // namespace fun
