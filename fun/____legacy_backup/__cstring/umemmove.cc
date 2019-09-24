#include "fun/base/cstring/ucstring.h"
#include "fun/base/unsafe_memory.h"


namespace fun {


UNICHAR* umemmove(UNICHAR* dst, const UNICHAR* src, size_t n)
{
  return (UNICHAR*)UnsafeMemory::Memmove(dst, src, n * sizeof(UNICHAR));
}


} // namespace fun
