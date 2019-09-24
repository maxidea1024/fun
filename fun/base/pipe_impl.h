#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/pipe_impl_dummy.h"
#else
#include "fun/base/pipe_impl_win32.h"
#endif
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/pipe_impl_posix.h"
#else
#include "fun/base/pipe_impl_dummy.h"
#endif
