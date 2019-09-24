#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucscpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src)
{
  UNICHAR* d = dst;
  while ((*d++ = *src++) != '\0') ;
  return dst;
}


} // namespace fun
