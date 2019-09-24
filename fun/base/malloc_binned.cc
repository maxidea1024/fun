#include "fun/base/malloc_binned.h"
#include "HAL/MemoryMisc.h"

namespace fun {

/** Malloc binned allocator specific stats. */
DEFINE_STAT(STAT_Binned_OsCurrent);
DEFINE_STAT(STAT_Binned_OsPeak);
DEFINE_STAT(STAT_Binned_WasteCurrent);
DEFINE_STAT(STAT_Binned_WastePeak);
DEFINE_STAT(STAT_Binned_UsedCurrent);
DEFINE_STAT(STAT_Binned_UsedPeak);
DEFINE_STAT(STAT_Binned_CurrentAllocs);
DEFINE_STAT(STAT_Binned_TotalAllocs);
DEFINE_STAT(STAT_Binned_SlackCurrent);

/** Information about a piece of free memory. 8 bytes */
struct MemoryAllocatorBinned::CFreeMem {
  /** Next or MemLastPool[], always in order by pool. */
  CFreeMem* next;
  /** Number of consecutive free blocks here, at least 1. */
  uint32 free_block_count;
};

// Memory pool info. 32 bytes.
struct MemoryAllocatorBinned::CPoolInfo {
  /** Number of allocated elements in this pool, when counts down to zero can
   * free the entire pool. */
  uint16 Taken;  // 2

  /**
  index of pool. index into mem_size_to_pool_table_[]. Valid when <
  MAX_POOLED_ALLOCATION_SIZE, MAX_POOLED_ALLOCATION_SIZE is os_table_. When
  alloc_size is 0, this is the number of pages to step back to find the base
  address of an allocation. See FindPoolInfoInternal()
  */
  uint16 table_index;  // 4

  /** Number of bytes allocated */
  uint32 alloc_size;  // 8

  /** Pointer to first free memory in this pool or the OS Allocation size in
   * bytes if this allocation is not binned*/
  CFreeMem* first_mem;  // 12/16

  CPoolInfo* next;  // 16/24

  CPoolInfo** prev_link;  // 20/32

#if PLATFORM_32BITS
  /** Explicit padding for 32 bit builds */
  uint8 padding[12];  // 32
#endif

  void SetAllocationSizes(uint32 InBytes, uintptr_t InOsBytes,
                          uint32 InTableIndex, uint32 SmallAllocLimt) {
    table_index = InTableIndex;
    alloc_size = InBytes;
    if (table_index == SmallAllocLimt) {
      first_mem = (CFreeMem*)InOsBytes;
    }
  }

  uint32 GetBytes() const { return alloc_size; }

  uintptr_t GetOsBytes(uint32 InPageSize, uint32 SmallAllocLimt) const {
    if (table_index == SmallAllocLimt) {
      return (uintptr_t)first_mem;
    } else {
      return Align(alloc_size, InPageSize);
    }
  }

  void Link(CPoolInfo*& before) {
    if (before) {
      before->prev_link = &next;
    }
    next = before;
    prev_link = &before;
    before = this;
  }

  void Unlink() {
    if (next) {
      next->prev_link = prev_link;
    }
    *prev_link = next;
  }
};

/** hash table struct for retrieving allocation book keeping information */
struct MemoryAllocatorBinned::CPoolHashBucket {
  uintptr_t key;
  CPoolInfo* first_pool;
  CPoolHashBucket* prev;
  CPoolHashBucket* next;

  CPoolHashBucket() {
    key = 0;
    first_pool = nullptr;
    prev = this;
    next = this;
  }

  void Link(CPoolHashBucket* after) { Link(after, prev, this); }

  static void Link(CPoolHashBucket* node, CPoolHashBucket* before,
                   CPoolHashBucket* after) {
    node->prev = before;
    node->next = after;
    before->next = node;
    after->prev = node;
  }

  void Unlink() {
    next->prev = prev;
    prev->next = next;
    prev = this;
    next = this;
  }
};

struct MemoryAllocatorBinned::Private {
  /** Default alignment for binned allocator */
  enum { DEFAULT_BINNED_ALLOCATOR_ALIGNMENT = sizeof(CFreeMem) };
  enum { PAGE_SIZE_LIMIT = 65536 };
  // BINNED_ALLOC_POOL_SIZE can be increased beyond 64k to cause binned malloc
  // to allocate the small size bins in bigger chunks. If OS Allocation is slow,
  // increasing this number *may* help performance but YMMV.
  enum { BINNED_ALLOC_POOL_SIZE = 65536 };

  // Implementation.
  static CA_NO_RETURN void OutOfMemory(uint64 size, uint32 alignment = 0) {
    // this is expected not to return
    PlatformMemory::OnOutOfMemory(size, alignment);
  }

  static inline void TrackStats(CPoolTable* table, size_t size) {
#if STATS
    // keep track of memory lost to padding
    table->total_waste += table->block_size - size;
    table->total_request_count++;
    table->active_request_count++;
    table->max_active_requests =
        MathBase::Max(table->max_active_requests, table->active_request_count);
    table->max_request = size > table->max_request ? size : table->max_request;
    table->min_request = size < table->min_request ? size : table->min_request;
#endif  // STATS
  }

  /**
  Create a 64k page of CPoolInfo structures for tracking allocations
  */
  static CPoolInfo* CreateIndirect(MemoryAllocatorBinned& allocator) {
    uint64 IndirectPoolBlockSizeBytes =
        allocator.indect_pool_block_size_ * sizeof(CPoolInfo);

    CHECK_SLOW(IndirectPoolBlockSizeBytes <= allocator.PAGE_SIZE);
    CPoolInfo* indirect = (CPoolInfo*)PlatformMemory::BinnedAllocFromOS(
        IndirectPoolBlockSizeBytes);
    if (indirect == nullptr) {
      OutOfMemory(IndirectPoolBlockSizeBytes);
    }
    UnsafeMemory::Memset(indirect, 0, IndirectPoolBlockSizeBytes);

    BINNED_PEAK_STATCOUNTER(
        allocator.os_peak_,
        BINNED_ADD_STATCOUNTER(
            allocator.os_current_,
            (int64)(Align(IndirectPoolBlockSizeBytes, allocator.PAGE_SIZE))));
    BINNED_PEAK_STATCOUNTER(
        allocator.waste_peak_,
        BINNED_ADD_STATCOUNTER(
            allocator.waste_current_,
            (int64)(Align(IndirectPoolBlockSizeBytes, allocator.PAGE_SIZE))));

    return indirect;
  }

