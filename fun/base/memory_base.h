#pragma once

#include "fun/base/base.h"
#include "fun/base/atomic_counter.h"

namespace fun {

/**
 * Default allocator alignment. If the default is specified,
 * the allocator applies to engine rules.
 * Blocks >= 16 bytes will be 16-byte-aligned, Blocks < 16
 * will be 8-byte aligned. If the allocator does
 * not support allocation alignment, the alignment will be ignored.
 */
enum { DEFAULT_ALIGNMENT = 0 };

/**
 * Minimum allocator alignment
 */
enum { MIN_ALIGNMENT = 8 };

/**
 * Inherit from UseSystemMemoryAllocatorForNew if you want your objects
 * to be placed in memory alloced by the system malloc routines,
 * bypassing g_malloc. This is e.g. used by MemoryAllocator itself.
 */
class FUN_BASE_API UseSystemMemoryAllocatorForNew {
 public:
  void* operator new(size_t size);
  void operator delete(void* ptr);

  void* operator new[](size_t size);
  void operator delete[](void* ptr);
};


class FUN_BASE_API MemoryAllocator : public UseSystemMemoryAllocatorForNew {
 public:
  virtual void* Malloc(size_t count, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
  virtual void* Realloc(void* ptr, size_t new_count, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
  virtual void Free(void* ptr) = 0;
  virtual size_t QuantizeSize(size_t count, uint32 alignment) { return count; }

  virtual void Trim() {}
  virtual void SetupTlsCachesOnCurrentThread() {}
  virtual void ClearAndDisableTlsCachesOnCurrentThread() {}

  virtual bool IsInternallyThreadSafe() const { return false; }

  virtual bool ValidateHeap() { return true; }

  virtual const char* GetAllocatorName() const { return "MemoryAllocator"; }

  //TODO Stats

  static MemoryAllocator* GetGlobalMalloc();
  static MemoryAllocator** GetFixedMallocLocationPtr();

 protected:
  static AtomicCounter32 total_malloc_calls_;
  static AtomicCounter32 total_free_calls_;
  static AtomicCounter32 total_realloc_calls_;
};

} // namespace fun
