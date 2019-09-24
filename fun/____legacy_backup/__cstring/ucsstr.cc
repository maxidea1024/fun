#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucsstr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub)
{
  UNICHAR c, sc;
  size_t len;

  if ((c = *sub++) != '\0')
  {
    len = ucslen(sub);
    do
    {
      do
      {
        if ((sc = *str++) == '\0')
        {
          return nullptr;
        }
      } while (sc != c);
    } while (ucsncmp(str, sub, len) != 0);

    --str;
  }

  return (UNICHAR*)str;
}


} // namespace fun