  /**
  Gets the CPoolInfo for a memory address. If no valid info exists one is
  created. NOTE: This function requires a mutex across threads, but its is the
  callers responsibility to acquire the mutex before calling
  */
  static inline CPoolInfo* GetPoolInfo(MemoryAllocatorBinned& allocator,
                                       uintptr_t ptr) {
    if (allocator.hash_buckets_ == nullptr) {
      // Init tables.
      allocator.hash_buckets_ =
          (CPoolHashBucket*)PlatformMemory::BinnedAllocFromOS(
              Align(allocator.max_hash_buckets_ * sizeof(CPoolHashBucket),
                    allocator.PAGE_SIZE));

      for (uint32 i = 0; i < allocator.max_hash_buckets_; ++i) {
        new (allocator.hash_buckets_ + i) CPoolHashBucket();
      }
    }

    uintptr_t key = ptr >> allocator.hash_key_shift_;
    uintptr_t hash = key & (allocator.max_hash_buckets_ - 1);
    uintptr_t pool_index =
        ((uintptr_t)ptr >> allocator.pool_bit_shift_) & allocator.pool_mask_;

    CPoolHashBucket* collision = &allocator.hash_buckets_[hash];
    do {
      if (collision->key == key || collision->first_pool == nullptr) {
        if (collision->first_pool == nullptr) {
          collision->key = key;
          InitializeHashBucket(allocator, collision);
          CA_ASSUME(collision->first_pool);
        }
        return &collision->first_pool[pool_index];
      }

      collision = collision->next;
    } while (collision != &allocator.hash_buckets_[hash]);

    // Create a new hash bucket entry
    CPoolHashBucket* NewBucket = CreateHashBucket(allocator);
    NewBucket->key = key;
    allocator.hash_buckets_[hash].Link(NewBucket);

    return &NewBucket->first_pool[pool_index];
  }

  static inline CPoolInfo* FindPoolInfo(MemoryAllocatorBinned& allocator,
                                        uintptr_t ptr1,
                                        uintptr_t& allocation_base) {
    uint16 next_step = 0;
    uintptr_t ptr = ptr1 & ~((uintptr_t)allocator.PAGE_SIZE - 1);
    for (uint32 i = 0, N = (BINNED_ALLOC_POOL_SIZE / allocator.PAGE_SIZE) + 1;
         i < N; ++i) {
      CPoolInfo* pool = FindPoolInfoInternal(allocator, ptr, next_step);
      if (pool) {
        allocation_base = ptr;
        // CHECK_SLOW(ptr1 >= allocation_base && ptr1 < allocation_base +
        // pool->GetBytes());
        return pool;
      }
      ptr = ((ptr - (allocator.PAGE_SIZE * next_step)) - 1) &
            ~((uintptr_t)allocator.PAGE_SIZE - 1);
    }
    allocation_base = 0;
    return nullptr;
  }

  static inline CPoolInfo* FindPoolInfoInternal(
      MemoryAllocatorBinned& allocator, uintptr_t ptr, uint16& jump_offset) {
    CHECK_SLOW(allocator.hash_buckets_);

    uint32 key = ptr >> allocator.hash_key_shift_;
    uint32 hash = key & (allocator.max_hash_buckets_ - 1);
    uint32 pool_index =
        ((uintptr_t)ptr >> allocator.pool_bit_shift_) & allocator.pool_mask_;

    jump_offset = 0;

    CPoolHashBucket* collision = &allocator.hash_buckets_[hash];
    do {
      if (collision->key == key) {
        if (collision->first_pool[pool_index].alloc_size == 0) {
          jump_offset = collision->first_pool[pool_index].table_index;
          return nullptr;
        }
        return &collision->first_pool[pool_index];
      }
      collision = collision->next;
    } while (collision != &allocator.hash_buckets_[hash]);

    return nullptr;
  }

  /**
   * Returns a newly created and initialized CPoolHashBucket for use.
   */
  static inline CPoolHashBucket* CreateHashBucket(
      MemoryAllocatorBinned& allocator) {
    CPoolHashBucket* bucket = AllocateHashBucket(allocator);
    InitializeHashBucket(allocator, bucket);
    return bucket;
  }

  /**
   * Initializes bucket with valid parameters
   *
   * @param bucket - pointer to be initialized
   */
  static inline void InitializeHashBucket(MemoryAllocatorBinned& allocator,
                                          CPoolHashBucket* bucket) {
    if (bucket->first_pool == nullptr) {
      bucket->first_pool = CreateIndirect(allocator);
    }
  }

  /**
   * Allocates a hash bucket from the free list of hash buckets
   */
  static CPoolHashBucket* AllocateHashBucket(MemoryAllocatorBinned& allocator) {
    if (allocator.hash_bucket_free_list_ == nullptr) {
      const uint32 PAGE_SIZE = allocator.PAGE_SIZE;

      allocator.hash_bucket_free_list_ =
          (CPoolHashBucket*)PlatformMemory::BinnedAllocFromOS(PAGE_SIZE);
      BINNED_PEAK_STATCOUNTER(
          allocator.os_peak_,
          BINNED_ADD_STATCOUNTER(allocator.os_current_, PAGE_SIZE));
      BINNED_PEAK_STATCOUNTER(
          allocator.waste_peak_,
          BINNED_ADD_STATCOUNTER(allocator.waste_current_, PAGE_SIZE));

      for (uintptr_t i = 0, N = PAGE_SIZE / sizeof(CPoolHashBucket); i < N;
           ++i) {
        allocator.hash_bucket_free_list_->Link(
            new (allocator.hash_bucket_free_list_ + i) CPoolHashBucket());
      }
    }

    CPoolHashBucket* next_free = allocator.hash_bucket_free_list_->next;
    CPoolHashBucket* Free = allocator.hash_bucket_free_list_;

    Free->Unlink();
    if (next_free == Free) {
      next_free = nullptr;
    }
    allocator.hash_bucket_free_list_ = next_free;

    return Free;
  }

