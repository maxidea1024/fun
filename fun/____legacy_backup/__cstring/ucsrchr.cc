#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucsrchr(const UNICHAR* str, UNICHAR ch)
{
  const UNICHAR* last;

  last = nullptr;
  for (;;)
  {
    if (*str == ch)
    {
      last = str;
    }
    
    if (*str == '\0')
    {
      break;
    }

    ++str;
  }

  return (UNICHAR*)last;
}


} // namespace fun
