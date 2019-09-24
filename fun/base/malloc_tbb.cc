#include "fun/base/malloc_tbb.h"

// Only use for supported platforms
#if FUN_PLATFORM_SUPPORTS_TBB && FUN_TBB_ALLOCATOR_ALLOWED

// Value we fill a memory block with after it is free, in FUN_BUILD_DEBUG
#define DEBUG_FILL_FREED (0xdd)

// Value we fill a new memory block with, in FUN_BUILD_DEBUG
#define DEBUG_FILL_NEW (0xcd)

// Statically linked tbbmalloc requires tbbmalloc_debug.lib in debug
#if FUN_BUILD_DEBUG && \
    !defined(NDEBUG)  // Use !defined(NDEBUG) to check to see if we actually are
                      // linking with Debug third party libraries
                      // (bDebugBuildsActuallyUseDebugCRT)
#ifndef TBB_USE_DEBUG
#define TBB_USE_DEBUG 1
#endif
#endif

#include <tbb/scalable_allocator.h>

#define MEM_TIME(X)

namespace fun {

void* MemoryAllocatorTBB::Malloc(size_t size, uint32 alignment) {
  IncrementTotalMallocCalls();

  MEM_TIME(mem_time_ -= SystemTime::Seconds());

  void* new_ptr = nullptr;
  if (alignment != DEFAULT_ALIGNMENT) {
    alignment = MathBase::Max(size >= 16 ? (uint32)16 : (uint32)8, alignment);
    new_ptr = scalable_aligned_malloc(size, alignment);
  } else {
    new_ptr = scalable_malloc(size);
  }

  if (new_ptr == nullptr && size > 0) {
    OutOfMemory(size, alignment);
  }
#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
  else if (size > 0) {
    UnsafeMemory::Memset(new_ptr, DEBUG_FILL_NEW, size);
  }
#endif
  MEM_TIME(mem_time_ += SystemTime::Seconds());
  return new_ptr;
}

void* MemoryAllocatorTBB::Realloc(void* ptr, size_t new_size,
                                  uint32 alignment) {
  IncrementTotalReallocCalls();

  MEM_TIME(mem_time_ -= SystemTime::Seconds())
#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
  size_t old_size = 0;
  if (ptr) {
    old_size = scalable_msize(ptr);
    if (new_size < old_size) {
      UnsafeMemory::Memset((uint8*)ptr + new_size, DEBUG_FILL_FREED,
                           old_size - new_size);
    }
  }
#endif

  void* new_ptr = nullptr;
  if (alignment != DEFAULT_ALIGNMENT) {
    alignment =
        MathBase::Max(new_size >= 16 ? (uint32)16 : (uint32)8, alignment);
    new_ptr = scalable_aligned_realloc(ptr, new_size, alignment);
  } else {
    new_ptr = scalable_realloc(ptr, new_size);
  }

#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
  if (new_ptr && new_size > old_size) {
    UnsafeMemory::Memset((uint8*)new_ptr + old_size, DEBUG_FILL_NEW,
                         new_size - old_size);
  }
#endif

  if (new_ptr == nullptr && new_size > 0) {
    OutOfMemory(new_size, alignment);
  }
  MEM_TIME(mem_time_ += SystemTime::Seconds())
  return new_ptr;
}

void MemoryAllocatorTBB::Free(void* ptr) {
  if (ptr == nullptr) {
    return;
  }

  MEM_TIME(mem_time_ -= SystemTime::Seconds())
#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
  UnsafeMemory::Memset(ptr, DEBUG_FILL_FREED, scalable_msize(ptr));
#endif
  IncrementTotalFreeCalls();
  scalable_free(ptr);

  MEM_TIME(mem_time_ += SystemTime::Seconds())
}

bool MemoryAllocatorTBB::GetAllocationSize(void* reported_ptr,
                                           size_t& out_allocation_size) {
  out_allocation_size = scalable_msize(reported_ptr);
  return true;
}

}  // namespace fun

#undef MEM_TIME

#endif  // FUN_PLATFORM_SUPPORTS_TBB && FUN_TBB_ALLOCATOR_ALLOWED
