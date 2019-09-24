#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucsristr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub)
{
  const size_t str_len = ucslen(str);
  const size_t sub_len = ucslen(sub);
  
  if (sub_len > str_len)
  {
    return nullptr;
  }

  const UNICHAR* s = str + str_len - sub_len;
  do
  {
    if (ucsnicmp(s, sub, sub_len) == 0)
    {
      return (UNICHAR*)s;
    }
    --s;
  } while (s >= str);

  return nullptr;
}


} // namespace fun
