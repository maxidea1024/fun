#include "fun/base/cstring/ucstring.h"
#include "fun/base/unsafe_memory.h"


namespace fun {


UNICHAR* umemchr(const UNICHAR* str, UNICHAR ch, size_t n)
{
  while (n--)
  {
    if (*str++ == ch)
    {
      return (UNICHAR*)(str - 1);
    }
  }
  return nullptr;
}


} // namespace fun
