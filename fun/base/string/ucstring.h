#pragma once

#include "fun/base/base.h"

namespace fun {

FUN_BASE_API size_t   ucslen(const UNICHAR* str);
FUN_BASE_API size_t   ucsnlen(const UNICHAR* str, size_t n);

FUN_BASE_API UNICHAR* ucsnset(UNICHAR* dst, UNICHAR ch, size_t n);

FUN_BASE_API UNICHAR* ucscpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src);
FUN_BASE_API UNICHAR* ucsncpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src, size_t n);

FUN_BASE_API UNICHAR* ucscat(UNICHAR* __restrict dst, const UNICHAR* __restrict src);
FUN_BASE_API UNICHAR* ucsncat(UNICHAR* __restrict dst, const UNICHAR* __restrict src, size_t n);

FUN_BASE_API UNICHAR* ucsupr(UNICHAR* dst, size_t n);
FUN_BASE_API UNICHAR* ucslwr(UNICHAR* dst, size_t n);

FUN_BASE_API int32    ucscmp(const UNICHAR* str1, const UNICHAR* str2);
FUN_BASE_API int32    ucsncmp(const UNICHAR* str1, const UNICHAR* str2, size_t n);
FUN_BASE_API int32    ucsicmp(const UNICHAR* str1, const UNICHAR* str2, size_t n);
FUN_BASE_API int32    ucsnicmp(const UNICHAR* str1, const UNICHAR* str2, size_t n);

FUN_BASE_API UNICHAR* ucschr(const UNICHAR* str, UNICHAR ch);
FUN_BASE_API UNICHAR* ucsrchr(const UNICHAR* str, UNICHAR ch);
FUN_BASE_API UNICHAR* ucsichr(const UNICHAR* str, UNICHAR ch);
FUN_BASE_API UNICHAR* ucsrichr(const UNICHAR* str, UNICHAR ch);

FUN_BASE_API UNICHAR* ucsstr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub);
FUN_BASE_API UNICHAR* ucsrstr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub);
FUN_BASE_API UNICHAR* ucsistr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub);
FUN_BASE_API UNICHAR* ucsristr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub);

FUN_BASE_API UNICHAR* ucstok(UNICHAR* __restrict str, const UNICHAR* __restrict delim, UNICHAR** __restrict last);
FUN_BASE_API UNICHAR* ucsitok(UNICHAR* __restrict str, const UNICHAR* __restrict delim, UNICHAR** __restrict last);

FUN_BASE_API size_t   ucsspn(const UNICHAR* str, const UNICHAR* set);
FUN_BASE_API UNICHAR* ucspbrk(const UNICHAR* str, const UNICHAR* set);
FUN_BASE_API size_t   ucscspn(const UNICHAR* str, const UNICHAR* set);

FUN_BASE_API UNICHAR* ucsdup(const UNICHAR* str);

FUN_BASE_API UNICHAR* umemset(UNICHAR* dst, UNICHAR ch, size_t n);
FUN_BASE_API UNICHAR* umemmove(UNICHAR* dst, const UNICHAR* src, size_t n);
FUN_BASE_API UNICHAR* umemcpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src, size_t n);
FUN_BASE_API int32    umemcmp(const UNICHAR* str1, const UNICHAR* str2, size_t n);
FUN_BASE_API UNICHAR* umemchr(const UNICHAR* str, UNICHAR ch, size_t n);

} // namespace fun
