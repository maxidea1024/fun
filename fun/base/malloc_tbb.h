#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"

namespace fun {

#if FUN_PLATFORM_SUPPORTS_TBB && FUN_TBB_ALLOCATOR_ALLOWED // Only use for supported platforms

/**
 * TBB 64-bit scalable memory allocator.
 */
class MemoryAllocatorTBB : public MemoryAllocator {
 public:
  /** Default constructor. */
  MemoryAllocatorTBB() : mem_time_(0.0) {}

 public:
  // MemoryAllocator interface.

  void* Malloc(size_t size, uint32 alignment) override;
  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override;
  void Free(void* ptr) override;
  bool GetAllocationSize(void* reported_ptr, size_t& out_allocation_size) override;
  bool IsInternallyThreadSafe() const override { return true; }
  const char* GetDescriptiveName() override { return "TBB"; }

 protected:
  void OutOfMemory(uint64 size, uint32 alignment) {
    // this is expected not to return
    PlatformMemory::OnOutOfMemory(size, alignment);
  }

 private:
  /** Total time spent allocating / releasing memory */
  double mem_time_;
};

#endif //FUN_PLATFORM_SUPPORTS_TBB && FUN_TBB_ALLOCATOR_ALLOWED

} // namespace fun
