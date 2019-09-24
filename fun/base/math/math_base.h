#pragma once

#include "fun/base/base.h"

//TODO 플랫폼 분기를 디테일하게 해야함. (정리되는 대로...)

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/math/math_base_win32.h"
#else
#include "fun/base/math/math_base_generic.h"
namespace fun {
  typedef GenericMathBase MathBase;
}
#endif
