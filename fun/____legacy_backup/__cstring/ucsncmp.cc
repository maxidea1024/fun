#include "fun/base/cstring/ucstring.h"


namespace fun {


int32 ucsncmp(const UNICHAR* str1, const UNICHAR* str2, size_t n)
{
  if (n == 0)
  {
    return 0;
  }

  do
  {
    if (*str1 != *str2++)
    {
      --str2;
      return (int32)*str1 - (int32)*str2;
    }

    if (*str1++ == 0)
    {
      break;
    }
  } while (--n != 0);

  return 0;
}


} // namespace fun
