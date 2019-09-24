#include "fun/base/shared_library_win32.h"
//#include "fun/base/unicode_converter.h"
#include "fun/base/path.h"
#include "fun/base/windows_less.h"

namespace fun {

FastMutex SharedLibrayImpl::mutex_;

SharedLibrayImpl::SharedLibrayImpl() : handle_(nullptr) {}

SharedLibrayImpl::~SharedLibrayImpl() {
  //???
  // UnloadImpl();
}

void SharedLibrayImpl::LoadImpl(const String& path, int32 flags) {
  FUN_UNUSED(flags);

  ScopedLock<FastMutex> guard(mutex_);

  if (handle_) {
    throw LibraryAlreadyLoadedException(path);
  }

  DWORD win_flags = 0;
  Path p(path);
  if (p.IsAbsolute()) {
    win_flags |= LOAD_WITH_ALTERED_SEARCH_PATH;
  }

  UString upath = UString::FromUtf8(path);
  handle_ = LoadLibraryExW(upath.c_str(), 0, win_flags);
  if (handle_ == nullptr) {
    throw LibraryLoadException(path);
  }
  path_ = path;
}

void SharedLibrayImpl::UnloadImpl() {
  ScopedLock<FastMutex> guard(mutex_);

  if (handle_) {
    FreeLibrary((HMODULE)handle_);
    handle_ = nullptr;
  }
  path_.Clear();
}

bool SharedLibrayImpl::IsLoadedImpl() const { return !!handle_; }

void* SharedLibrayImpl::FindSymbolImpl(const String& symbol) {
  ScopedLock<FastMutex> guard(mutex_);

  if (handle_) {
    return (void*)GetProcAddress((HMODULE)handle_, symbol.c_str());
  }

  return nullptr;
}

const String& SharedLibrayImpl::GetPathImpl() const { return path_; }

String SharedLibrayImpl::PrefixImpl() { return ""; }

String SharedLibrayImpl::SuffixImpl() {
#if defined(_DEBUG) && !defined(FUN_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
  return "d.dll";
#else
  return ".dll";
#endif
}

}  // namespace fun
