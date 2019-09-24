#pragma once

#include "fun/base/base.h"
#include "fun/base/types.h"
#include <string>

namespace fun {

//TODO Traits...

//TODO 정리를 좀 해야하는데... stl 의존성을 그대로 두어야할까??
//UTF32는 구지 지원할 필요가 없어보이는데...
#if defined(FUN_PLATFORM_WINDWOS_FAMILY)
  typedef wchar_t UTF16CHAR;
  typedef std::wstring UTF16String;
#else
  typedef char16_t UTF16CHAR;
  typedef std::u16string UTF16String;
#endif

typedef char32_t UTF32CHAR;
typedef std::u32string UTF32String;

} // namespace fun
