#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"
#include "fun/base/scoped_lock.h"
#include "fun/base/memory_base.h"

namespace fun {

class Printer;

/**
 * Malloc proxy that synchronizes access, making the used malloc thread safe.
 */
class MemoryAllocatorThreadSafeProxy : public MemoryAllocator {
 public:
  /**
   * Constructor for thread safe proxy malloc that takes a malloc to be used and a
   * synchronization object used via ScopedLock<FastMutex> as a parameter.
   *
   * @param malloc - MemoryAllocator that is going to be used for actual allocations.
   */
  MemoryAllocatorThreadSafeProxy(MemoryAllocator* malloc)
    : inner_malloc_(malloc) {}

  // MemoryAllocator interface

  void InitializeStatsMetadata() override {
    inner_malloc_->InitializeStatsMetadata();
  }

  void* Malloc(size_t size, uint32 alignment) override {
    IncrementTotalMallocCalls();

    ScopedLock<FastMutex> guard(mutex_);
    return inner_malloc_->Malloc(size, alignment);
  }

  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override {
    IncrementTotalReallocCalls();

    ScopedLock<FastMutex> guard(mutex_);
    return inner_malloc_->Realloc(ptr, new_size, alignment);
  }

  void Free(void* ptr) override {
    if (FUN_LIKELY(ptr)) {
      IncrementTotalFreeCalls();

      ScopedLock<FastMutex> guard(mutex_);
      inner_malloc_->Free(ptr);
    }
  }

  void GetAllocatorStats(GenericMemoryStats& out_stats) override {
    ScopedLock<FastMutex> guard(mutex_);
    inner_malloc_->GetAllocatorStats(out_stats);
  }

  void DumpAllocatorStats(Printer& out) override {
    ScopedLock<FastMutex> guard(mutex_);
    inner_malloc_->DumpAllocatorStats(out);
  }

  bool ValidateHeap() override {
    ScopedLock<FastMutex> guard(mutex_);
    return inner_malloc_->ValidateHeap();
  }

  bool Exec(RuntimeEnv* env, const char* cmd, Printer& out) override {
    ScopedLock<FastMutex> guard(mutex_);
    return inner_malloc_->Exec(env, cmd, out);
  }

  bool GetAllocationSize(void* reported_ptr, size_t& out_allocation_size) override {
    ScopedLock<FastMutex> guard(mutex_);
    return inner_malloc_->GetAllocationSize(reported_ptr, out_allocation_size);
  }

  const char* GetDescriptiveName() override {
    ScopedLock<FastMutex> guard(mutex_);
    fun_check_ptr(inner_malloc_);
    return inner_malloc_->GetDescriptiveName();
  }

 private:
  /** Malloc we're based on, aka using under the hood. */
  MemoryAllocator* inner_malloc_;

  /** Object used for synchronization via a scoped lock. */
  FastMutex mutex_;
};

} // namespace fun
