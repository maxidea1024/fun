#pragma once

#include "fun/base/version.h"


#if FUN_VERSION >= 0x02000000
  #define FUN_ENABLE_CPP11  1
#endif


#define FUN_STATIC        1
#define FUN_BASE_EXPORTS  1


// Define to desired default thread stack size
// Zero means OS default
#ifndef FUN_THREAD_STACK_SIZE
  #define FUN_THREAD_STACK_SIZE  0
#endif


// Macro enabling FUN Exceptions and assertions
// to append stack backtrace (if available)
#define FUN_EXCEPTION_BACKTRACE  1
