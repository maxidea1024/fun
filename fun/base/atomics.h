#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/atomics_win32.h"
#elif FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/atomics_android.h"
#elif FUN_PLATFORM_APPLE_FAMILY
#include "fun/base/atomics_apple.h"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/atomics_unix.h"
#else
#include "fun/base/atomics_std.h"
#endif
