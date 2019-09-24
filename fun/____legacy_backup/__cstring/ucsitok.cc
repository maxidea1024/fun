#include "fun/base/cstring/ucstring.h"
#include "fun/base/unicode.h"


namespace fun {


UNICHAR* ucsitok(UNICHAR* __restrict str, const UNICHAR* __restrict delim,
                UNICHAR** __restrict last)
{
  const UNICHAR* spanp;
  UNICHAR* tok;
  UNICHAR c;
  UNICHAR sc;

  if (str == nullptr && (str = *last) == nullptr)
  {
    return nullptr;
  }

  // Skip (span) leading delimiters (str += ucsspn(str, delim), sort of).
Cont:
  c = CharTraitsU::ToLower(*str++);
  for (spanp = delim; (sc = CharTraitsU::ToLower(*spanp++)) != '\0';)
  {
    if (c == sc)
    {
      goto Cont;
    }
  }

  if (c == '\0') // no non-delimiter characters
  {
    *last = nullptr;
    return nullptr;
  }
  tok = str - 1;

  // Scan token (scan for delimiters: str += ucscspn(str, delim), sort of).
  // Note that delim must have one NUL; we stop if we see that, too.
  for (;;)
  {
    c = CharTraitsU::ToLower(*str++);
    spanp = delim;
    do
    {
      if ((sc = CharTraitsU::ToLower(*spanp++)) == c)
      {
        if (c == '\0')
        {
          str = nullptr;
        }
        else
        {
          str[-1] = '\0';
        }
        *last = str;
        return tok;
      }
    } while (sc != '\0');
  }
  // NOTREACHED
}


} // namespace fun
