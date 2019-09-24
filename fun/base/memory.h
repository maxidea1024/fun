#pragma once

#include "fun/base/base.h"

/*
#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/memory_win32.h"
//#elif PLATFORM_PS4
//#include "fun/base/memory_ps4.h"
//#elif PLATFORM_XBOXONE
//#include "fun/base/memory_xbox.h"
//#elif PLATFORM_MAC
//#include "fun/base/memory_mac.h"
//#elif PLATFORM_IOS
//#include "fun/base/memory_ios.h"
//#elif PLATFORM_ANDROID
//#include "fun/base/memory_android.h"
//#elif PLATFORM_WINRT
#include "fun/base/memory_winrt.h"
//#elif PLATFORM_HTML5
#include "fun/base/memory_html5.h"
#elif PLATFORM_LINUX
#include "fun/base/memory_linux.h"
#else
#error This platform is not yet supported.
#endif
*/

#include "fun/base/memory_base.h"

namespace fun {

// 이하는 임시 코드임.

class FUN_BASE_API UnsafeMemory {
 public:
  inline static void* Memmove(void* dst, const void* src, size_t count) {
    return memmove(dst, src, count);
  }

  inline static int32 Memcmp(const void* lhs, const void* rhs, size_t count) {
    return memcmp(lhs, rhs, count);
  }

  inline static bool Memequals(const void* lhs, const void* rhs, size_t count) {
    return memcmp(lhs, rhs, count) == 0;
  }

  inline static void* Memset(void* dst, uint8 ch, size_t count) {
    return memset(dst, ch, count);
  }

  // template <typename T>
  // inline static void Memset(T& dst, uint8 ch) {
  //  static_assert(!IsPointer<T>::Value, "For pointers use the three parameters
  //  function"); memset(&dst, ch, sizeof(T));
  //}

  inline static void* Memzero(void* dst, size_t count) {
    return memset(dst, 0, count);
  }

  // template <typename T>
  // inline static void Memzero(T& dst) {
  //  static_assert(!IsPointer<T>::Value, "For pointers use the three parameters
  //  function"); Memzero(&dst, sizeof(T));
  //}

  inline static void* Memcpy(void* dst, const void* src, size_t count) {
    return memcpy(dst, src, count);
  }

  // template <typename T>
  // inline static void Memcpy(T& dst, const T& src) {
  //  static_assert(!IsPointer<T>::Value, "For pointers use the three parameters
  //  function"); Memcpy(&dst, &src, sizeof(T));
  //}

  inline static void* BigBlockMemcpy(void* dst, const void* src, size_t count) {
    return Memcpy(dst, src, count);
  }

  inline static void* StreamingMemcpy(void* dst, const void* src,
                                      size_t count) {
    return Memcpy(dst, src, count);
  }

  inline static void Memswap(void* lhs, void* rhs, size_t count) {
    // PlatformMemory::Memswap(lhs, rhs, count);
    // TODO
  }

  inline static void* SystemMalloc(size_t size) { return std::malloc(size); }
  inline static void SystemFree(void* ptr) { std::free(ptr); }

  static void* Malloc(size_t size, uint32 align = DEFAULT_ALIGNMENT);
  static void* Realloc(void* ptr, size_t new_size,
                       uint32 align = DEFAULT_ALIGNMENT);
  static void Free(void* ptr);
  static size_t GetAllocSize(void* ptr);

  /**
   * For some allocators this will return the actual size
   * that should be requested to eliminate internal fragmentation.
   * The return value will always be >= Count. This can be used to grow
   * and shrink containers to optimal sizes.
   * This call is always fast and threadsafe with no locking.
   */
  static size_t QuantizeSize(size_t size, uint32 align = DEFAULT_ALIGNMENT);

  /**
   * Releases as much memory as possible. Must be called from the main thread.
   */
  static void Trim();

  /**
   * Set up TLS caches on the current thread.
   * These are the threads that we can trim.
   */
  static void SetupTlsCachesOnCurrentThread();

  /**
   * Clears the TLS caches on the current thread and
   * disables any future caching.
   */
  static void ClearAndDisableTlsCachesOnCurrentThread();

 private:
  static void GCreateMalloc();

  // These versions are called either at startup or in the event of a crash
  static void* MallocExternal(size_t size, uint32 align = DEFAULT_ALIGNMENT);
  static void* ReallocExternal(void* ptr, size_t new_size,
                               uint32 align = DEFAULT_ALIGNMENT);
  static void FreeExternal(void* ptr);
  static size_t GetAllocSizeExternal(void* ptr);
  static size_t QuantizeSizeExternal(size_t size,
                                     uint32 align = DEFAULT_ALIGNMENT);
};

}  // namespace fun
