#include "fun/base/memory.h"

#include "fun/base/malloc_thread_safe_proxy.h"

#if defined(FUN_MALLOC_VERIFY)
#include "fun/base/malloc_verify.h"
#endif

#if defined(FUN_MALLOC_LEAKDETECTION)
#include "fun/base/malloc_leak_detection.h"
#endif

#if defined(FUN_USE_MALLOC_FILL_BYTES)
#include "fun/base/malloc_poison_proxy.h"
#endif

namespace fun {

MemoryAllocator* MemoryAllocator::GetGlobalMalloc() {
  static MemoryAllocator* global_malloc_instance = nullptr;

  if (global_malloc_instance) {
    return global_malloc_instance;
  }

  global_malloc_instance = PlatformMemory::BaseAllocator();

  // TODO 이것에 대해서는 적용여부를 고민해봐야....
  // Setup malloc crash as soon as possible.
  PlatformMallocCrash::Get(global_malloc_instance);

  // TODO MallocProfiler / MallocProfiler 적용해야함.

  if (!global_malloc_instance->IsInternallyThreadSafe()) {
    global_malloc_instance =
        new MemoryAllocatorThreadSafeProxy(global_malloc_instance);
  }

#if defined(FUN_MALLOC_VERIFY)
  global_malloc_instance =
      new MemoryAllocatorVerifyProxy(global_malloc_instance);
#endif

#if defined(FUN_MALLOC_LEAKDETECTION)
  global_malloc_instance =
      new MemoryAllocatorLeakDetectionProxy(global_malloc_instance);
#endif

#if defined(FUN_USE_MALLOC_FILL_BYTES)
  global_malloc_instance =
      new MemoryAllocatorPoisonProxy(global_malloc_instance);
#endif
}

//응급상황에서 사용하는??
MemoryAllocator** MemoryAllocator::GetFixedMallocLocationPtr() {
  // TODO what is this??
  return nullptr;
}

void* UnsafeMemory::MallocExternal(size_t count, uint32 alignment) {
  return MemoryAllocator::GetGlobalMalloc()->Malloc(count, alignment);
}

void* UnsafeMemory::ReallocExternal(void* ptr, size_t new_count,
                                    uint32 alignment) {
  return MemoryAllocator::GetGlobalMalloc()->Realloc(ptr, new_count, alignment);
}

void UnsafeMemory::FreeExternal(void* ptr) {
  MemoryAllocator::GetGlobalMalloc()->Free(ptr);
}

size_t UnsafeMemory::GetAllocSizeExternal(void* ptr) {
  size_t size = 0;
  return MemoryAllocator::GetGlobalMalloc()->GetAllocSize(ptr, size) ? size : 0;
}

size_t UnsafeMemory::QuantizeSizeExternal(void* ptr) {
  size_t size = 0;
  return MemoryAllocator::GetGlobalMalloc()->QuantizeSize(ptr, size);
}

void UnsafeMemory::Trim() { MemoryAllocator::GetGlobalMalloc()->Trim(); }

void UnsafeMemory::SetupTlsCachesOnCurrentThread() {
  MemoryAllocator::GetGlobalMalloc()->SetupTlsCachesOnCurrentThread();
}

void UnsafeMemory::ClearAndDisableTlsCachesOnCurrentThread() {
  // TODO 할당자가 유효할때만 해야하는군....
  MemoryAllocator::GetGlobalMalloc()->SetupTlsCachesOnCurrentThread();
}

//
// UseSystemMallocForNew
//

void* UseSystemMallocForNew::operator new(size_t size) {
  return UnsafeMemory::SystemMalloc(size);
}

void UseSystemMallocForNew::operator delete(void* ptr) {
  UnsafeMemory::SystemFree(ptr);
}

void* UseSystemMallocForNew::operator new[](size_t size) {
  return UnsafeMemory::SystemMalloc(size);
}

void UseSystemMallocForNew::operator delete[](void* ptr) {
  UnsafeMemory::SystemFree(ptr);
}

// TODO 필요한 이유에 대해서 고민을 해봐야함....
//#if !INLINE_CMEMORY_OPERATION && !PLATFORM_USES_FIXED_GMalloc_CLASS
//#include "fun/memory_inline.h"
//#endif

}  // namespace fun
