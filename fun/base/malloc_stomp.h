#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"

namespace fun {

#ifndef FUN_WITH_EDITOR
#error "FUN_WITH_EDITOR must be defined whether zero or one"
#endif

#ifndef FUN_USE_MALLOC_STOMP
#if !FUN_WITH_EDITOR && PLATFORM_DESKTOP
#define FUN_USE_MALLOC_STOMP 0
#endif
#endif

#ifndef FUN_USE_MALLOC_STOMP
#define FUN_USE_MALLOC_STOMP 0
#endif

//@maxidea: temp
//#undef FUN_USE_MALLOC_STOMP
//#define FUN_USE_MALLOC_STOMP  1

#if FUN_USE_MALLOC_STOMP

/**
 * Stomp memory allocator. It helps find the following errors:
 *  - Read or writes off the end of an allocation.
 *  - Read or writes off the beginning of an allocation.
 *  - Read or writes after freeing an allocation.
 */
class MemoryAllocatorStomp : public MemoryAllocator {
 public:
  MemoryAllocatorStomp(const bool use_underrun_mode = false)
      : use_underrun_mode_(use_underrun_mode) {}

  // MemoryAllocator interface.

  void* Malloc(size_t size, uint32 alignment) override;
  void* Realloc(void* old_ptr, size_t new_size, uint32 alignment) override;
  void Free(void* ptr) override;
  bool GetAllocationSize(void* reported_ptr,
                         size_t& out_allocation_size) override;
  void DumpAllocatorStats(Printer& out) override {}
  bool ValidateHeap() override;
  bool Exec(RuntimeEnv* env, const char* cmd, Printer& out) override {
    return false;
  }
  const char* GetDescriptiveName() override { return "Stomp"; }

 private:
#if FUN_64_BIT
  /** Expected value to be found in the sentinel. */
  static const size_t SENTINEL_EXPECTED_VALUE = 0xDEADBEEFDEADBEEF;
#else
  /** Expected value to be found in the sentinel. */
  static const size_t SENTINEL_EXPECTED_VALUE = 0xDEADBEEF;
#endif

  static const size_t PAGE_SIZE =
      4096U;  // TODO: Verify the assumption that all relevant platforms use
              // 4KiB pages.

#if FUN_PLATFORM_WINDOWS_FAMILY
  static const uint32 NO_ACCESS_PROTECT_MODE = PAGE_NOACCESS;
#elif FUN_PLATFORM_UNIX_FAMILY
  static const uint32 NO_ACCESS_PROTECT_MODE = PROT_NONE;
#else
#error The stomp allocator isn't supported in this platform.
#endif

  struct AllocationData {
    /** Pointer to the full allocation. Needed so the OS knows what to free. */
    void* full_allocation_pointer;

    /** Full size of the allocation including the extra page. */
    size_t full_size;

    /** size of the allocation requested. */
    size_t size;

    /** Sentinel used to check for underrun. */
    size_t sentinel;
  };

  /** If it is set to true, instead of focusing on overruns the allocator will
   * focus on underruns. */
  const bool use_underrun_mode_;
};

#endif  // FUN_USE_MALLOC_STOMP

}  // namespace fun
