#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"
#include "fun/base/container/ansi_allocator.h"

//@maxidea: temp
//#define FUN_MALLOC_VERIFY  1

#if FUN_MALLOC_VERIFY

namespace fun {

/**
 * Maintains a list of all pointers to currently allocated memory.
 */
class MallocVerify {
 public:
  /** Handles new allocated pointer */
  void Malloc(void* ptr);

  /** Handles reallocation */
  void Realloc(void* old_ptr, void* new_ptr);

  /** Removes allocated pointer from list */
  void Free(void* ptr);

 private:
  /** List of all currently allocated pointers */
  Set<void*, DefaultKeyFuncs<void*>, AnsiSetAllocator> allocated_pointers_;
};


/**
 * A verifying proxy malloc that takes a malloc to be used and checks that the caller
 * is passing valid pointers.
 */
class MemoryAllocatorVerifyProxy : public MemoryAllocator {
 public:
  MemoryAllocatorVerifyProxy(Malloc* malloc)
    : inner_malloc_(malloc) {}

  // MemoryAllocator interface

  void* Malloc(size_t size, uint32 alignment) override {
    ScopedLock<FastMutex> guard(mutex_);
    void* result = inner_malloc_->Malloc(size, alignment);
    verify_.Malloc(result);
    return result;
  }

  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override {
    ScopedLock<FastMutex> guard(mutex_);
    void* result = inner_malloc_->Realloc(ptr, new_size, alignment);
    verify_.Realloc(ptr, result);
    return result;
  }

  void Free(void* ptr) override {
    if (ptr) {
      ScopedLock<FastMutex> guard(mutex_);
      verify_.Free(ptr);
      inner_malloc_->Free(ptr);
    }
  }

  void InitializeStatsMetadata() override {
    inner_malloc_->InitializeStatsMetadata();
  }

  void GetAllocatorStats(GenericMemoryStats& out_stats) override {
    inner_malloc_->GetAllocatorStats(out_stats);
  }

  void DumpAllocatorStats(Printer& ar) override {
    inner_malloc_->DumpAllocatorStats(ar);
  }

  bool ValidateHeap() override {
    return inner_malloc_->ValidateHeap();
  }

  bool Exec(RuntimeEnv* env, const char* cmd, Printer& out) override {
    return inner_malloc_->Exec(env, cmd, out);
  }

  bool GetAllocationSize(void* original, size_t& out_allocation_size) override {
    return inner_malloc_->GetAllocationSize(original, out_allocation_size);
  }

  size_t QuantizeSize(size_t count, uint32 alignment) override {
    return inner_malloc_->QuantizeSize(Count, alignment);
  }

  void Trim() override {
    return inner_malloc_->Trim();
  }

  void SetupTlsCachesOnCurrentThread() override {
    return inner_malloc_->SetupTlsCachesOnCurrentThread();
  }

  void ClearAndDisableTlsCachesOnCurrentThread() override {
    return inner_malloc_->ClearAndDisableTlsCachesOnCurrentThread();
  }

  const char* GetDescriptiveName() override {
    return inner_malloc_->GetDescriptiveName();
  }

 private:
  /** MemoryAllocator we're based on, aka using under the hood */
  MemoryAllocator* inner_malloc_;

  /* Verifier object */
  MallocVerify verify_;

  /** MemoryAllocator critical section */
  FastMutex mutex_;
};

} // namespace fun

#endif //FUN_MALLOC_VERIFY
