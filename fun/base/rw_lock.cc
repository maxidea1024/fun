#include "fun/base/rw_lock.h"

//TODO rw_lock_std.cc로 대체하는게 여러모로 좋을듯...
#if defined(FUN_CXX11_RWLOCK_FINISHED) && defined(FUN_ENABLE_CPP14)
#include "fun/base/rw_lock_std.cc"
#elif FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/rw_lock_wince.cc"
#else
#include "fun/base/rw_lock_win32.cc"
#endif
#elif FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/rw_lock_android.cc"
#elif FUN_PLATFORM == FUN_PLATFORM_VXWORKS
#include "fun/base/rw_lock_vx.cc"
#else
#include "fun/base/rw_lock_posix.cc"
#endif

namespace fun {

RWLock::RWLock() {
  // NOOP
}

RWLock::~RWLock() {
  // NOOP
}

} // namespace fun
