#include "fun/base/memory_base.h"

namespace fun {

AtomicCounter32 MemoryAllocator::total_malloc_calls_;
AtomicCounter32 MemoryAllocator::total_free_calls_;
AtomicCounter32 MemoryAllocator::total_realloc_calls_;

} // namespace fun
