#include "fun/base/cstring/ucstring.h"


namespace fun {


size_t ucscspn(const UNICHAR* str, const UNICHAR* set)
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
        goto Done;
      }
      ++q;
    }
    ++s;
  }

Done:
  return (s - str);
}


} // namespace fun
