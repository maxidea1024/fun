#include "fun/base/cstring/ucstring.h"
#include "fun/base/unicode.h"


namespace fun {


UNICHAR* ucsichr(const UNICHAR* str, UNICHAR ch)
{
  ch = CharTraitsU::ToLower(ch);
  while (CharTraitsU::ToLower(*str) != ch && *str != '\0')
  {
    ++str;
  }
  if (CharTraitsU::ToLower(*str) == ch)
  {
    return (UNICHAR*)str;
  }
  return nullptr;
}


} // namespace fun
