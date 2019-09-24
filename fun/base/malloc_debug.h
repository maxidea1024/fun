#pragma once

#include "fun/base/base.h"
#include "fun/base/memory_base.h"

namespace fun {

/**
 * Debug memory allocator.
 */
class MemoryAllocatorDebug : public MemoryAllocator {
  // Tags.
  enum { MEM_PRE_TAG = 0xF0ED1CEE };
  enum { MEM_POST_TAG = 0xDEADF00F };
  enum { MEM_TAG = 0xFE };
  enum { MEM_WIPE_TAG = 0xCD };

  // alignment.
  enum { ALLOCATION_ALIGNMENT = 16 };

  // Number of block sizes to collate (in steps of 4 bytes)
  enum { MEM_SIZE_MAX = 128 };
  enum { MEM_RECENT = 5000 };
  enum { MEM_AGE_MAX = 80 };
  enum { MEM_AGE_SLICE = 100 };

 private:
  // Structure for memory debugging.
  struct MemDebug {
    size_t size;
    int32 ref_count;
    int32* pre_tag;
    MemDebug* next;
    MemDebug** prev_link;
  };

  /** First node */
  MemDebug* g_first_debug;

  /** Total size of allocations */
  size_t total_allocation_size_;

  /** Total size of allocation overhead */
  size_t total_waste_size_;

  static const uint32 ALLOCATOR_OVERHEAD = sizeof(MemDebug) +
                                           sizeof(MemDebug*) + sizeof(int32) +
                                           ALLOCATION_ALIGNMENT + sizeof(int32);

 public:
  MemoryAllocatorDebug()
      : g_first_debug(nullptr),
        total_allocation_size_(0),
        total_waste_size_(0) {}

  //
  // MemoryAllocator interface
  //

  void* Malloc(size_t size, uint32 alignment) override {
    fun_check_msg(alignment == DEFAULT_ALIGNMENT,
                  "alignment currently unsupported in this allocator");
    MemDebug* ptr = (MemDebug*)malloc(ALLOCATOR_OVERHEAD + size);
    fun_check_ptr(ptr);
    uint8* aligned_ptr = Align(
        (uint8*)ptr + sizeof(MemDebug) + sizeof(MemDebug*) + sizeof(int32),
        ALLOCATION_ALIGNMENT);
    ptr->ref_count = 1;

    ptr->size = size;
    ptr->next = g_first_debug;
    ptr->prev_link = &g_first_debug;
    ptr->pre_tag = (int32*)(aligned_ptr - sizeof(int32));
    *ptr->pre_tag = MEM_PRE_TAG;
    *((MemDebug**)(aligned_ptr - sizeof(int32) - sizeof(MemDebug*))) = ptr;
    *(int32*)(aligned_ptr + size) = MEM_POST_TAG;
    UnsafeMemory::Memset(aligned_ptr, MEM_TAG, size);
    if (g_first_debug) {
      fun_check(GIsCriticalError || g_first_debug->prev_link == &g_first_debug);
      g_first_debug->prev_link = &ptr->next;
    }
    g_first_debug = ptr;
    total_allocation_size_ += size;
    total_waste_size_ += ALLOCATOR_OVERHEAD;
    fun_check(!(intptr_t(aligned_ptr) &
                ((intptr_t)0xF)));  // must be 16 bytes aligned.
    return aligned_ptr;
  }

  void* Realloc(void* reported_ptr, size_t new_size,
                uint32 alignment) override {
    if (reported_ptr && new_size > 0) {
      MemDebug* ptr = *((MemDebug**)((uint8*)reported_ptr - sizeof(int32) -
                                     sizeof(MemDebug*)));
      fun_check(GIsCriticalError || (ptr->ref_count == 1));
      void* result = Malloc(new_size, alignment);
      UnsafeMemory::Memcpy(result, reported_ptr,
                           MathBase::Min<size_t>(ptr->size, new_size));
      Free(reported_ptr);
      return result;
    } else if (reported_ptr == nullptr) {
      return Malloc(new_size, alignment);
    } else {
      Free(reported_ptr);
      return nullptr;
    }
  }

  void Free(void* reported_ptr) override {
    if (reported_ptr == nullptr) {
      return;
    }

    MemDebug* ptr = *(
        (MemDebug**)((uint8*)reported_ptr - sizeof(int32) - sizeof(MemDebug*)));

    fun_check(GIsCriticalError || ptr->ref_count == 1);
    fun_check(GIsCriticalError || *ptr->pre_tag == MEM_PRE_TAG);
    fun_check(GIsCriticalError ||
              *(int32*)((uint8*)reported_ptr + ptr->size) == MEM_POST_TAG);

    total_allocation_size_ -= ptr->size;
    total_waste_size_ -= ALLOCATOR_OVERHEAD;

    UnsafeMemory::Memset(reported_ptr, MEM_WIPE_TAG, ptr->size);
    ptr->size = 0;
    ptr->ref_count = 0;

    fun_check(GIsCriticalError || ptr->prev_link);
    fun_check(GIsCriticalError || *ptr->prev_link == ptr);
    *ptr->prev_link = ptr->next;
    if (ptr->next) {
      ptr->next->prev_link = ptr->prev_link;
    }

    free(ptr);
  }

  bool GetAllocationSize(void* reported_ptr,
                         size_t& out_allocation_size) override {
    if (reported_ptr == nullptr) {
      out_allocation_size = 0;
    } else {
      const MemDebug* ptr = *((MemDebug**)((uint8*)reported_ptr -
                                           sizeof(int32) - sizeof(MemDebug*)));
      out_allocation_size = ptr->size;
    }

    return true;
  }

  void DumpAllocatorStats(Printer& out) override {
    out.Printf("Total Allocation size: %u", total_allocation_size_);
    out.Printf("Total Waste size: %u", total_waste_size_);

    int32 count = 0;
    int32 chunks = 0;

    out.Printf("");
    out.Printf("Unfreed memory:");
    for (MemDebug* ptr = g_first_debug; ptr; ptr = ptr->next) {
      count += ptr->size;
      chunks++;
    }

    out.Printf("End of list: %i Bytes still allocated", count);
    out.Printf("             %i Chunks allocated", chunks);
  }

  bool ValidateHeap() override {
    for (MemDebug** link = &g_first_debug; *link; link = &(*link)->next) {
      fun_check(GIsCriticalError || *(*link)->prev_link == *link);
    }

#if FUN_PLATFORM_WINDOWS_FAMILY
    int32 result = _heapchk();
    fun_check(result != _HEAPBADBEGIN);
    fun_check(result != _HEAPBADNODE);
    fun_check(result != _HEAPBADPTR);
    fun_check(result != _HEAPEMPTY);
    fun_check(result == _HEAPOK);
#endif

    return true;
  }

  bool Exec(RuntimeEnv* env, const char* cmd, Printer& out) override {
    return false;
  }

  const char* GetDescriptiveName() override { return "Debug"; }
};

}  // namespace fun
