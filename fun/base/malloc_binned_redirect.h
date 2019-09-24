#pragma once

#ifndef MALLOC_BINNED_VERSION_OVERRIDE
#define MALLOC_BINNED_VERSION_OVERRIDE 1
#endif

#if MALLOC_BINNED_VERSION_OVERRIDE != 1 && MALLOC_BINNED_VERSION_OVERRIDE != 2
#error MALLOC_BINNED_VERSION_OVERRIDE should be set to 1 or 2
#endif

#if MALLOC_BINNED_VERSION_OVERRIDE
#include "malloc_binned.h"
namespace fun {
typedef MemoryAllocatorBinned MallocBinnedRedirect;
}  // namespace fun
#else
#include "malloc_binned2.h"
namespace fun {
typedef MemoryAllocatorBinned2 MallocBinnedRedirect;
}  // namespace fun
#endif
