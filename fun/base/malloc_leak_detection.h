#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"
#include "fun/base/container/ansi_allocator.h"

#if FUN_MALLOC_LEAKDETECTION

namespace fun {

/**
 * Maintains a list of all pointers to currently allocated memory.
 */
class MemoryAllocatorLeakDetection {
  struct CallstackTrack {
    CallstackTrack() {
      UnsafeMemory::Memzero(this, sizeof(CallstackTrack));
    }
    static const int32 CALLSTACK_DEPTH = 32;
    uint64 callstack[CALLSTACK_DEPTH];
    uint32 frame_number;
    uint32 size;
    uint32 count;

    bool operator == (const CallstackTrack& other) const {
      bool equal = true;
      for (int32 i = 0; i < CALLSTACK_DEPTH; ++i) {
        if (callstack[i] != other.callstack[i]) {
          equal = false;
          break;
        }
      }
      return equal;
    }

    bool operator != (const CallstackTrack& other) const {
      return !(*this == other);
    }
  };

  MemoryAllocatorLeakDetection();
  ~MemoryAllocatorLeakDetection();

  void AddCallstack(CallstackTrack& callstack);
  void RemoveCallstack(CallstackTrack& callstack);
  void HandleMallocLeakCommandInternal(const Array<String>& args);

  /** List of all currently allocated pointers */
  Map<void*, CallstackTrack> open_pointers_;

  /** List of all unique callstacks with allocated memory */
  Map<uint32, CallstackTrack> unique_callstacks_;

  bool recursive_;
  bool capture_allocs_;
  bool dump_outstanding_allocs_;

  FastMutex allocated_pointers_mutex_;

 public:
  static MemoryAllocatorLeakDetection& Get();
  static void HandleMallocLeakCommand(const Array<String>& args);

  void SetAllocationCollection(bool enable);
  void DumpOpenCallstacks(uint32 filter_size = 0);
  void ClearData();

  bool Exec(RuntimeEnv* env, const UNICHAR* cmd, Printer& out);

  /** Handles new allocated pointer */
  void Malloc(void* ptr, size_t size);

  /** Handles reallocation */
  void Realloc(void* old_ptr, void* new_ptr, size_t size);

  /** Removes allocated pointer from list */
  void Free(void* ptr);
};


/**
 * A verifying proxy malloc that takes a malloc to be used and tracks
 * unique callstacks with outstanding allocations to help identify leaks.
 */
class MemoryAllocatorLeakDetectionProxy : public MemoryAllocator {
 public:
  explicit MemoryAllocatorLeakDetectionProxy(MemoryAllocator* malloc);

  void* Malloc(size_t size, uint32 alignment) override {
    CScopedLock pointers_guard(allocated_pointers_mutex_);
    void* result = inner_malloc_->Malloc(size, alignment);
    verify_.Malloc(result, size);
    return result;
  }

  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override {
    CScopedLock pointers_guard(allocated_pointers_mutex_);
    void* result = inner_malloc_->Realloc(ptr, new_size, alignment);
    verify_.Realloc(ptr, result, new_size);
    return result;
  }

  void Free(void* ptr) override {
    if (ptr) {
      CScopedLock pointers_guard(allocated_pointers_mutex_);
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

  void DumpAllocatorStats(Printer& out) override {
    verify_.DumpOpenCallstacks(1024 * 1024);
    inner_malloc_->DumpAllocatorStats(out);
  }

  bool ValidateHeap() override {
    return inner_malloc_->ValidateHeap();
  }

  bool Exec(RuntimeEnv* env, const UNICHAR* cmd, Printer& out) override {
    verify_.Exec(env, cmd, out);
    return inner_malloc_->Exec(env, cmd, out);
  }

  bool GetAllocationSize(void* original, size_t& out_allocation_size) override {
    return inner_malloc_->GetAllocationSize(original, out_allocation_size);
  }

  size_t QuantizeSize(size_t count, uint32 alignment) override {
    return inner_malloc_->QuantizeSize(count, alignment);
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

  const UNICHAR* GetDescriptiveName() override {
    return inner_malloc_->GetDescriptiveName();
  }

 private:
  /** MemoryAllocator we're based on, aka using under the hood */
  MemoryAllocator* inner_malloc_;

  /** Verifier object */
  MemoryAllocatorLeakDetection& verify_;

  FastMutex allocated_pointers_mutex_;
};

} // namespace fun

#endif //FUN_MALLOC_LEAKDETECTION