  static CPoolInfo* AllocatePoolMemory(MemoryAllocatorBinned& allocator,
                                       CPoolTable* table, uint32 PoolSize,
                                       uint16 table_index) {
    const uint32 PAGE_SIZE = allocator.PAGE_SIZE;

    // Must create a new pool.
    uint32 Blocks = PoolSize / table->BlockSize;
    uint32 Bytes = Blocks * table->BlockSize;
    uintptr_t OsBytes = Align(Bytes, PAGE_SIZE);

    CHECK_SLOW(Blocks >= 1);
    CHECK_SLOW(Blocks * table->BlockSize <= Bytes && PoolSize >= Bytes);

    // Allocate memory.
    CFreeMem* Free = nullptr;
    size_t ActualPoolSize;  // TODO: use this to reduce waste?
    Free = (CFreeMem*)OSAlloc(allocator, OsBytes, ActualPoolSize);

    CHECK_SLOW(!((uintptr_t)Free & (PAGE_SIZE - 1)));
    if (Free == nullptr) {
      OutOfMemory(OsBytes);
    }

    // Create pool in the indirect table.
    CPoolInfo* pool;
    {
#ifdef USE_FINE_GRAIN_LOCKS
      CScopedLock PoolInfoLock(allocator.access_mutex_);
#endif
      pool = GetPoolInfo(allocator, (uintptr_t)Free);
      for (uintptr_t i = (uintptr_t)PAGE_SIZE, offset = 0; i < OsBytes;
           i += PAGE_SIZE, ++offset) {
        CPoolInfo* TrailingPool = GetPoolInfo(allocator, ((uintptr_t)Free) + i);
        fun_check(TrailingPool);

        // Set trailing pools to point back to first pool
        TrailingPool->SetAllocationSizes(0, 0, offset,
                                         allocator.binned_os_table_index_);
      }

      BINNED_PEAK_STATCOUNTER(
          allocator.os_peak_,
          BINNED_ADD_STATCOUNTER(allocator.os_current_, OsBytes));
      BINNED_PEAK_STATCOUNTER(
          allocator.waste_peak_,
          BINNED_ADD_STATCOUNTER(allocator.waste_current_, (OsBytes - Bytes)));
    }

    // Init pool.
    pool->Link(table->first_pool);
    pool->SetAllocationSizes(Bytes, OsBytes, table_index,
                             allocator.binned_os_table_index_);
    pool->taken = 0;
    pool->first_mem = Free;

#if STATS
    table->active_pool_count++;
    table->max_active_pools =
        MathBase::Max(table->max_active_pools, table->active_pool_count);
#endif
    // Create first free item.
    Free->free_block_count = Blocks;
    Free->next = nullptr;

    return pool;
  }

  static inline CFreeMem* AllocateBlockFromPool(
      MemoryAllocatorBinned& allocator, CPoolTable* table, CPoolInfo* pool,
      uint32 alignment) {
    // Pick first available block and unlink it.
    pool->taken++;
    CHECK_SLOW(
        pool->table_index <
        allocator.binned_os_table_index_);  // if this is false, first_mem is
                                            // actually a size not a pointer
    CHECK_SLOW(pool->first_mem);
    CHECK_SLOW(pool->first_mem->free_block_count > 0);
    CHECK_SLOW(pool->first_mem->free_block_count < PAGE_SIZE_LIMIT);
    CFreeMem* Free =
        (CFreeMem*)((uint8*)pool->first_mem +
                    --pool->first_mem->free_block_count * table->BlockSize);
    if (pool->first_mem->free_block_count == 0) {
      pool->first_mem = pool->first_mem->next;
      if (pool->first_mem == nullptr) {
        // Move to exhausted list.
        pool->Unlink();
        pool->Link(table->exhausted_pool);
      }
    }
    BINNED_PEAK_STATCOUNTER(
        allocator.used_peak_,
        BINNED_ADD_STATCOUNTER(allocator.used_current_, table->BlockSize));
    return Align(Free, alignment);
  }

  /**
   * Releases memory back to the system. This is not protected from
   * multi-threaded access and it's the callers responsibility to guard
   * access_mutex_ before calling this.
   */
  static void FreeInternal(MemoryAllocatorBinned& allocator, void* ptr) {
    MEM_TIME(mem_time_ -= SystemTime::Seconds());
    BINNED_DECREMENT_STATCOUNTER(allocator.current_alloc_count_);

    uintptr_t base_ptr;
    CPoolInfo* pool = FindPoolInfo(allocator, (uintptr_t)ptr, base_ptr);
#if PLATFORM_IOS || PLATFORM_MAC
    if (pool == nullptr) {
      fun_log(LogMemory, Warning,
              TEXT("Attempting to free a pointer we didn't allocate!"));
      return;
    }
#endif
    CHECK_SLOW(pool);
    CHECK_SLOW(pool->GetBytes() != 0);
    if (pool->table_index < allocator.binned_os_table_index_) {
      CPoolTable* table = allocator.mem_size_to_pool_table_[pool->table_index];
#ifdef USE_FINE_GRAIN_LOCKS
      CScopedLock TableLock(table->CriticalSection);
#endif
#if STATS
      table->active_request_count--;
#endif
      // If this pool was exhausted, move to available list.
      if (pool->first_mem == nullptr) {
        pool->Unlink();
        pool->Link(table->first_pool);
      }

      void* BaseAddress = (void*)base_ptr;
      uint32 BlockSize = table->BlockSize;
      intptr_t OffsetFromBase = (intptr_t)ptr - (intptr_t)BaseAddress;
      fun_check(OffsetFromBase >= 0);
      uint32 AlignOffset = OffsetFromBase % BlockSize;

      // Patch pointer to include previously applied alignment.
      ptr = (void*)((intptr_t)ptr - (intptr_t)AlignOffset);

      // Free a pooled allocation.
      CFreeMem* Free = (CFreeMem*)ptr;
      Free->free_block_count = 1;
      Free->next = pool->first_mem;
      pool->first_mem = Free;
      BINNED_ADD_STATCOUNTER(allocator.used_current_,
                             -(int64)(table->BlockSize));

      // Free this pool.
      CHECK_SLOW(pool->taken >= 1);
      if (--pool->taken == 0) {
#if STATS
        table->active_pool_count--;
#endif
        // Free the OS memory.
        size_t OsBytes = pool->GetOsBytes(allocator.PAGE_SIZE,
                                          allocator.binned_os_table_index_);
        BINNED_ADD_STATCOUNTER(allocator.os_current_, -(int64)OsBytes);
        BINNED_ADD_STATCOUNTER(allocator.waste_current_,
                               -(int64)(OsBytes - pool->GetBytes()));
        pool->Unlink();
        pool->SetAllocationSizes(0, 0, 0, allocator.binned_os_table_index_);
        OSFree(allocator, (void*)base_ptr, OsBytes);
      }
    } else {
      // Free an OS allocation.
      CHECK_SLOW(!((uintptr_t)ptr & (allocator.PAGE_SIZE - 1)));
      size_t OsBytes = pool->GetOsBytes(allocator.PAGE_SIZE,
                                        allocator.binned_os_table_index_);

      BINNED_ADD_STATCOUNTER(allocator.used_current_, -(int64)pool->GetBytes());
      BINNED_ADD_STATCOUNTER(allocator.os_current_, -(int64)OsBytes);
      BINNED_ADD_STATCOUNTER(allocator.waste_current_,
                             -(int64)(OsBytes - pool->GetBytes()));
      OSFree(allocator, (void*)base_ptr, OsBytes);
    }

    MEM_TIME(mem_time_ += SystemTime::Seconds());
  }

