#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* umemset(UNICHAR* dst, UNICHAR ch, size_t n)
{
  UNICHAR* d = dst;
  while (n--)
  {
    *d++ = ch;
  }
  return dst;
}


} // namespace fun
