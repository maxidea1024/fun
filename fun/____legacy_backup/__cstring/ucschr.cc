#include "fun/base/cstring/ucstring.h"


namespace fun {


UNICHAR* ucschr(const UNICHAR* str, UNICHAR ch)
{
  while (*str != ch && *str != '\0') ++str;
  if (*str == ch)
  {
    return (UNICHAR*)str;
  }
  return nullptr;
}


} // namespace fun