  static void PushFreeLockless(MemoryAllocatorBinned& allocator, void* ptr) {
#ifdef USE_LOCKFREE_DELETE
    allocator.pending_free_list_->Push(ptr);
#else
#ifdef USE_COARSE_GRAIN_LOCKS
    CScopedLock ScopedLock(&access_mutex_);
#endif
    FreeInternal(allocator, ptr);
#endif
  }

  /**
   * Clear and Process the list of frees to be deallocated. It's the callers
   * responsibility to guard access_mutex_ before calling this
   */
  static void FlushPendingFrees(MemoryAllocatorBinned& allocator) {
#ifdef USE_LOCKFREE_DELETE
    if (allocator.pending_free_list_ == nullptr &&
        !allocator.done_flush_list_init_) {
      allocator.done_flush_list_init_ = true;
      allocator.pending_free_list_ =
          new ((void*)allocator.pending_free_list_memory_)
              TLockFreePointerList<void>();
    }

    // Because a lockless list and TArray calls new/malloc internally, need to
    // guard against re-entry
    if (allocator.flushing_frees_ || allocator.pending_free_list_ == nullptr) {
      return;
    }
    allocator.flushing_frees_ = true;
    allocator.pending_free_list_->PopAll(allocator.flushed_frees_);
    for (uint32 i = 0, n = allocator.flushed_frees_.Num(); i < n; ++i) {
      FreeInternal(allocator, allocator.flushed_frees_[i]);
    }
    allocator.flushed_frees_.Reset();
    allocator.flushing_frees_ = false;
#endif
  }

  static inline void OSFree(MemoryAllocatorBinned& allocator, void* ptr,
                            size_t size) {
#ifdef CACHE_FREED_OS_ALLOCS
#ifdef USE_FINE_GRAIN_LOCKS
    CScopedLock MainLock(allocator.access_mutex_);
#endif
    if (size > MAX_CACHED_OS_FREES_BYTE_LIMIT / 4) {
      PlatformMemory::BinnedFreeToOS(ptr, size);
      return;
    }

    while (allocator.freed_page_block_count_ &&
           (allocator.freed_page_block_count_ >= MAX_CACHED_OS_FREES ||
            allocator.cached_total_ + size > MAX_CACHED_OS_FREES_BYTE_LIMIT)) {
      // Remove the oldest one
      void* free_ptr = allocator.freed_page_blocks_[0].ptr;
      size_t free_size = allocator.freed_page_blocks_[0].ByteSize;
      allocator.cached_total_ -= free_size;
      allocator.freed_page_block_count_--;
      if (allocator.freed_page_block_count_ > 0) {
        UnsafeMemory::Memmove(
            &allocator.freed_page_blocks_[0], &allocator.freed_page_blocks_[1],
            sizeof(CFreePageBlock) * allocator.freed_page_block_count_);
      }
      PlatformMemory::BinnedFreeToOS(free_ptr, free_size);
    }
    allocator.freed_page_blocks_[allocator.freed_page_block_count_].ptr = ptr;
    allocator.freed_page_blocks_[allocator.freed_page_block_count_].ByteSize =
        size;
    allocator.cached_total_ += size;
    ++allocator.freed_page_block_count_;
#else
    (void)size;
    PlatformMemory::BinnedFreeToOS(ptr);
#endif
  }

  static inline void* OSAlloc(MemoryAllocatorBinned& allocator, size_t new_size,
                              size_t& OutActualSize) {
#ifdef CACHE_FREED_OS_ALLOCS
    {
#ifdef USE_FINE_GRAIN_LOCKS
      // We want to hold the lock a little as possible so release it
      // before the big call to the OS
      CScopedLock MainLock(allocator.access_mutex_);
#endif
      for (uint32 i = 0; i < allocator.freed_page_block_count_; ++i) {
        // look for exact matches first, these are aligned to the page size, so
        // it should be quite common to hit these on small pages sizes
        if (allocator.freed_page_blocks_[i].ByteSize == new_size) {
          void* Ret = allocator.freed_page_blocks_[i].ptr;
          OutActualSize = allocator.freed_page_blocks_[i].ByteSize;
          allocator.cached_total_ -= allocator.freed_page_blocks_[i].ByteSize;
          if (i < allocator.freed_page_block_count_ - 1) {
            UnsafeMemory::Memmove(
                &allocator.freed_page_blocks_[i],
                &allocator.freed_page_blocks_[i + 1],
                sizeof(CFreePageBlock) *
                    (allocator.freed_page_block_count_ - i - 1));
          }
          allocator.freed_page_block_count_--;
          return Ret;
        }
      };

      for (uint32 i = 0; i < allocator.freed_page_block_count_; ++i) {
        // is it possible (and worth i.e. <25% overhead) to use this block
        if (allocator.freed_page_blocks_[i].ByteSize >= new_size &&
            allocator.freed_page_blocks_[i].ByteSize * 3 <= new_size * 4) {
          void* Ret = allocator.freed_page_blocks_[i].ptr;
          OutActualSize = allocator.freed_page_blocks_[i].ByteSize;
          allocator.cached_total_ -= allocator.freed_page_blocks_[i].ByteSize;
          if (i < allocator.freed_page_block_count_ - 1) {
            UnsafeMemory::Memmove(
                &allocator.freed_page_blocks_[i],
                &allocator.freed_page_blocks_[i + 1],
                sizeof(CFreePageBlock) *
                    (allocator.freed_page_block_count_ - i - 1));
          }
          allocator.freed_page_block_count_--;
          return Ret;
        }
      };
    }
    OutActualSize = new_size;
    void* ptr = PlatformMemory::BinnedAllocFromOS(new_size);
    if (ptr == nullptr) {
      // Are we holding on to much mem? Release it all.
      FlushAllocCache(allocator);
      ptr = PlatformMemory::BinnedAllocFromOS(new_size);
    }
    return ptr;
#else
    (void)OutActualSize;
    return PlatformMemory::BinnedAllocFromOS(new_size);
#endif
  }

#ifdef CACHE_FREED_OS_ALLOCS
  static void FlushAllocCache(MemoryAllocatorBinned& allocator) {
#ifdef USE_FINE_GRAIN_LOCKS
    CScopedLock MainLock(allocator.access_mutex_);
#endif
    for (int32 i = 0, N = allocator.freed_page_block_count_; i < N; ++i) {
      // Remove allocs
      PlatformMemory::BinnedFreeToOS(allocator.freed_page_blocks_[i].ptr,
                                     allocator.freed_page_blocks_[i].ByteSize);
      allocator.freed_page_blocks_[i].ptr = nullptr;
      allocator.freed_page_blocks_[i].ByteSize = 0;
    }
    allocator.freed_page_block_count_ = 0;
    allocator.cached_total_ = 0;
  }
#endif

