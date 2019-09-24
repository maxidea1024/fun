#include "fun/base/malloc_verify.h"

#if FUN_MALLOC_VERIFY

namespace fun {

void MallocVerify::Malloc(void* ptr) {
  if (ptr) {
    CLOG(allocated_pointers_.Contains(ptr), LogMemory, Fatal, "Malloc allocated pointer that's already been allocated: 0x%016llx", (int64)(intptr_t)ptr);

    allocated_pointers_.Add(ptr);
  }
}

void MallocVerify::Realloc(void* old_ptr, void* new_ptr) {
  if (old_ptr != new_ptr) {
    if (old_ptr) {
      CLOG(!allocated_pointers_.Contains(old_ptr), LogMemory, Fatal, "Realloc entered with old pointer that hasn't been allocated yet: 0x%016llx", (int64)(intptr_t)old_ptr);

      allocated_pointers_.Remove(old_ptr);
    }

    if (new_ptr) {
      CLOG(allocated_pointers_.Contains(new_ptr), LogMemory, Fatal, "Realloc allocated new pointer that has already been allocated: 0x%016llx", (int64)(intptr_t)new_ptr);

      allocated_pointers_.Add(new_ptr);
    }
  } else if (old_ptr) {
    CLOG(!allocated_pointers_.Contains(old_ptr), LogMemory, Fatal, "Realloc entered with old pointer that hasn't been allocated yet: 0x%016llx", (int64)(intptr_t)old_ptr);
  }
}

void MallocVerify::Free(void* ptr) {
  if (ptr) {
    CLOG(!allocated_pointers_.Contains(ptr), LogMemory, Fatal, "Free attempts to free a pointer that hasn't been allocated yet: 0x%016llx", (int64)(intptr_t)ptr);

    allocated_pointers_.Remove(ptr);
  }
}

} // namespace fun

#endif //FUN_MALLOC_VERIFY
