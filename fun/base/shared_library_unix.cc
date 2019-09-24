#include "fun/base/shared_library_unix.h"
#include "fun/base/exception.h"

#include <dlfcn.h>

// Note: cygwin is missing RTLD_LOCAL, set it to 0
#if FUN_PLATFORM == FUN_PLATFORM_CYGWIN && !defined(RTLD_LOCAL)
  #define RTLD_LOCAL 0
#endif

namespace fun {

FastMutex SharedLibrayImpl::mutex_;

SharedLibrayImpl::SharedLibrayImpl()
  : handle_(nullptr) {}

SharedLibrayImpl::~SharedLibrayImpl() {
  //???
  //UnloadImpl();
}

void SharedLibrayImpl::LoadImpl(const String& path, int32 flags) {
  ScopedLock<FastMutex> guard(mutex_);

  if (handle_) {
    throw LibraryAlreadyLoadedException(path);
  }

  int32 real_flags = RTLD_LAZY;
  if (flags & SHLIB_LOCAL_IMPL) {
    real_flags |= RTLD_LOCAL;
  } else {
    real_flags |= RTLD_GLOBAL;
  }

  handle_ = dlopen(path.c_str(), real_flags);

  if (handle_ == nullptr) {
    const char* err = dlerror();
    throw LibraryLoadException(err ? err : path);
  }

  path_ = path;
}

void SharedLibrayImpl::UnloadImpl() {
  ScopedLock<FastMutex> guard(mutex_);

  if (handle_) {
    dlclose(handle_);
    handle_ = nullptr;
  }
  path_.Clear();
}

bool SharedLibrayImpl::IsLoadedImpl() const {
  return !!handle_;
}

void* SharedLibrayImpl::FindSymbolImpl(const String& symbol) {
  ScopedLock<FastMutex> guard(mutex_);

  if (handle_) {
    return (void*)dlsym((HMODULE)handle_, symbol.c_str());
  }

  return nullptr;
}

const String& SharedLibrayImpl::GetPathImpl() const {
  return path_;
}

String SharedLibrayImpl::PrefixImpl() {
#if FUN_PLATFORM == FUN_PLATFORM_CYGWIN
  return "cyg";
#else
  return "lib";
#endif
}

String SharedLibrayImpl::SuffixImpl()
{
#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  #if defined(_DEBUG) && !defined(FUN_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
    return "d.dylib";
  #else
    return ".dylib";
  #endif
#elif FUN_PLATFORM == FUN_PLATFORM_HPUX
  #if defined(_DEBUG) && !defined(FUN_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
    return "d.sl";
  #else
    return ".sl";
  #endif
#elif FUN_PLATFORM == FUN_PLATFORM_CYGWIN
  #if defined(_DEBUG) && !defined(FUN_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
    return "d.dll";
  #else
    return ".dll";
  #endif
#else
  #if defined(_DEBUG) && !defined(FUN_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
    return "d.so";
  #else
    return ".so";
  #endif
#endif
}

} // namespace fun
