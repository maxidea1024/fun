#include "fun/base/cstring/ucstring.h"
#include "fun/base/unsafe_memory.h"


namespace fun {


UNICHAR* ucsdup(const UNICHAR* str)
{
  const size_t len = ucslen(str) + 1;
  UNICHAR* copy = UnsafeMemory::Malloc(len * sizeof(UNICHAR));
  if (copy == nullptr)
  {
    return nullptr;
  }
  umemcpy(copy, str, len);
  return copy;
}


} // namespace fun
