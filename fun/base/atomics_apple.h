#pragma once

#include "fun/base/base.h"
#include "fun/base/atomics_generic.h"

namespace fun {

class AppleAtomics : public GenericAtomics {
 public:
  FUN_ALWAYS_INLINE static int32 Increment(volatile int32* value) {
    return (int32)OSAtomicIncrement32Barrier((int32_t*)value);
  }

  FUN_ALWAYS_INLINE static int64 Increment(volatile int64* value) {
    return (int64)OSAtomicIncrement64Barrier((int64_t*)value);
  }

  FUN_ALWAYS_INLINE static int32 Decrement(volatile int32* value) {
    return (int32)OSAtomicDecrement32Barrier((int32_t*)value);
  }

  FUN_ALWAYS_INLINE static int64 Decrement(volatile int64* value) {
    return (int64)OSAtomicDecrement64Barrier((int64_t*)value);
  }

  FUN_ALWAYS_INLINE static int32 Add(volatile int32* value, int32 amount) {
    return OSAtomicAdd32Barrier((int32_t)amount, (int32_t*)value) - amount;
  }

  FUN_ALWAYS_INLINE static int64 Add(volatile int64* value, int64 amount) {
    return OSAtomicAdd64Barrier((int64_t)amount, (int64_t*)value) - amount;
  }

  FUN_ALWAYS_INLINE static int32 Exchange(volatile int32* value, int32 xchg) {
    int32 ret;

    do {
      ret = *value;
    } while (!OSAtomicCompareAndSwap32Barrier(ret, xchg, (int32_t*)value));

    return ret;
  }

  FUN_ALWAYS_INLINE static int64 Exchange(volatile int64* value, int64 xchg) {
    int64 ret;

    do {
      ret = *value;
    } while (!OSAtomicCompareAndSwap64Barrier(ret, xchg, (int64_t*)value));

    return ret;
  }

  FUN_ALWAYS_INLINE static void* ExchangePtr(void** dst, void* xchg) {
    void* ret;

    do {
      ret = *dst;
    } while (!OSAtomicCompareAndSwapPtrBarrier(ret, xchg, dst));

    return ret;
  }

  FUN_ALWAYS_INLINE static int32 CompareExchange(volatile int32* dst, int32 xchg, int32 comparand) {
    int32 ret;
    do {
      if (OSAtomicCompareAndSwap32Barrier(comparand, xchg, (int32_t*)dst)) {
        return comparand;
      }
      ret = *dst;
    } while (ret == comparand);

    return ret;
  }

#if FUN_64_BIT
  FUN_ALWAYS_INLINE static int64 CompareExchange(volatile int64* dst, int64 xchg, int64 comparand) {
    int64 ret;
    do {
      if (OSAtomicCompareAndSwap64Barrier(comparand, xchg, (int64_t*)dst)) {
        return comparand;
      }
      ret = *dst;
    } while (ret == comparand);

    return ret;
  }
#endif

  FUN_ALWAYS_INLINE static void* CompareExchangePointer(void** dst, void* xchg, void* comparand) {
    void* ret;
    do {
      if (OSAtomicCompareAndSwapPtrBarrier(comparand, xchg, dst)) {
        return comparand;
      }
      ret = *dst;
    } while (ret == comparand);

    return ret;
  }
};

typedef AppleAtomics Atomics;

} // namespace fun
