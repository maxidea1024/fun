#include "fun/base/cstring/ucstring.h"


namespace fun {


int32 ucscmp(const UNICHAR* str1, const UNICHAR* str2)
{
  while (*str1 == *str2++)
  {
    if (*str1++ == '\0')
    {
      return 0;
    }
  }

  --str2;
  return (int32)*str1 - (int32)*str2;
}


} // namespace fun
