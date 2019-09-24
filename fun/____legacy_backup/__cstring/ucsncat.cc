#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucsncat(UNICHAR* __restrict dst, const UNICHAR* __restrict src, size_t n)
{
  UNICHAR* d;
  UNICHAR* q;
  const UNICHAR* r;

  d = dst;
  while (*d++);
  q = d;
  r = src;
  while (n && *r)
  {
    *q++ = *r++;
    --n;
  }
  *q = '\0';
  return dst;
}


} // namespace fun
