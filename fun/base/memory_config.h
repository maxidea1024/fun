#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * true we want to force an ansi allocator
 */
#ifndef FUN_FORCE_ANSI_ALLOCATOR
#define FUN_FORCE_ANSI_ALLOCATOR  0
#endif

/**
 * true if intelTBB (a.k.a MallocTBB) can be used (different to the platform actually supporting it)
 */
#ifndef FUN_TBB_ALLOCATOR_ALLOWED
#define FUN_TBB_ALLOCATOR_ALLOWED  1
#endif

/**
 *  IMPORTANT:  The malloc proxy flags are mutually exclusive.
 *              You can have either none of them set or only one of them.
 */
/** FUN_USE_MALLOC_PROFILER             - Define this to use the CMallocProfiler allocator.         */
/**                                   Make sure to enable Stack Frame pointers:                 */
/**                                   bOmitFramePointers = false, or /Oy-                       */

#ifndef FUN_USE_MALLOC_PROFILER
#define FUN_USE_MALLOC_PROFILER  0
#endif

#if FUN_USE_MALLOC_PROFILER
#define MALLOC_PROFILER(X)  X
#else
#define MALLOC_PROFILER(...)
#endif

} // namespace fun
