#include "fun/base/named_mutex.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/named_mutex_win32.cc"
#elif FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/named_mutex_android.cc"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/named_mutex_unix.cc"
#endif

namespace fun {

NamedMutex::NamedMutex(const String& name) : NamedMutexImpl(name) {}

NamedMutex::~NamedMutex() {}

} // namespace fun
