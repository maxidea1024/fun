#ifndef CMEMORY_INLINE_FUNCTION_DECORATOR
# define CMEMORY_INLINE_FUNCTION_DECORATOR
#endif

#ifndef CMEMORY_INLINE_GMalloc
# define CMEMORY_INLINE_GMalloc  GMalloc
#endif

CMEMORY_INLINE_FUNCTION_DECORATOR void* UsafeMemory::Malloc(size_t count, uint32 alignment) {
  if (CMEMORY_INLINE_GMalloc == nullptr) {
    return MallocExternal(count, alignment);
  }
  DoGamethreadHook(0);
  ScopedMallocTimer timer(0);
  return CMEMORY_INLINE_GMalloc->Malloc(count, alignment);
}

CMEMORY_INLINE_FUNCTION_DECORATOR void* UsafeMemory::Realloc(void* original, size_t count, uint32 alignment) {
  if (CMEMORY_INLINE_GMalloc == nullptr) {
    return ReallocExternal(original, count, alignment);
  }
  DoGamethreadHook(1);
  ScopedMallocTimer timer(1);
  return CMEMORY_INLINE_GMalloc->Realloc(original, count, alignment);
}

CMEMORY_INLINE_FUNCTION_DECORATOR void UsafeMemory::Free(void* original) {
  if (original == nullptr) {
    ScopedMallocTimer timer(3);
    return;
  }

  if (CMEMORY_INLINE_GMalloc == nullptr) {
    FreeExternal(original);
    return;
  }
  DoGamethreadHook(2);
  ScopedMallocTimer timer(2);
  CMEMORY_INLINE_GMalloc->Free(original);
}

CMEMORY_INLINE_FUNCTION_DECORATOR size_t UsafeMemory::GetAllocSize(void* original) {
  if (CMEMORY_INLINE_GMalloc == nullptr) {
    return GetAllocSizeExternal(original);
  }

  size_t size = 0;
  return CMEMORY_INLINE_GMalloc->GetAllocationSize(original, size) ? size : 0;
}

CMEMORY_INLINE_FUNCTION_DECORATOR size_t UsafeMemory::QuantizeSize(size_t count, uint32 alignment) {
  if (CMEMORY_INLINE_GMalloc == nullptr) {
    return count;
  }
  return CMEMORY_INLINE_GMalloc->QuantizeSize(count, alignment);
}
