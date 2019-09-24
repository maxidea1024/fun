#include "fun/base/pipe_impl.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/pipe_impl_dummy.cc"
#else
#include "fun/base/pipe_impl_win32.cc"
#endif
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/pipe_impl_posix.cc"
#else
#include "fun/base/pipe_impl_dummy.cc"
#endif
