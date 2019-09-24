#pragma once

#include "fun/base/base.h"
#include "fun/base/atomics_generic.h"

namespace fun {

class UnixAtomics : public GenericAtomics {
 public:
  FUN_ALWAYS_INLINE static int32 Increment(volatile int32* value) {
    return __sync_fetch_and_add(value, 1) + 1;
  }

  FUN_ALWAYS_INLINE static int64 Increment(volatile int64* value) {
    return __sync_fetch_and_add(value, 1) + 1;
  }

  FUN_ALWAYS_INLINE static int32 Decrement(volatile int32* value) {
    return __sync_fetch_and_sub(value, 1) - 1;
  }

  FUN_ALWAYS_INLINE static int64 Decrement(volatile int64* value) {
    return __sync_fetch_and_sub(value, 1) - 1;
  }

  FUN_ALWAYS_INLINE static int32 Add(volatile int32* value, int32 amount) {
    return __sync_fetch_and_add(value, amount);
  }

  FUN_ALWAYS_INLINE static int64 Add(volatile int64* value, int64 amount) {
    return __sync_fetch_and_add(value, amount);
  }

  FUN_ALWAYS_INLINE static int32 Exchange(volatile int32* value, int32 xchg) {
    return __sync_lock_test_and_set(value, xchg);
  }

  FUN_ALWAYS_INLINE static int64 Exchange(volatile int64* value, int64 xchg) {
    return __sync_lock_test_and_set(value, xchg);
  }

  FUN_ALWAYS_INLINE static void* ExchangePtr(void** dst, void* xchg) {
    return __sync_lock_test_and_set(dst, xchg);
  }

  FUN_ALWAYS_INLINE static int32 CompareExchange(volatile int32* dst, int32 xchg, int32 comparand) {
    return __sync_val_compare_and_swap(dst, comparand, xchg);
  }

#if FUN_64_BIT
  FUN_ALWAYS_INLINE static int64 CompareExchange(volatile int64* dst, int64 xchg, int64 comparand) {
    return __sync_val_compare_and_swap(dst, comparand, xchg);
  }
#endif

  FUN_ALWAYS_INLINE static void* CompareExchangePointer(void** dst, void* xchg, void* comparand) {
    return __sync_val_compare_and_swap(dst, comparand, xchg);
  }
};

typedef UnixAtomics Atomics;

} // namespace fun
