#include "fun/base/malloc_stomp.h"

#if FUN_USE_MALLOC_STOMP

namespace fun {

//TODO
//static void MallocStompOverrunTest()
//{
//  const uint32 ARRAY_SIZE = 4;
//  uint8* ptr = new uint8[ARRAY_SIZE];
//  // Overrun.
//  ptr[ARRAY_SIZE+1] = 0;
//}
//
//AutoConsoleCommand MallocStompTestCommand(
//  "MemoryAllocatorStomp.OverrunTest",
//  "Overrun test for the MemoryAllocatorStomp",
//  ConsoleCommandDelegate::CreateStatic(&MallocStompOverrunTest)
//);

void* MemoryAllocatorStomp::Malloc(size_t size, uint32 alignment) {
  if (size == 0U) {
    return nullptr;
  }

  const size_t allocated_size = (alignment > 0U) ? ((size + alignment - 1U) & -static_cast<int32>(alignment)) : size;
  const size_t alloc_full_page_size = allocated_size + sizeof(AllocationData) + (PAGE_SIZE - 1) & ~(PAGE_SIZE - 1U);

#if PLATFORM_LINUX || PLATFORM_MAC
  // Note: can't implement BinnedAllocFromOS as a mmap call. See Free() for the reason.
  void* full_allocation_ptr = mmap(nullptr, alloc_full_page_size + PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#else
  void* full_allocation_ptr = PlatformMemory::BinnedAllocFromOS(alloc_full_page_size + PAGE_SIZE);
#endif // PLATFORM_LINUX || PLATFORM_MAC

  void* returned_pointer = nullptr;
  static const size_t ALLOCATION_DATA_SIZE = sizeof(AllocationData);

  if (use_underrun_mode_) {
    const size_t aligned_allocation_data = (alignment > 0U) ? ((ALLOCATION_DATA_SIZE + alignment - 1U) & -static_cast<int32>(alignment)) : ALLOCATION_DATA_SIZE;
    returned_pointer = reinterpret_cast<void*>(reinterpret_cast<uint8*>(full_allocation_ptr) + PAGE_SIZE + aligned_allocation_data);

    AllocationData* alloc_data_ptr = reinterpret_cast<AllocationData*>(reinterpret_cast<uint8*>(full_allocation_ptr) + PAGE_SIZE);
    AllocationData alloc_data = { full_allocation_ptr, alloc_full_page_size + PAGE_SIZE, allocated_size, SENTINEL_EXPECTED_VALUE };
    *alloc_data_ptr = alloc_data;

    // Page protect the first page, this will cause the exception in case the is an underrun.
    PlatformMemory::PageProtect(full_allocation_ptr, PAGE_SIZE, false, false);
  } else {
    returned_pointer = reinterpret_cast<void*>(reinterpret_cast<uint8*>(full_allocation_ptr) + alloc_full_page_size - allocated_size);

    AllocationData* alloc_data_ptr = reinterpret_cast<AllocationData*>(reinterpret_cast<uint8*>(returned_pointer) - ALLOCATION_DATA_SIZE);
    AllocationData alloc_data = { full_allocation_ptr, alloc_full_page_size + PAGE_SIZE, allocated_size, SENTINEL_EXPECTED_VALUE };
    *alloc_data_ptr = alloc_data;

    // Page protect the last page, this will cause the exception in case the is an overrun.
    PlatformMemory::PageProtect(reinterpret_cast<void*>(reinterpret_cast<uint8*>(full_allocation_ptr) + alloc_full_page_size), PAGE_SIZE, false, false);
  }

  return returned_pointer;
}

void* MemoryAllocatorStomp::Realloc(void* ptr, size_t new_size, uint32 alignment) {
  if (new_size == 0U) {
    Free(ptr);
    return nullptr;
  }

  void* return_ptr = nullptr;
  if (ptr) {
    return_ptr = Malloc(new_size, alignment);

    AllocationData* alloc_data_ptr = reinterpret_cast<AllocationData*>(reinterpret_cast<uint8*>(ptr) - sizeof(AllocationData));
    UnsafeMemory::Memcpy(return_ptr, ptr, MathBase::Min(alloc_data_ptr->size, new_size));
    Free(ptr);
  } else {
    return_ptr = Malloc(new_size, alignment);
  }

  return return_ptr;
}

void MemoryAllocatorStomp::Free(void* ptr) {
  if (ptr == nullptr) {
    return;
  }

  AllocationData* alloc_data_ptr = reinterpret_cast<AllocationData*>(ptr);
  alloc_data_ptr--;

  // Check that our sentinel is intact.
  if (alloc_data_ptr->sentinel != SENTINEL_EXPECTED_VALUE) {
    // There was a memory underrun related to this allocation.
    CPlatformMisc::DebugBreak();
  }

#if PLATFORM_LINUX || PLATFORM_MAC
  // Note: Can't wrap munmap inside BinnedFreeToOS() because the code doesn't
  // expect the size of the allocation to be freed to be available, nor the
  // pointer be aligned with the page size. We can guarantee that here so that's
  // why we can do it.
  munmap(alloc_data_ptr->full_allocation_ptr, alloc_data_ptr->full_size);
#else
  PlatformMemory::BinnedFreeToOS(alloc_data_ptr->full_allocation_ptr, alloc_data_ptr->full_size);
#endif // PLATFORM_LINUX || PLATFORM_MAC
}

bool MemoryAllocatorStomp::GetAllocationSize(void* original, size_t& out_allocation_size) {
  if (original == nullptr) {
    out_allocation_size = 0U;
  } else {
    AllocationData* alloc_data_ptr = reinterpret_cast<AllocationData*>(original);
    alloc_data_ptr--;
    out_allocation_size = alloc_data_ptr->full_size;
  }

  return true;
}

} // namespace fun

#endif // FUN_USE_MALLOC_STOMP
