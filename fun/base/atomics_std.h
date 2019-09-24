#pragma once

#include "fun/base/base.h"
#include "fun/base/atomics_generic.h"

namespace fun {

class StdAtomics : public GenericAtomics {
 public:
  FUN_ALWAYS_INLINE static int32 Increment(volatile int32* value) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int64 Increment(volatile int64* value) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int32 Decrement(volatile int32* value) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int64 Decrement(volatile int64* value) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int32 Add(volatile int32* value, int32 amount) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int64 Add(volatile int64* value, int64 amount) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int32 Exchange(volatile int32* value, int32 xchg) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int64 Exchange(volatile int64* value, int64 xchg) {
    //TODO
  }

  FUN_ALWAYS_INLINE static void* ExchangePtr(void** dst, void* xchg) {
    //TODO
  }

  FUN_ALWAYS_INLINE static int32 CompareExchange(volatile int32* dst, int32 xchg, int32 comparand) {
    //TODO
  }

#if FUN_64_BIT
  FUN_ALWAYS_INLINE static int64 CompareExchange(volatile int64* dst, int64 xchg, int64 comparand) {
    //TODO
  }
#endif

  FUN_ALWAYS_INLINE static void* CompareExchangePointer(void** dst, void* xchg, void* comparand) {
    //TODO
  }
};

typedef StdAtomics Atomics;

} // namespace fun