  static void UpdateSlackStat(MemoryAllocatorBinned& allocator) {
#if STATS
    size_t LocalWaste = allocator.waste_current_;
    double Waste = 0.0;
    for (int32 pool_index = 0; pool_index < POOL_COUNT; pool_index++) {
      CPoolTable& table = allocator.pool_table_[pool_index];

      Waste += ((double)table.total_waste / (double)table.total_request_count) *
               (double)table.active_request_count;
      Waste += table.active_pool_count *
               (BINNED_ALLOC_POOL_SIZE -
                ((BINNED_ALLOC_POOL_SIZE / table.BlockSize) * table.BlockSize));
    }
    LocalWaste += (uint32)Waste;
    allocator.slack_current_ =
        allocator.os_current_ - LocalWaste - allocator.used_current_;
#endif  // STATS
  }
};

void MemoryAllocatorBinned::GetAllocatorStats(GenericMemoryStats& out_stats) {
  Malloc::GetAllocatorStats(out_stats);

#if STATS
  size_t local_os_current = 0;
  size_t local_os_peak = 0;
  size_t local_waste_count = 0;
  size_t local_waste_peak = 0;
  size_t local_used_current = 0;
  size_t local_used_peak = 0;
  size_t local_current_allocs = 0;
  size_t local_total_allocs = 0;
  size_t local_slack_current = 0;

  {
#ifdef USE_INTERNAL_LOCKS
    CScopedLock ScopedLock(access_mutex_);
#endif

    Private::UpdateSlackStat(*this);

    // Copy memory stats.
    local_os_current = os_current_;
    local_os_peak = os_peak_;
    local_waste_count = waste_current_;
    local_waste_peak = waste_peak_;
    local_used_current = used_current_;
    local_used_peak = used_peak_;
    local_current_allocs = current_alloc_count_;
    local_total_allocs = total_alloc_count_;
    local_slack_current = slack_current_;
  }

  // Malloc binned stats.
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_OsCurrent), local_os_current);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_OsPeak), local_os_peak);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_WasteCurrent),
                local_waste_count);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_WastePeak), local_waste_peak);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_UsedCurrent),
                local_used_current);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_UsedPeak), local_used_peak);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_CurrentAllocs),
                local_current_allocs);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_TotalAllocs),
                local_total_allocs);
  out_stats.Add(GET_STATDESCRIPTION(STAT_Binned_SlackCurrent),
                local_slack_current);
#endif  // STATS
}

void MemoryAllocatorBinned::InitializeStatsMetadata() {
  Malloc::InitializeStatsMetadata();

  // Initialize stats metadata here instead of UpdateStats.
  // Mostly to avoid dead-lock when stats malloc profiler is enabled.
  GET_STATCNAME(STAT_Binned_OsCurrent);
  GET_STATCNAME(STAT_Binned_OsPeak);
  GET_STATCNAME(STAT_Binned_WasteCurrent);
  GET_STATCNAME(STAT_Binned_WastePeak);
  GET_STATCNAME(STAT_Binned_UsedCurrent);
  GET_STATCNAME(STAT_Binned_UsedPeak);
  GET_STATCNAME(STAT_Binned_CurrentAllocs);
  GET_STATCNAME(STAT_Binned_TotalAllocs);
  GET_STATCNAME(STAT_Binned_SlackCurrent);
}

MemoryAllocatorBinned::MemoryAllocatorBinned(uint32 InPageSize,
                                             uint64 AddressLimit)
    : table_address_limit_(AddressLimit)
#ifdef USE_LOCKFREE_DELETE
      ,
      pending_free_list_(nullptr),
      flushing_frees_(false),
      done_flush_list_init_(false)
#endif
      ,
      hash_buckets_(nullptr),
      hash_bucket_free_list_(nullptr),
      PAGE_SIZE(InPageSize)
#ifdef CACHE_FREED_OS_ALLOCS
      ,
      freed_page_block_count_(0),
      cached_total_(0)
#endif
#if STATS
      ,
      os_current_(0),
      os_peak_(0),
      waste_current_(0),
      waste_peak_(0),
      used_current_(0),
      used_peak_(0),
      current_alloc_count_(0),
      total_alloc_count_(0),
      slack_current_(0),
      mem_time_(0.0)
#endif
{
  fun_check(!(PAGE_SIZE & (PAGE_SIZE - 1)));
  fun_check(!(AddressLimit & (AddressLimit - 1)));
  fun_check(PAGE_SIZE <= 65536);  // There is internal limit on page size of 64k
  fun_check(AddressLimit >
            PAGE_SIZE);  // Check to catch 32 bit overflow in AddressLimit

  // Shift to get the reference from the indirect tables
  pool_bit_shift_ = CPlatformMath::CeilLogTwo(PAGE_SIZE);
  indect_pool_block_shift_ =
      CPlatformMath::CeilLogTwo(PAGE_SIZE / sizeof(CPoolInfo));
  indect_pool_block_size_ = PAGE_SIZE / sizeof(CPoolInfo);

  max_hash_buckets_ =
      AddressLimit >> (indect_pool_block_shift_ + pool_bit_shift_);
  max_hash_bucket_bits_ = CPlatformMath::CeilLogTwo(max_hash_buckets_);
  max_hash_bucket_waste_ = (max_hash_buckets_ * sizeof(CPoolHashBucket)) / 1024;
  max_book_keeping_overhead_ =
      ((AddressLimit / PAGE_SIZE) * sizeof(CPoolHashBucket)) / (1024 * 1024);
  // Shift required to get required hash table key.
  hash_key_shift_ = pool_bit_shift_ + indect_pool_block_shift_;
  // Used to mask off the bits that have been used to lookup the indirect table
  pool_mask_ = ((1ull << (hash_key_shift_ - pool_bit_shift_)) - 1);
  binned_size_limit_ = Private::PAGE_SIZE_LIMIT / 2;
  binned_os_table_index_ =
      binned_size_limit_ + EXTENDED_PAGE_POOL_ALLOCATION_COUNT;

  fun_check((binned_size_limit_ & (binned_size_limit_ - 1)) == 0);

  // Init tables.
  os_table_.first_pool = nullptr;
  os_table_.exhausted_pool = nullptr;
  os_table_.BlockSize = 0;

  // The following options are not valid for page sizes less than 64k. They are
  // here to reduce waste
  page_pool_table_[0].first_pool = nullptr;
  page_pool_table_[0].exhausted_pool = nullptr;
  page_pool_table_[0].BlockSize =
      PAGE_SIZE == Private::PAGE_SIZE_LIMIT
          ? binned_size_limit_ + (binned_size_limit_ / 2)
          : 0;

  page_pool_table_[1].first_pool = nullptr;
  page_pool_table_[1].exhausted_pool = nullptr;
  page_pool_table_[1].BlockSize = PAGE_SIZE == Private::PAGE_SIZE_LIMIT
                                      ? PAGE_SIZE + binned_size_limit_
                                      : 0;

  // Block sizes are based around getting the maximum amount of allocations per
  // pool, with as little alignment waste as possible. Block sizes should be
  // close to even divisors of the POOL_SIZE, and well distributed. They must be
  // 16-byte aligned as well.
  static const uint32 BlockSizes[POOL_COUNT] = {
      8,    16,   32,   48,   64,    80,    96,    112,   128,  160,  192,
      224,  256,  288,  320,  384,   448,   512,   576,   640,  704,  768,
      896,  1024, 1168, 1360, 1632,  2048,  2336,  2720,  3264, 4096, 4672,
      5456, 6544, 8192, 9360, 10912, 13104, 16384, 21840, 32768};

  for (uint32 i = 0; i < POOL_COUNT; i++) {
    pool_table_[i].first_pool = nullptr;
    pool_table_[i].exhausted_pool = nullptr;
    pool_table_[i].BlockSize = BlockSizes[i];
#if STATS
    pool_table_[i].min_request = pool_table_[i].BlockSize;
#endif
  }

  for (uint32 i = 0; i < MAX_POOLED_ALLOCATION_SIZE; i++) {
    uint32 index = 0;
    while (pool_table_[index].BlockSize < i) {
      ++index;
    }
    CHECK_SLOW(index < POOL_COUNT);
    mem_size_to_pool_table_[i] = &pool_table_[index];
  }

  mem_size_to_pool_table_[binned_size_limit_] = &page_pool_table_[0];
  mem_size_to_pool_table_[binned_size_limit_ + 1] = &page_pool_table_[1];

  fun_check(MAX_POOLED_ALLOCATION_SIZE - 1 ==
            pool_table_[POOL_COUNT - 1].BlockSize);
}

