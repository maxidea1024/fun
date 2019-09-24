#include "fun/base/malloc_jemalloc.h"

// Only use for supported platforms
#if FUN_SUPPORTS_JEMALLOC

#define JEMALLOC_NO_DEMANGLE // use stable function names prefixed with je_
#include "jemalloc.h"

#ifndef JEMALLOC_VERSION
#error JEMALLOC_VERSION not defined!
#endif

// Value we fill a memory block with after it is free, in FUN_BUILD_DEBUG and FUN_BUILD_DEVELOPMENT
#define DEBUG_FILL_FREED  (0xdd)
// Value we fill a new memory block with, in FUN_BUILD_DEBUG and FUN_BUILD_DEVELOPMENT
#define DEBUG_FILL_NEW    (0xcd)

namespace fun {

void* MemoryAllocatorJemalloc::Malloc(size_t size, uint32 alignment) {
  MEM_TIME(mem_time_ -= SystemTime::Seconds());

  void* ptr = nullptr;
  if (alignment != DEFAULT_ALIGNMENT) {
    alignment = MathBase::Max(size >= 16 ? (uint32)16 : (uint32)8, alignment);

    // use aligned_alloc when allocating exact multiplies of an alignment
    // use the fact that alignment is power of 2 and avoid %, but check it
    fun_check_dbg(0 == (alignment & (alignment - 1)));
    if ((size & (alignment - 1)) == 0) {
      ptr = je_aligned_alloc(alignment, size);
    } else if (je_posix_memalign(&ptr, alignment, size) != 0) {
      ptr = nullptr; // explicitly nullify
    }
  } else {
    ptr = je_malloc(size);
  }

  if (ptr == nullptr) {
    OutOfMemory();
  }

#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
  else if (size > 0) {
    UnsafeMemory::Memset(ptr, DEBUG_FILL_NEW, size);
  }
#endif

  MEM_TIME(mem_time_ += SystemTime::Seconds());
  return ptr;
}

void* MemoryAllocatorJemalloc::Realloc(void* ptr, size_t new_size, uint32 alignment) {
  fun_check_msg(alignment == DEFAULT_ALIGNMENT, "alignment with realloc is not supported with MemoryAllocatorJemalloc");
  MEM_TIME(mem_time_ -= SystemTime::Seconds());

  size_t old_size = 0;
  if (ptr != nullptr) {
    old_size = je_malloc_usable_size(ptr);
#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
    if (new_size < old_size) {
      UnsafeMemory::Memset((uint8*)ptr + new_size, DEBUG_FILL_FREED, old_size - new_size);
    }
#endif
  }

  void* new_ptr = nullptr;
  if (alignment != DEFAULT_ALIGNMENT) {
    new_ptr = Malloc(new_size, alignment);
    if (ptr) {
      UnsafeMemory::Memcpy(new_ptr, ptr, old_size);
      Free(ptr);
    }
  } else {
    new_ptr = je_realloc(ptr, new_size);
  }

#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
  if (new_ptr && new_size > old_size) {
    UnsafeMemory::Memset((uint8*)new_ptr + old_size, DEBUG_FILL_NEW, new_size - old_size);
  }
#endif

  MEM_TIME(mem_time_ += SystemTime::Seconds());
  return new_ptr;
}

void MemoryAllocatorJemalloc::Free(void* ptr) {
  if (ptr == nullptr) {
    return;
  }

  MEM_TIME(mem_time_ -= SystemTime::Seconds());

#if FUN_BUILD_DEBUG || FUN_BUILD_DEVELOPMENT
  UnsafeMemory::Memset(ptr, DEBUG_FILL_FREED, je_malloc_usable_size(ptr));
#endif
  je_free(ptr);

  MEM_TIME(mem_time_ += SystemTime::Seconds());
}

namespace {

void JemallocStatsPrintCallback(void* user_data, const char* string) {
  Printer* out = reinterpret_cast<Printer*>(user_data);
  fun_check_ptr(out);
  if (out) {
    String sanitized(string);
    sanitized.ReplaceInline("\n", "", CaseSensitivity::CaseSensitive);
    sanitized.ReplaceInline("\r", "", CaseSensitivity::CaseSensitive);
    out->Printf(sanitized);
  }
}

} // namespace

void MemoryAllocatorJemalloc::DumpAllocatorStats(Printer& out) {
  MEM_TIME(out.Printf("Seconds     % 5.3f", mem_time_));

  // "g" omits static stats, "a" omits per-arena stats, "l" omits large object stats
  // see http://www.canonware.com/download/jemalloc/jemalloc-latest/doc/jemalloc.html for detailed opts explanation.
  je_malloc_stats_print(JemallocStatsPrintCallback, &out, "gla");
}

bool MemoryAllocatorJemalloc::GetAllocationSize(void* reported_ptr, size_t& out_allocation_size) {
  out_allocation_size = je_malloc_usable_size(reported_ptr);
  return true;
}

} // namespace fun

#undef DEBUG_FILL_FREED
#undef DEBUG_FILL_NEW

#endif // FUN_SUPPORTS_JEMALLOC
