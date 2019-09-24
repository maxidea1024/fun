#include "fun/base/cstring/ucstring.h"
#include "fun/base/unicode.h"


namespace fun {


UNICHAR* ucslwr(UNICHAR* dst, size_t n)
{
  UNICHAR* d = dst;
  while (*d && n--)
  {
    *d = CharTraitsU::ToLower(*d);
    ++d;
  }
  return dst;
}


} // namespace fun