MemoryAllocatorBinned::~MemoryAllocatorBinned() {}

bool MemoryAllocatorBinned::IsInternallyThreadSafe() const {
#ifdef USE_INTERNAL_LOCKS
  return true;
#else
  return false;
#endif
}

void* MemoryAllocatorBinned::Malloc(size_t size, uint32 alignment) {
#ifdef USE_COARSE_GRAIN_LOCKS
  CScopedLock ScopedLock(&access_mutex_);
#endif

  Private::FlushPendingFrees(*this);

  // Handle DEFAULT_ALIGNMENT for binned allocator.
  if (alignment == DEFAULT_ALIGNMENT) {
    alignment = Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT;
  }

  alignment = MathBase::Max<uint32>(
      alignment, Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT);
  size_t SpareBytesCount =
      MathBase::Min<size_t>(Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT, size);
  size = MathBase::Max<size_t>(pool_table_[0].BlockSize,
                               size + (alignment - SpareBytesCount));
  MEM_TIME(mem_time_ -= SystemTime::Seconds());

  BINNED_INCREMENT_STATCOUNTER(current_alloc_count_);
  BINNED_INCREMENT_STATCOUNTER(total_alloc_count_);

  CFreeMem* Free;
  if (size < binned_size_limit_) {
    // Allocate from pool.
    CPoolTable* table = mem_size_to_pool_table_[size];
#ifdef USE_FINE_GRAIN_LOCKS
    CScopedLock TableLock(table->CriticalSection);
#endif
    CHECK_SLOW(size <= table->BlockSize);

    Private::TrackStats(table, size);

    CPoolInfo* pool = table->first_pool;
    if (pool == nullptr) {
      pool = Private::AllocatePoolMemory(
          *this, table, Private::BINNED_ALLOC_POOL_SIZE /*PAGE_SIZE*/, size);
    }

    Free = Private::AllocateBlockFromPool(*this, table, pool, alignment);
  } else if (((size >= binned_size_limit_ &&
               size <= page_pool_table_[0].BlockSize) ||
              (size > PAGE_SIZE && size <= page_pool_table_[1].BlockSize))) {
    // bucket in a pool of 3*PAGE_SIZE or 6*PAGE_SIZE
    uint32 BinType = size < PAGE_SIZE ? 0 : 1;
    uint32 PageCount = 3 * BinType + 3;
    CPoolTable* table = &page_pool_table_[BinType];
#ifdef USE_FINE_GRAIN_LOCKS
    CScopedLock TableLock(table->CriticalSection);
#endif
    CHECK_SLOW(size <= table->BlockSize);

    Private::TrackStats(table, size);

    CPoolInfo* pool = table->first_pool;
    if (pool == nullptr) {
      pool = Private::AllocatePoolMemory(*this, table, PageCount * PAGE_SIZE,
                                         binned_size_limit_ + BinType);
    }

    Free = Private::AllocateBlockFromPool(*this, table, pool, alignment);
  } else {
    // Use OS for large allocations.
    uintptr_t AlignedSize = Align(size, PAGE_SIZE);
    size_t ActualPoolSize;  // TODO: use this to reduce waste?
    Free = (CFreeMem*)Private::OSAlloc(*this, AlignedSize, ActualPoolSize);
    if (Free == nullptr) {
      Private::OutOfMemory(AlignedSize);
    }

    void* aligned_free = Align(Free, alignment);

    // Create indirect.
    CPoolInfo* pool;
    {
#ifdef USE_FINE_GRAIN_LOCKS
      CScopedLock PoolInfoLock(access_mutex_);
#endif
      pool = Private::GetPoolInfo(*this, (uintptr_t)Free);

      if ((uintptr_t)Free !=
          ((uintptr_t)aligned_free & ~((uintptr_t)PAGE_SIZE - 1))) {
        // Mark the CPoolInfo for aligned_free to jump back to the CPoolInfo for
        // ptr.
        for (uintptr_t i = (uintptr_t)PAGE_SIZE, offset = 0; i < AlignedSize;
             i += PAGE_SIZE, ++offset) {
          CPoolInfo* TrailingPool =
              Private::GetPoolInfo(*this, ((uintptr_t)Free) + i);
          fun_check(TrailingPool);
          // Set trailing pools to point back to first pool
          TrailingPool->SetAllocationSizes(0, 0, offset,
                                           binned_os_table_index_);
        }
      }
    }
    Free = (CFreeMem*)aligned_free;
    pool->SetAllocationSizes(size, AlignedSize, binned_os_table_index_,
                             binned_os_table_index_);
    BINNED_PEAK_STATCOUNTER(os_peak_,
                            BINNED_ADD_STATCOUNTER(os_current_, AlignedSize));
    BINNED_PEAK_STATCOUNTER(used_peak_,
                            BINNED_ADD_STATCOUNTER(used_current_, size));
    BINNED_PEAK_STATCOUNTER(
        waste_peak_,
        BINNED_ADD_STATCOUNTER(waste_current_, (int64)(AlignedSize - size)));
  }

  MEM_TIME(mem_time_ += SystemTime::Seconds());
  return Free;
}

