#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"

#if _MSC_VER || FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
# define FUN_USE_ALIGNED_MALLOC  1
#else
//@todo gcc: this should be implemented more elegantly on other platforms
# define FUN_USE_ALIGNED_MALLOC  0
#endif

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
#include "mach/mach.h"
#endif

namespace fun {

/**
 * ANSI C memory allocator.
 */
class MemoryAllocatorAnsi : public MemoryAllocator {
 public:
  MemoryAllocatorAnsi() {
#if FUN_PLATFORM_WINDOWS_FAMILY
    // Enable low fragmentation heap - http://msdn2.microsoft.com/en-US/library/aa366750.aspx
    intptr_t crt_heap_handle = _get_heap_handle();
    ULONG enable_LFH = 2;
    HeapSetInformation((PVOID)crt_heap_handle, HeapCompatibilityInformation, &enable_LFH, sizeof(enable_LFH));
#endif
  }

  // MemoryAllocator interface

  void* Malloc(size_t size, uint32 alignment) override {
    IncrementTotalMallocCalls();

    alignment = MathBase::Max(size >= 16 ? (uint32)16 : (uint32)8, alignment);

#if FUN_USE_ALIGNED_MALLOC
    void* result = _aligned_malloc(size, alignment);
#else
    void* ptr = malloc(size + alignment + sizeof(void*) + sizeof(size_t));
    fun_check_ptr(ptr);
    void* result = Align((uint8*)ptr + sizeof(void*) + sizeof(size_t), alignment);
    *((void**)((uint8*)result - sizeof(void*))) = ptr;
    *((size_t*)((uint8*)result - sizeof(void*) - sizeof(size_t))) = size;
#endif

    if (result == nullptr) {
      PlatformMemory::OnOutOfMemory(size, alignment);
    }

    return result;
  }

  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override {
    IncrementTotalReallocCalls();

    void* result;
    alignment = MathBase::Max(new_size >= 16 ? (uint32)16 : (uint32)8, alignment);

#if FUN_USE_ALIGNED_MALLOC
    if (ptr && new_size > 0) {
      result = _aligned_realloc(ptr, new_size, alignment);
    } else if (ptr == nullptr) {
      result = _aligned_malloc(new_size, alignment);
    } else {
      _aligned_free(ptr);
      result = nullptr;
    }
#else
    if (ptr && new_size > 0) {
      // Can't use realloc as it might screw with alignment.
      result = Malloc(new_size, alignment);
      size_t ptr_size = 0;
      GetAllocationSize(ptr, ptr_size);
      UnsafeMemory::Memcpy(result, ptr, MathBase::Min(new_size, ptr_size));
      Free(ptr);
    } else if (ptr == nullptr) {
      result = Malloc(new_size, alignment);
    } else {
      free(*((void**)((uint8*)ptr - sizeof(void*))));
      result = nullptr;
    }
#endif
    if (result == nullptr && new_size != 0) {
      PlatformMemory::OnOutOfMemory(new_size, alignment);
    }

    return result;
  }

  void Free(void* ptr) override {
    IncrementTotalFreeCalls();

#if FUN_USE_ALIGNED_MALLOC
    _aligned_free(ptr);
#else
    if (ptr) {
      free(*((void**)((uint8*)ptr - sizeof(void*))));
    }
#endif
  }

  bool GetAllocationSize(void* original, size_t& out_allocation_size) override {
    if (original == nullptr) {
      return false;
    }

#if FUN_USE_ALIGNED_MALLOC
    out_allocation_size = _aligned_msize(original, 16, 0); // Assumes alignment of 16
#else
    out_allocation_size = *((size_t*)((uint8*)original - sizeof(void*) - sizeof(size_t)));
#endif
    return true;
  }

  bool IsInternallyThreadSafe() const override {
#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
    return true;
#else
    return false;
#endif
  }

  bool ValidateHeap() override {
#if FUN_PLATFORM_WINDOWS_FAMILY
    int32 result = _heapchk();
    fun_check(result != _HEAPBADBEGIN);
    fun_check(result != _HEAPBADNODE);
    fun_check(result != _HEAPBADPTR);
    fun_check(result != _HEAPEMPTY);
    fun_check(result == _HEAPOK);
#else
    return true;
#endif
    return true;
  }

  const char* GetDescriptiveName() override { return "ANSI"; }
};

} // namespace fun
