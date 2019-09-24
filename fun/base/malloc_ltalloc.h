#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"

namespace fun {

#if FUN_SUPPORTS_JEMALLOC  // Only use for supported platforms

#define MEM_TIME(X)

/**
 * Jason Evans' malloc (default in FreeBSD, NetBSD, used in Firefox, Facebook
 * servers) http://www.canonware.com/jemalloc/
 */
class MemoryAllocatorJemalloc : public MemoryAllocator {
 public:
  MemoryAllocatorJemalloc() : mem_time_(0.0) {}

 public:
  // MemoryAllocator interface.
  void* Malloc(size_t size, uint32 alignment) override;
  void* Realloc(void* ptr, size_t new_size, uint32 alignment) override;
  void Free(void* ptr) override;
  void DumpAllocatorStats(Printer& out) override;
  bool GetAllocationSize(void* reported_ptr,
                         size_t& out_allocation_size) override;
  bool IsInternallyThreadSafe() const override { return true; }
  const char* GetDescriptiveName() override { return "jemalloc"; }

 protected:
  void OutOfMemory() {
    fun_log(LogHAL, Fatal, "%s",
            "Ran out of virtual memory. To prevent this condition, you must "
            "free up more space on your primary hard disk.");
  }

 private:
  /** Time spent on memory tasks, in seconds */
  double mem_time_;
};

#endif  // FUN_SUPPORTS_JEMALLOC

}  // namespace fun