void* MemoryAllocatorBinned::Realloc(void* ptr, size_t new_size,
                                     uint32 alignment) {
  // Handle DEFAULT_ALIGNMENT for binned allocator.
  if (alignment == DEFAULT_ALIGNMENT) {
    alignment = Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT;
  }

  alignment = MathBase::Max<uint32>(
      alignment, Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT);
  const uint32 NewSizeUnmodified = new_size;
  size_t SpareBytesCount = MathBase::Min<size_t>(
      Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT, new_size);
  if (new_size > 0) {
    new_size = MathBase::Max<size_t>(pool_table_[0].BlockSize,
                                     new_size + (alignment - SpareBytesCount));
  }
  MEM_TIME(mem_time_ -= SystemTime::Seconds());
  uintptr_t base_ptr;
  void* new_ptr = ptr;
  if (ptr && new_size > 0) {
    CPoolInfo* pool = Private::FindPoolInfo(*this, (uintptr_t)ptr, base_ptr);

    if (pool->table_index < binned_os_table_index_) {
      // Allocated from pool, so grow or shrink if necessary.
      fun_check(pool->table_index >
                0);  // it isn't possible to allocate a size of 0, Malloc will
                     // increase the size to DEFAULT_BINNED_ALLOCATOR_ALIGNMENT
      if (NewSizeUnmodified >
              mem_size_to_pool_table_[pool->table_index]->BlockSize ||
          NewSizeUnmodified <=
              mem_size_to_pool_table_[pool->table_index - 1]->BlockSize) {
        new_ptr = Malloc(NewSizeUnmodified, alignment);
        UnsafeMemory::Memcpy(
            new_ptr, ptr,
            MathBase::Min<size_t>(
                NewSizeUnmodified,
                mem_size_to_pool_table_[pool->table_index]->BlockSize -
                    (alignment - SpareBytesCount)));
        Free(ptr);
      } else if (((uintptr_t)ptr & (uintptr_t)(alignment - 1)) != 0) {
        new_ptr = Align(ptr, alignment);
        UnsafeMemory::Memmove(new_ptr, ptr, new_size);
      }
    } else {
      // Allocated from OS.
      if (new_size > pool->GetOsBytes(PAGE_SIZE, binned_os_table_index_) ||
          new_size * 3 <
              pool->GetOsBytes(PAGE_SIZE, binned_os_table_index_) * 2) {
        // Grow or shrink.
        new_ptr = Malloc(NewSizeUnmodified, alignment);
        UnsafeMemory::Memcpy(
            new_ptr, ptr,
            MathBase::Min<size_t>(NewSizeUnmodified, pool->GetBytes()));
        Free(ptr);
      } else {
// need a lock to cover the SetAllocationSizes()
#ifdef USE_FINE_GRAIN_LOCKS
        CScopedLock PoolInfoLock(access_mutex_);
#endif
        int32 UsedChange = (new_size - pool->GetBytes());

        // Keep as-is, reallocation isn't worth the overhead.
        BINNED_ADD_STATCOUNTER(used_current_, UsedChange);
        BINNED_PEAK_STATCOUNTER(used_peak_, used_current_);
        BINNED_ADD_STATCOUNTER(waste_current_, (pool->GetBytes() - new_size));
        pool->SetAllocationSizes(
            NewSizeUnmodified,
            pool->GetOsBytes(PAGE_SIZE, binned_os_table_index_),
            binned_os_table_index_, binned_os_table_index_);
      }
    }
  } else if (ptr == nullptr) {
    new_ptr = Malloc(NewSizeUnmodified, alignment);
  } else {
    Free(ptr);
    new_ptr = nullptr;
  }

  MEM_TIME(mem_time_ += SystemTime::Seconds());
  return new_ptr;
}

void MemoryAllocatorBinned::Free(void* ptr) {
  if (ptr == nullptr) {
    return;
  }

  Private::PushFreeLockless(*this, ptr);
}

bool MemoryAllocatorBinned::GetAllocationSize(void* reported_ptr,
                                              size_t& out_allocation_size) {
  if (reported_ptr == nullptr) {
    return false;
  }

  uintptr_t base_ptr;
  CPoolInfo* pool =
      Private::FindPoolInfo(*this, (uintptr_t)reported_ptr, base_ptr);
  out_allocation_size =
      pool->table_index < binned_os_table_index_
          ? mem_size_to_pool_table_[pool->table_index]->BlockSize
          : pool->GetBytes();
  return true;
}

bool MemoryAllocatorBinned::ValidateHeap() {
#ifdef USE_COARSE_GRAIN_LOCKS
  CScopedLock ScopedLock(&access_mutex_);
#endif
  for (int32 i = 0; i < POOL_COUNT; i++) {
    CPoolTable* table = &pool_table_[i];

#ifdef USE_FINE_GRAIN_LOCKS
    CScopedLock TableLock(table->CriticalSection);
#endif

    for (CPoolInfo** pool_ptr = &table->first_pool; *pool_ptr;
         pool_ptr = &(*pool_ptr)->next) {
      CPoolInfo* pool = *pool_ptr;
      fun_check(pool->prev_link == pool_ptr);
      fun_check(pool->first_mem);
      for (CFreeMem* Free = pool->first_mem; Free; Free = Free->next) {
        fun_check(Free->free_block_count > 0);
      }
    }

    for (CPoolInfo** pool_ptr = &table->exhausted_pool; *pool_ptr;
         pool_ptr = &(*pool_ptr)->next) {
      CPoolInfo* pool = *pool_ptr;
      fun_check(pool->prev_link == pool_ptr);
      fun_check(pool->first_mem == nullptr);
    }
  }

  return true;
}

void MemoryAllocatorBinned::UpdateStats() {
  Malloc::UpdateStats();

#if STATS
  size_t local_os_current = 0;
  size_t local_os_peak = 0;
  size_t local_waste_count = 0;
  size_t local_waste_peak = 0;
  size_t local_used_current = 0;
  size_t local_used_peak = 0;
  size_t local_current_allocs = 0;
  size_t local_total_allocs = 0;
  size_t local_slack_current = 0;

  {
#ifdef USE_INTERNAL_LOCKS
    CScopedLock ScopedLock(access_mutex_);
#endif
    Private::UpdateSlackStat(*this);

    // Copy memory stats.
    local_os_current = os_current_;
    local_os_peak = os_peak_;
    local_waste_count = waste_current_;
    local_waste_peak = waste_peak_;
    local_used_current = used_current_;
    local_used_peak = used_peak_;
    local_current_allocs = current_alloc_count_;
    local_total_allocs = total_alloc_count_;
    local_slack_current = slack_current_;
  }

  SET_MEMORY_STAT(STAT_Binned_OsCurrent, local_os_current);
  SET_MEMORY_STAT(STAT_Binned_OsPeak, local_os_peak);
  SET_MEMORY_STAT(STAT_Binned_WasteCurrent, local_waste_count);
  SET_MEMORY_STAT(STAT_Binned_WastePeak, local_waste_peak);
  SET_MEMORY_STAT(STAT_Binned_UsedCurrent, local_used_current);
  SET_MEMORY_STAT(STAT_Binned_UsedPeak, local_used_peak);
  SET_DWORD_STAT(STAT_Binned_CurrentAllocs, local_current_allocs);
  SET_DWORD_STAT(STAT_Binned_TotalAllocs, local_total_allocs);
  SET_MEMORY_STAT(STAT_Binned_SlackCurrent, local_slack_current);
#endif
}

