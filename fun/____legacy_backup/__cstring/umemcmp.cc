#include "fun/base/cstring/ucstring.h"
#include "fun/base/unsafe_memory.h"


namespace fun {


int32 umemcmp(const UNICHAR* str1, const UNICHAR* str2, size_t n)
{
  while (n--)
  {
    if (*str1 != *str2)
    {
      return *str1 > *str2 ? 1 : -1; 
    }
    ++str1;
    ++str2;
  }
  return 0;
}


} // namespace fun
