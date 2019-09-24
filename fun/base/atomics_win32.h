#pragma once

#include "fun/base/base.h"
#include "fun/base/atomics_generic.h"

//TODO inline으로 하자니 포함은 시켜야하겠는데...
//이렇게 되면 windows_less.h를 전역으로 포함하게 되는...
#include "fun/base/windows_less.h"

namespace fun {

class WindowsAtomics : public GenericAtomics {
 public:
  FUN_ALWAYS_INLINE static int32 Increment(volatile int32* value) {
    return (int32)::_InterlockedIncrement((long*)value);
  }

#if FUN_PLATFORM_HAS_64BIT_ATOMICS
  FUN_ALWAYS_INLINE static int64 Increment(volatile int64* value) {
  #if FUN_64_BIT
    return (int64)::_InterlockedIncrement64((int64*)value);
  #else
    for (;;) {
      int64 old = *value;
      if (::_InterlockedCompareExchange64(value, old + 1, old) == old) {
        return old + 1;
      }
    }
  #endif
  }
#endif

  FUN_ALWAYS_INLINE static int32 Decrement(volatile int32* value) {
    return (int32)::_InterlockedDecrement((long*)value);
  }

#if FUN_PLATFORM_HAS_64BIT_ATOMICS
  FUN_ALWAYS_INLINE static int64 Decrement(volatile int64* value) {
  #if FUN_64_BIT
    return (int64)::_InterlockedDecrement64((int64*)value);
  #else
    for (;;) {
      int64 old = *value;
      if (::_InterlockedCompareExchange64(value, old - 1, old) == old) return old-1;
    }
  #endif
  }
#endif

  FUN_ALWAYS_INLINE static int32 Add(volatile int32* value, int32 amount) {
    return (int32)::_InterlockedExchangeAdd((long*)value, (long)amount);
  }

#if FUN_PLATFORM_HAS_64BIT_ATOMICS
  FUN_ALWAYS_INLINE static int64 Add(volatile int64* value, int64 amount) {
  #if FUN_64_BIT
    return (int64)::_InterlockedExchangeAdd64((int64*)value, (int64)amount);
  #else
    for (;;) {
      int64 old = *value;
      if (::_InterlockedCompareExchange64(value, old + amount, old) == old) {
        return old + amount;
      }
    }
  #endif
  }
#endif

  FUN_ALWAYS_INLINE static int32 Exchange(volatile int32* value, int32 xchg) {
    return (int32)::_InterlockedExchange((long*)value, (long)xchg);
  }

#if FUN_PLATFORM_HAS_64BIT_ATOMICS
  FUN_ALWAYS_INLINE static int64 Exchange(volatile int64* value, int64 xchg) {
  #if FUN_64_BIT
    return (int64)::_InterlockedExchange64((int64*)value, (int64)xchg);
  #else
    for (;;) {
      int64 old = *value;
      if (::_InterlockedCompareExchange64(value, xchg, old) == old) {
        return old;
      }
    }
  #endif
  }
#endif

  FUN_ALWAYS_INLINE static void* ExchangePtr(void** dst, void* xchg) {
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
    if (IsAligned(dst) == false) {
      HandleAtomicsFailure("InterlockedExchangePointer requires dst pointer to be aligned to %d bytes", sizeof(void*));
    }
#endif

  #if FUN_64_BIT
    return (void*)::_InterlockedExchange64((int64*)dst, (int64)xchg);
  #else
    return (void*)::_InterlockedExchange((long*)(dst), (long)(xchg));
  #endif
  }

  FUN_ALWAYS_INLINE static int32 CompareExchange(volatile int32* dst, int32 xchg, int32 comparand) {
    return (int32)::_InterlockedCompareExchange((long*)dst, (long)xchg, (long)comparand);
  }

#if FUN_PLATFORM_HAS_64BIT_ATOMICS
  FUN_ALWAYS_INLINE static int64 CompareExchange(volatile int64* dst, int64 xchg, int64 comparand) {
  #if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
    if (IsAligned(dst) == false) {
      HandleAtomicsFailure("InterlockedCompareExchangePointer requires dst pointer to be aligned to %d bytes", sizeof(void*));
    }
  #endif

    return (int64)::_InterlockedCompareExchange64(dst, xchg, comparand);
  }

  FUN_ALWAYS_INLINE static int64 AtomicRead64(volatile const int64* src) {
    return InterlockedCompareExchange((volatile int64*)src, 0, 0);
  }
#endif

  /**
   * The function compares the Destination value with the comparand value:
   *     - If the Destination value is equal to the comparand value, the xchg value is stored in the address specified by Destination,
   *     - Otherwise, the initial value of the Destination parameter is stored in the address specified specified by comparand.
   *
   * \return true if comparand equals the original value of the Destination parameter, or false if comparand does not equal the original value of the Destination parameter.
   *
   * Early AMD64 processors lacked the CMPXCHG16B instruction.
   * To determine whether the processor supports this operation, call the IsProcessorFeaturePresent function with PF_COMPARE64_EXCHANGE128.
   */
#if FUN_PLATFORM_HAS_128BIT_ATOMICS
  FUN_ALWAYS_INLINE static bool CompareExchange128(volatile Int128* dst, const Int128& xchg, Int128* comparand) {
  #if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
    if (IsAligned(dst, 16) == false) {
      HandleAtomicsFailure("InterlockedCompareExchangePointer requires dst pointer to be aligned to 16 bytes");
    }

    if (IsAligned(comparand, 16) == false) {
      HandleAtomicsFailure("InterlockedCompareExchangePointer requires comparand pointer to be aligned to 16 bytes");
    }
  #endif

    return ::_InterlockedCompareExchange128((int64 volatile *)dst, xchg.high, xchg.low, (int64*)comparand) == 1;
  }

  FUN_ALWAYS_INLINE static void AtomicRead128(volatile const Int128* src, Int128* dst) {
    Int128 zero;
    zero.high = 0;
    zero.low = 0;
    *dst = zero;
    InterlockedCompareExchange128((volatile Int128*)src, zero, dst);
  }
#endif //FUN_PLATFORM_HAS_128BIT_ATOMICS

  FUN_ALWAYS_INLINE static void* CompareExchangePointer(void** dst, void* xchg, void* comparand) {
  #if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
    if (!IsAligned(dst)) {
      HandleAtomicsFailure("InterlockedCompareExchangePointer requires dst pointer to be aligned to %d bytes", sizeof(void*));
    }
  #endif

  #if FUN_64_BIT
    return (void*)::_InterlockedCompareExchange64((int64 volatile*)dst, (int64)xchg, (int64)comparand);
  #else
    return (void*)_InterlockedCompareExchange((long volatile*)dst, (long)xchg, (long)comparand);
  #endif
  }

  /**
   * \return true, if the processor we are running on can execute compare and exchange 128-bit operation.
   *
   * @see cmpxchg16b, early AMD64 processors don't support this operation.
   */
  FUN_ALWAYS_INLINE static bool CanUseCompareExchange128() {
    return !!IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE128);
  }

 protected:
  /**
   * Handles atomics function failure.
   *
   * Since 'check' has not yet been declared here we need to call external function to use it.
   *
   * \param fmt - The string format string.
   */
  static void HandleAtomicsFailure(const char* fmt, ...);
};

typedef WindowsAtomics Atomics;

} // namespace fun
