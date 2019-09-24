#pragma once

// Boilerplate that is included once for each module, even in monolithic builds
#ifndef PER_MODULE_BOILERPLATE_ANYLINK
# define PER_MODULE_BOILERPLATE_ANYLINK(ModuleImplClass, ModuleName)
#endif


/**
 * Override new + delete operators (and array versions) in every module.
 * This prevents the possibility of mismatched new/delete calls such as a new[] that
 * uses FUN's allocator and a delete[] that uses the system allocator.
 */
#if USING_CODE_ANALYSIS
# define OPERATOR_NEW_MSVC_PRAGMA  MSVC_PRAGMA(warning(suppress : 28251)) //  warning C28251: Inconsistent annotation for 'new': this instance has no annotations
#else
# define OPERATOR_NEW_MSVC_PRAGMA
#endif

#define REPLACEMENT_OPERATOR_NEW_AND_DELETE \
  namespace fun { \
  OPERATOR_NEW_MSVC_PRAGMA void* operator new  (size_t size                       ) OPERATOR_NEW_THROW_SPEC      { return ::silky::UnsafeMemory::Malloc(size); } \
  OPERATOR_NEW_MSVC_PRAGMA void* operator new[](size_t size                       ) OPERATOR_NEW_THROW_SPEC      { return ::silky::UnsafeMemory::Malloc(size); } \
  OPERATOR_NEW_MSVC_PRAGMA void* operator new  (size_t size, const std::nothrow_t&) OPERATOR_NEW_NOTHROW_SPEC    { return ::silky::UnsafeMemory::Malloc(size); } \
  OPERATOR_NEW_MSVC_PRAGMA void* operator new[](size_t size, const std::nothrow_t&) OPERATOR_NEW_NOTHROW_SPEC    { return ::silky::UnsafeMemory::Malloc(size); } \
  void operator delete  (void* ptr)                                                 OPERATOR_DELETE_THROW_SPEC   { ::silky::UnsafeMemory::Free(ptr); } \
  void operator delete[](void* ptr)                                                 OPERATOR_DELETE_THROW_SPEC   { ::silky::UnsafeMemory::Free(ptr); } \
  void operator delete  (void* ptr, const std::nothrow_t&)                          OPERATOR_DELETE_NOTHROW_SPEC { ::silky::UnsafeMemory::Free(ptr); } \
  void operator delete[](void* ptr, const std::nothrow_t&)                          OPERATOR_DELETE_NOTHROW_SPEC { ::silky::UnsafeMemory::Free(ptr); } \
  }

 /* GCNameDebuggerVisualizersIsFun는 제거하는게 아무래도... */

// in DLL builds, these are done per-module, otherwise we just need one in the application
// visual studio cannot find cross dll data for visualizers, so these provide access
#define PER_MODULE_BOILERPLATE \
  Array<CNameEntry const*>* GCNameTableForDebuggerVisualizers = CName::GetNameTableForDebuggerVisualizers_ST(); \
  CNameEntry*** GCNameTableForDebuggerVisualizers_MT = CName::GetNameTableForDebuggerVisualizers_MT(); \
  CFixedUObjectArray* GObjectArrayForDebugVisualizers = CCoreDelegates::GetObjectArrayForDebugVisualizersDelegate().IsBound() ? CCoreDelegates::GetObjectArrayForDebugVisualizersDelegate().Execute() : nullptr; \
  bool GCNameDebuggerVisualizersIsFun = false; \
  REPLACEMENT_OPERATOR_NEW_AND_DELETE
