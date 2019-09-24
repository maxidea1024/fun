#include "fun/base/cstring/ucstring.h"
#include "fun/base/unicode.h"


namespace fun {


UNICHAR* ucsistr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub)
{
  UNICHAR c, sc;
  size_t len;

  if ((c = CharTraitsU::ToLower(*sub++)) != '\0')
  {
    len = ucslen(sub);
    do
    {
      do
      {
        if ((sc = CharTraitsU::ToLower(*str++)) == '\0')
        {
          return nullptr;
        }
      } while (sc != c);
    } while (ucsnicmp(str, sub, len) != 0);
    --str;
  }

  return (UNICHAR*)str;
}


} // namespace fun
