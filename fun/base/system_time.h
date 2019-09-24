#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/system_time_win32.h"
#else
#include "fun/base/system_time_generic.h"
namespace fun {
typedef SystemTimeGeneric SystemTime;
}
#endif
