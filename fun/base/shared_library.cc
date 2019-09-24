#include "fun/base/shared_library.h"
#include "fun/base/exception.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/shared_library_win32.cc"
#else
#include "fun/base/shared_library_unix.cc"
#endif

namespace fun {

SharedLibrary::SharedLibrary() {}

SharedLibrary::SharedLibrary(const String& path) {
  LoadImpl(path, 0);
}

SharedLibrary::SharedLibrary(const String& path, int32 flags) {
  LoadImpl(path, flags);
}

SharedLibrary::~SharedLibrary() {
  //TODO 안해도 되남??
  //Unload();
}

void SharedLibrary::Load(const String& path) {
  LoadImpl(path, 0);
}

void SharedLibrary::Load(const String& path, int32 flags) {
  LoadImpl(path, flags);
}

void SharedLibrary::Unload() {
  UnloadImpl();
}

bool SharedLibrary::IsLoaded() const {
  return IsLoadedImpl();
}

bool SharedLibrary::HasSymbol(const String& symbol) {
  return FindSymbolImpl(symbol) != nullptr;
}

void* SharedLibrary::GetSymbol(const String& symbol) {
  if (void* rv = FindSymbolImpl(symbol)) {
    return rv;
  }

  throw NotFoundException(symbol);
}

const String& SharedLibrary::GetPath() const {
  return GetPathImpl();
}

String SharedLibrary::Prefix() {
  return PrefixImpl();
}

String SharedLibrary::Suffix() {
  return SuffixImpl();
}

String SharedLibrary::GetOsName(const String& name) {
  return Prefix() + name + Suffix();
}

} // namespace fun
