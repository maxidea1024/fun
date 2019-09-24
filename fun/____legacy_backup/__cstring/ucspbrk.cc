#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucspbrk(const UNICHAR* str, const UNICHAR* set)
{
  const UNICHAR* s;
  const UNICHAR* q;

  s = str;
  while (*s)
  {
    q = set;
    while (*q)
    {
      if (*s == *q)
      {
        return (UNICHAR*)s;
      }
      ++q;
    }
    ++s;
  }

  return nullptr;
}


} // namespace fun
