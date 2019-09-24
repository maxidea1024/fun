#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"

namespace fun {

/** Governs when malloc that poisons the allocations is enabled. */
#ifndef FUN_USE_MALLOC_FILL_BYTES
#define FUN_USE_MALLOC_FILL_BYTES                                         \
  ((FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT) && !WITH_EDITORONLY_DATA && \
   !PLATFORM_USES_FIXED_GMalloc_CLASS)
#endif

/** Value that a freed memory block will be filled with when
 * FUN_USE_MALLOC_FILL_BYTES is defined. */
#define DEBUG_FILL_FREED (0xdd)

/** Value that a new memory block will be filled with when
 * FUN_USE_MALLOC_FILL_BYTES is defined. */
#define DEBUG_FILL_NEW (0xcd)

/**
 * MemoryAllocator proxy that poisons new and freed allocations, helping to
 * catch code that relies on uninitialized or freed memory.
 */
class MemoryAllocatorPoisonProxy : public MemoryAllocator {
 public:
  // MemoryAllocator interface begin
  explicit MemoryAllocatorPoisonProxy(MemoryAllocator* malloc)
      : inner_malloc_(malloc) {
    fun_check_ptr(inner_malloc_);
  }

  void InitializeStatsMetadata() override {
    inner_malloc_->InitializeStatsMetadata();
  }

  void* Malloc(size_t size, uint32 alignment) override {
    IncrementTotalMallocCalls();

    void* result = inner_malloc_->Malloc(size, alignment);
    if (result && size > 0) {
      UnsafeMemory::Memset(result, DEBUG_FILL_NEW, size);
    }
    return result;
  }

  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override {
    // NOTE: case when Realloc returns completely new pointer
    // is not handled properly.
    // (we would like to have previous memory poisoned completely).
    //
    // This can be done by avoiding calling Realloc() of the nested allocator
    // and Malloc()/Free()'ing directly, but this is deemed unacceptable
    // from a performance/fragmentation standpoint.

    IncrementTotalReallocCalls();

    size_t old_size = 0;
    if (ptr && GetAllocationSize(ptr, old_size) && old_size > 0 &&
        old_size > new_size) {
      UnsafeMemory::Memset(static_cast<uint8*>(ptr) + new_size,
                           DEBUG_FILL_FREED, old_size - new_size);
    }

    void* result = inner_malloc_->Realloc(ptr, new_size, alignment);
    if (result && old_size > 0 && old_size < new_size) {
      UnsafeMemory::Memset(static_cast<uint8*>(result) + old_size,
                           DEBUG_FILL_NEW, new_size - old_size);
    }

    return result;
  }

  void Free(void* ptr) override {
    if (ptr) {
      IncrementTotalFreeCalls();

      size_t allocation_size;
      if (GetAllocationSize(ptr, allocation_size) && allocation_size > 0) {
        UnsafeMemory::Memset(ptr, DEBUG_FILL_FREED, allocation_size);
      }
      inner_malloc_->Free(ptr);
    }
  }

  void GetAllocatorStats(GenericMemoryStats& out_stats) override {
    inner_malloc_->GetAllocatorStats(out_stats);
  }

  void DumpAllocatorStats(Printer& out) override {
    inner_malloc_->DumpAllocatorStats(out);
  }

  bool ValidateHeap() override { return inner_malloc_->ValidateHeap(); }

  bool Exec(RuntimeEnv* env, const char* cmd, Printer& out) override {
    return inner_malloc_->Exec(env, cmd, out);
  }

  bool GetAllocationSize(void* reported_ptr,
                         size_t& out_allocation_size) override {
    return inner_malloc_->GetAllocationSize(reported_ptr, out_allocation_size);
  }

  const char* GetDescriptiveName() override {
    return inner_malloc_->GetDescriptiveName();
  }

  void Trim() override { inner_malloc_->Trim(); }

 private:
  /** Malloc we're based on, aka using under the hood */
  MemoryAllocator* inner_malloc_;
};

}  // namespace fun
