#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucscat(UNICHAR * __restrict dst, const UNICHAR * __restrict src)
{
  UNICHAR* d = dst;
  while (*d++);
  while ((*d++ = *src++));
  return dst;
}


} // namespace fun