void MemoryAllocatorBinned::DumpAllocatorStats(class Printer& ar) {
  CBufferedPrinter BufferedOutput;
  {
#ifdef USE_COARSE_GRAIN_LOCKS
    CScopedLock ScopedLock(&access_mutex_);
#endif

    ValidateHeap();

#if STATS
    Private::UpdateSlackStat(*this);

#if !NO_LOGGING
    // This is all of the memory including stuff too big for the pools
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("allocator Stats for %s:"), GetDescriptiveName());
    // Waste is the total overhead of the memory system
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Current Memory %.2f MB used, plus %.2f MB waste"),
        used_current_ / (1024.0f * 1024.0f),
        (os_current_ - used_current_) / (1024.0f * 1024.0f));
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Peak Memory %.2f MB used, plus %.2f MB waste"),
        used_peak_ / (1024.0f * 1024.0f),
        (os_peak_ - used_peak_) / (1024.0f * 1024.0f));

    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Current OS Memory %.2f MB, peak %.2f MB"),
        os_current_ / (1024.0f * 1024.0f), os_peak_ / (1024.0f * 1024.0f));
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Current Waste %.2f MB, peak %.2f MB"),
        waste_current_ / (1024.0f * 1024.0f),
        waste_peak_ / (1024.0f * 1024.0f));
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Current Used %.2f MB, peak %.2f MB"),
        used_current_ / (1024.0f * 1024.0f), used_peak_ / (1024.0f * 1024.0f));
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Current Slack %.2f MB"), slack_current_ / (1024.0f * 1024.0f));

    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Allocs      % 6i Current / % 6i Total"), current_alloc_count_,
        total_alloc_count_);
    MEM_TIME(BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Seconds     % 5.3f"), mem_time_));
    MEM_TIME(BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("MSec/Allc   % 5.5f"), 1000.0 * mem_time_ / MemAllocs));

    // This is the memory tracked inside individual allocation pools
    BufferedOutput.CategorizedPrintf(LogMemory.GetCategoryName(),
                                     ELogVerbosity::Info, TEXT(""));
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Block size Num Pools Max Pools Cur Allocs Total Allocs Min Req "
             "Max Req Mem Used Mem Slack Mem Waste Efficiency"));
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("---------- --------- --------- ---------- ------------ ------- "
             "------- -------- --------- --------- ----------"));

    uint32 total_memory = 0;
    uint32 total_waste = 0;
    uint32 total_active_requests = 0;
    uint32 total_requests = 0;
    uint32 total_pools = 0;
    uint32 total_slack = 0;

    CPoolTable* table = nullptr;
    for (int32 i = 0;
         i < binned_size_limit_ + EXTENDED_PAGE_POOL_ALLOCATION_COUNT; i++) {
      if (table == mem_size_to_pool_table_[i] ||
          mem_size_to_pool_table_[i]->BlockSize == 0) {
        continue;
      }

      table = mem_size_to_pool_table_[i];

#ifdef USE_FINE_GRAIN_LOCKS
      CScopedLock TableLock(table->CriticalSection);
#endif

      uint32 total_alloc_size = (table->BlockSize > binned_size_limit_
                                     ? (((3 * (i - binned_size_limit_)) + 3) *
                                        Private::BINNED_ALLOC_POOL_SIZE)
                                     : Private::BINNED_ALLOC_POOL_SIZE);
      // The amount of memory allocated from the OS
      uint32 mem_allocated =
          (table->active_pool_count * total_alloc_size) / 1024;
      // Amount of memory actually in use by allocations
      uint32 mem_used = (table->BlockSize * table->active_request_count) / 1024;
      // Wasted memory due to pool size alignment
      uint32 pool_mem_waste =
          table->active_pool_count *
          (total_alloc_size -
           ((total_alloc_size / table->BlockSize) * table->BlockSize)) /
          1024;
      // Wasted memory due to individual allocation alignment. This is an
      // estimate.
      uint32 mem_waste = (uint32)(((double)table->total_waste /
                                   (double)table->total_request_count) *
                                  (double)table->active_request_count) /
                             1024 +
                         pool_mem_waste;
      // Memory that is reserved in active pools and ready for future use
      uint32 mem_slack = mem_allocated - mem_used - pool_mem_waste;

      BufferedOutput.CategorizedPrintf(
          LogMemory.GetCategoryName(), ELogVerbosity::Info,
          TEXT("% 10i % 9i % 9i % 10i % 12i % 7i % 7i % 7iK % 8iK % 8iK % "
               "9.2f%%"),
          table->BlockSize, table->active_pool_count, table->max_active_pools,
          table->active_request_count, (uint32)table->total_request_count,
          table->min_request, table->max_request, mem_used, mem_slack,
          mem_waste,
          mem_allocated ? 100.0f * (mem_allocated - mem_waste) / mem_allocated
                        : 100.0f);

      total_memory += mem_allocated;
      total_waste += mem_waste;
      total_slack += mem_slack;
      total_active_requests += table->active_request_count;
      total_requests += table->total_request_count;
      total_pools += table->active_pool_count;
    }

    BufferedOutput.CategorizedPrintf(LogMemory.GetCategoryName(),
                                     ELogVerbosity::Info, TEXT(""));
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("%iK allocated in pools (with %iK slack and %iK waste). "
             "Efficiency %.2f%%"),
        total_memory, total_slack, total_waste,
        total_memory ? 100.0f * (total_memory - total_waste) / total_memory
                     : 100.0f);
    BufferedOutput.CategorizedPrintf(
        LogMemory.GetCategoryName(), ELogVerbosity::Info,
        TEXT("Allocations %i Current / %i Total (in %i pools)"),
        total_active_requests, total_requests, total_pools);
    BufferedOutput.CategorizedPrintf(LogMemory.GetCategoryName(),
                                     ELogVerbosity::Info, TEXT(""));
#endif
#endif
  }

  BufferedOutput.RedirectTo(ar);
}

const char* MemoryAllocatorBinned::GetDescriptiveName() { return "binned"; }

}  // namespace fun
