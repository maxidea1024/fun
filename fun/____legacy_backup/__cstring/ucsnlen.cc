#include "fun/base/cstring/ucstring.h"


namespace fun {


size_t ucsnlen(const UNICHAR* str, size_t n)
{
  const UNICHAR* s = str;
  while (*s && n)
  {
    ++s;
    --n;
  }
  return s - str;
}


} // namespace fun
