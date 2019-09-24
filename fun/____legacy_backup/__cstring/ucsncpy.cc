#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucsncpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src, size_t n)
{
  if (n != 0)
  {
    UNICHAR* d = dst;
    const UNICHAR* s = src;

    do
    {
      if ((*d++ = *s++) == '\0')
      {
        // NUL pad the remaining n-1 bytes
        while (--n)
        {
          *d++ = '\0';
        }
        break;
      }
    } while (--n);
  }

  return dst;
}


} // namespace fun
