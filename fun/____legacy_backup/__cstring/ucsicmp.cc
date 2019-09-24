#include "fun/base/cstring/ucstring.h"
#include "fun/base/cstring/unicode.h"


namespace fun {


int32 ucsicmp(const UNICHAR* str1, const UNICHAR* str2)
{
  while (CharTraitsU::ToLower(*str1) == CharTraitsU::ToLower(*str2++))
  {
    if (*str1++ == '\0')
    {
      return 0;
    }
  }

  --str2;
  return (int32)CharTraitsU::ToLower(*str1) - (int32)CharTraitsU::ToLower(*str2);
}


} // namespace fun
