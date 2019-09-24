#pragma once

#include "fun/net/net.h"
#include <assert.h>

namespace fun {

template <uint32 ARRAY_LENGTH>
class StackGuard {
 public:
  const uint8 FILLER_VALUE = 0xCC;
  uint8 sentinel[ARRAY_LENGTH];

  StackGuard() {
    //auto start = (reinterpret_cast<intptr_t>(sentinel) + 4095) & ~4095;
    //auto byteCount = reinterpret_cast<size_t>(sentinel + ARRAY_LENGTH - start) & ~4095;
    //
    //DWORD oldProtect;
    //auto result = VirtualProtect(reinterpret_cast<void*>(start), byteCount, PAGE_READONLY, &oldProtect);
    //assert(result);
  }
  
  ~StackGuard() {
    //auto start = (reinterpret_cast<intptr_t>(sentinel) + 4095) & ~4095;
    //auto byteCount = reinterpret_cast<size_t>(sentinel + ARRAY_LENGTH - start) & ~4095;
    //
    //DWORD oldProtect;
    //auto result = VirtualProtect(reinterpret_cast<void*>(start), byteCount, PAGE_READWRITE, &oldProtect);
    //assert(result);
  }

  inline void Check() const {
    if (this == nullptr) {
      return;
    }

    for (int32 i = 0; i < countof(sentinel); i++) {
      const int32 offset = i;
      //assert(sentinel[offset] == FILLER_VALUE);
      if (sentinel[offset] != FILLER_VALUE) {
        int halted = 0;
      }
    }
  }
};

//extern StackGuard<2048>* GGuard;

} // namespace fun
