#include "fun/base/cstring/ucstring.h"


namespace fun {


size_t ucslen(const UNICHAR* str)
{
  const UNICHAR* s = str;
  while (*s++);
  return s - str;
}


} // namespace fun
