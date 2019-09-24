#pragma once

#include "fun/base/base.h"

//TODO supports more platforms

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/string/cstring_win32.h"
#else
#include "fun/base/string/cstring_std.h"
#endif
