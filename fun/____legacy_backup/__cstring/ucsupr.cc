#include "fun/base/cstring/ucstring.h"
#include "fun/base/unicode.h"


namespace fun {


UNICHAR* ucsupr(UNICHAR* dst, size_t n)
{
  UNICHAR* d = dst;
  while (*d && n--)
  {
    *d = CharTraitsU::ToUpper(*d);
    ++d;
  }
  return dst;
}


} // namespace fun
