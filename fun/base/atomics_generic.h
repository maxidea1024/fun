#pragma once

#include "fun/base/base.h"

namespace fun {

//TODO 전역 타입쪽으로 옮겨주는게 좋을듯..
struct alignas(16) Int128 {
  int64 low;
  int64 high;
};


class GenericAtomics {
 public:
  FUN_ALWAYS_INLINE static bool CanUseCompareExchange128() { return false; }

 protected:
  FUN_ALWAYS_INLINE static bool IsAligned(const volatile void* ptr, const uint32 align = sizeof(void*)) {
    return !(intptr_t(ptr) & (align - 1));
  }
};

} // namespace fun
