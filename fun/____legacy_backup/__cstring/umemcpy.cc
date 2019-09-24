#include "fun/base/cstring/ucstring.h"
#include "fun/base/unsafe_memory.h"


namespace fun {


UNICHAR* umemcpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src, size_t n)
{
  return (UNICHAR*)UnsafeMemory::Memcpy(dst, src, n * sizeof(UNICHAR));
}


} // namespace fun
