#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Fixed-sized allocator that uses a free list to cache allocations.
 * Initial allocation block can be specified in the ctor to warm the cache.
 * Subsequent allocations are in multiples of the given blocksize.
 *
 * Grow can be called at any time to warm the cache with a single block
 * allocation. Initial allocation will want to be a reasonable guess as to how
 * big the pool might grow. BlockSize should be small enough to cut down on
 * allocations but not overcommit memory.
 *
 * NOTE: Currently there is not way to flush the pool because
 *       it doesn't track each block's allocation status.
 *
 * Not threadsafe <could be implemented using a ThreadingPolicy template
 * parameter>.
 *
 * Template params:
 *   AllocationSize - size of each allocation (must be at least as large as a
 * pointer) BlockSize - number of allocations to reserve room for when a new
 * block needs to be allocated.
 */
template <uint32 AllocationSize, uint32 BlockSize>
class AllocatorFixedSizeFreeList {
 public:
  /**
   * \param initial_block_size - number of allocations to warm the cache with
   */
  AllocatorFixedSizeFreeList(uint32 initial_block_size = 0)
      : free_list_(nullptr), allocated_count_(0), live_count_(0) {
    // need enough memory to store pointers for the free list
    static_assert(AllocationSize >= sizeof(FreeListNode),
                  "Allocation size must be large enough to hold pointer.");

    // warm the cache
    Grow(initial_block_size);
  }

  /**
   * Destructor. Can't free memory, so only checks that allocations have been
   * returned.
   */
  ~AllocatorFixedSizeFreeList() {
    // by now all block better have been returned to me
    // TODO
    // fun_check(GIsCriticalError || live_count_ == 0);
    fun_check(live_count_ == 0);
    // WRH - 2007/09/14 - Note we are stranding memory here.
    // These pools are meant to be global and never deleted.
  }

  /**
   * Allocates one element from the free list. Return it by calling Free.
   */
  void* Allocate() {
    CheckInvariants();
    if (!free_list_) {
      fun_check_dbg(live_count_ == allocated_count_);
      Grow(BlockSize);
    }

    // grab next free element, updating free_list_ pointer
    void* raw_mem = (void*)free_list_;
    free_list_ = free_list_->next_free_allocation;
    ++live_count_;
    CheckInvariants();
    return raw_mem;
  }

  /**
   * Returns one element from the free list.
   * Must have been acquired previously by Allocate.
   */
  void Free(void* element) {
    CheckInvariants();
    fun_check_dbg(live_count_ > 0);
    --live_count_;
    FreeListNode* new_free_element = (FreeListNode*)element;
    new_free_element->next_free_allocation = free_list_;
    free_list_ = new_free_element;
    CheckInvariants();
  }

  /**
   * Get total memory allocated
   */
  uint32 GetAllocatedSize() const {
    CheckInvariants();
    return allocated_count_ * AllocationSize;
  }

  /**
   * Grows the free list by a specific number of elements. Does one allocation
   * for all elements. Safe to call at any time to warm the cache with a single
   * block allocation.
   */
  void Grow(uint32 element_count) {
    if (element_count == 0) {
      return;
    }

    // need enough memory to store pointers for the free list
    fun_check(AllocationSize * element_count >= sizeof(FreeListNode));

    // allocate a block of memory
    uint8* raw_mem =
        (uint8*)UnsafeMemory::Malloc(AllocationSize * element_count);
    FreeListNode* new_free_list = (FreeListNode*)raw_mem;

    // Chain the block into a list of free list nodes
    for (uint32 i = 0; i < element_count - 1;
         ++i, new_free_list = new_free_list->next_free_allocation) {
      new_free_list->next_free_allocation =
          (FreeListNode*)(raw_mem + (i + 1) * AllocationSize);
    }

    // link the last Node to the previous free_list_
    new_free_list->next_free_allocation = free_list_;
    free_list_ = (FreeListNode*)raw_mem;

    allocated_count_ += element_count;
  }

 private:
  struct FreeListNode {
    FreeListNode* next_free_allocation;
  };

  void CheckInvariants() const {
    fun_check_dbg(allocated_count_ >= live_count_);
  }

 private:
  /** Linked List of free memory blocks. */
  FreeListNode* free_list_;

  /** The number of objects that have been allocated. */
  uint32 allocated_count_;

  /** The number of objects that are constructed and "out in the wild". */
  uint32 live_count_;
};

}  // namespace fun
