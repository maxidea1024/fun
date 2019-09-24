#include "fun/base/cstring/ucstring.h"
#include "fun/base/cstring/unicode.h"


namespace fun {


int32 ucsnicmp(const UNICHAR* str1, const UNICHAR* str2, size_t n)
{
  UNICHAR c1, c2;

  if (n == 0)
  {
    return 0;
  }

  for (; *str1; ++str1, ++str2)
  {
    c1 = CharTraitsU::ToLower(*str1);
    c2 = CharTraitsU::ToLower(*str2);

    if (c1 != c2)
    {
      return (int32)c1 - (int32)c2;
    }

    if (--n == 0)
    {
      return 0;
    }
  }

  return -(int32)*str2;
}


} // namespace fun
