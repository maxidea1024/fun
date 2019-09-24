#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/type_traits.h"

#if defined(_MSC_VER)
#include <stdlib.h> // builtins
#endif

namespace fun {

#if !defined(FUN_NO_BYTESWAP_BUILTINS)
  #if defined(_MSC_VER)
    #if (_MSC_VER > 1310) // >= Visual C++ .NET 2003 (7.1)
      #define FUN_HAVE_MSC_BYTESWAP 1
    #endif
  #elif defined(__clang__)
    #if __has_builtin(__builtin_bswap32)
      #define FUN_HAVE_GCC_BYTESWAP 1
    #endif
  #elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
    #define FUN_HAVE_GCC_BYTESWAP 1
  #endif
#endif

class ByteOrder {
 public:
  enum { IsLittleEndian = FUN_ARCH_LITTLE_ENDIAN };
  enum { IsBigEndian = !IsLittleEndian };

  FUN_ALWAYS_INLINE static void Flip(const void* src, void* dst, int32 count) {
    for (int32 i = 0; i < count; ++i) {
      static_cast<uint8*>(dst)[i] = static_cast<const uint8*>(src)[count - 1 - i];
    }
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void Flip(const T src, void* dst) {
    Flip(&src, dst, sizeof(T));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToUnaligned(const T src, void* dst) {
#if FUN_REQUIRES_ALIGNED_ACCESS
    UnsafeMemory::Memcpy(dst, &src, sizeof(T));
#else
    *((T*)dst) = src;
#endif
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T FromUnaligned(const void* src) {
#if FUN_REQUIRES_ALIGNED_ACCESS
    T dst;
    UnsafeMemory::Memcpy(&dst, src, sizeof(T));
    return dst;
#else
    return *((const T*)src);
#endif
  }

  FUN_ALWAYS_INLINE static uint64 Flip(uint64 src) {
#if FUN_HAVE_MSC_BYTESWAP
    return _byteswap_uint64(src);
#elif FUN_HAVE_GCC_BYTESWAP
    return __builtin_bswap64(src);
#else
    return
          ((src & 0x00000000000000FFULL) << 56)
        | ((src & 0x000000000000FF00ULL) << 40)
        | ((src & 0x0000000000FF0000ULL) << 24)
        | ((src & 0x00000000FF000000ULL) << 8)
        | ((src & 0x000000FF00000000ULL) >> 8)
        | ((src & 0x0000FF0000000000ULL) >> 24)
        | ((src & 0x00FF000000000000ULL) >> 40)
        | ((src & 0xFF00000000000000ULL) >> 56);
#endif
  }

  FUN_ALWAYS_INLINE static uint32 Flip(uint32 src) {
#if FUN_HAVE_MSC_BYTESWAP
    return _byteswap_ulong(src);
#elif FUN_HAVE_GCC_BYTESWAP
    return __builtin_bswap32(src);
#else
    return uint32(
          ((src & 0x000000FF) << 24)
        | ((src & 0x0000FF00) << 8)
        | ((src & 0x00FF0000) >> 8)
        | ((src & 0xFF000000) >> 24));
#endif
  }

  FUN_ALWAYS_INLINE static uint16 Flip(uint16 src) {
#if FUN_HAVE_MSC_BYTESWAP
    return _byteswap_ushort(src);
#elif FUN_HAVE_GCC_BYTESWAP
    return __builtin_bswap16(src);
#else
    return uint16(((src & 0x00FF) << 8) | ((src & 0xFF00) >> 8));
#endif
  }

  FUN_ALWAYS_INLINE static int64 Flip(int64 src) {
    return (int64)Flip(uint64(src));
  }

  FUN_ALWAYS_INLINE static int32 Flip(int32 src) {
    return (int32)Flip(uint32(src));
  }

  FUN_ALWAYS_INLINE static int16 Flip(int16 src) {
    return (int16)Flip(uint16(src));
  }


  // just for convenience

  FUN_ALWAYS_INLINE static uint8 Flip(uint8 src) {
    return src;
  }

  FUN_ALWAYS_INLINE static int8  Flip(int8 src) {
    return src;
  }

  FUN_ALWAYS_INLINE static float  Flip(float src) {
    union { float f; uint32 i; };
    f = src;
    i = Flip(i);
    return f;
  }

  FUN_ALWAYS_INLINE static double Flip(double src) {
    union { double d; uint64 i; };
    d = src; i = Flip(i);
    return d;
  }

  FUN_ALWAYS_INLINE static long Flip(long src) {
    return (long)Flip(uint32(src));
  }

  FUN_ALWAYS_INLINE static long Flip(unsigned long src) {
    return (unsigned long)Flip(uint32(src));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void Flip(T* array, size_t count) {
    while (count--) {
      *array = TypedFlip(*array);
      ++array;
    }
  }


  template <typename T> struct IsBool : FalseType {};
  template <> struct IsBool<bool> : TrueType {};
  template <> struct IsBool<const bool> : TrueType {};
  template <> struct IsBool<volatile bool> : TrueType {};
  template <> struct IsBool<const volatile bool> : TrueType {};

  template <typename T, size_t Size>
  struct IsFlippableIntegral {
    enum { Value = IsIntegral<T>::Value && !IsBool<T>::Value && sizeof(T) == Size }; // bool type은 제외.
  };

  template <typename T>
  static FUN_ALWAYS_INLINE typename EnableIf<IsFlippableIntegral<T,8>::Value,T>::Type TypedFlip(T src) {
    return (T)Flip<uint64>(src);
  }

  template <typename T>
  static FUN_ALWAYS_INLINE typename EnableIf<IsFlippableIntegral<T,4>::Value,T>::Type TypedFlip(T src) {
    return (T)Flip<uint32>(src);
  }

  template <typename T>
  static FUN_ALWAYS_INLINE typename EnableIf<IsFlippableIntegral<T,2>::Value,T>::Type TypedFlip(T src) {
    return (T)Flip<uint16>(src);
  }

  template <typename T>
  static FUN_ALWAYS_INLINE typename EnableIf<IsFlippableIntegral<T,1>::Value,T>::Type TypedFlip(T src) {
    return (T)Flip<uint8>(src);
  }


#if !FUN_ARCH_LITTLE_ENDIAN
  // BIG ENDIAN

  template <typename T>
  FUN_ALWAYS_INLINE static T ToBigEndian(T src) {
    return src;
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T FromBigEndian(T src) {
    return src;
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T ToLittleEndian(T src) {
    return Flip<T>(src);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T FromLittleEndian(T src) {
    return Flip<T>(src);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(T src, void* dst) {
    ToUnaligned(src, dst);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(T src, void* dst) {
    Flip(src, dst);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(const T* src, void* dst, size_t count) {
    UnsafeMemory::Memcpy(dst, src, count * sizeof(T));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(const T* src, void* dst, size_t count) {
    uint8* ptr = (uint8*)dst;
    while (count--) {
      Flip(*src, ptr);
      ++src;
      ptr += sizeof(T);
    }
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromBigEndian(T* array, size_t count) {
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(T* array, size_t count) {
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromLittleEndian(T* array, size_t count) {
    Flip(array, count);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(T* array, size_t count) {
    Flip(array, count);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromBigEndian(const T* src, T* dst, size_t count) {
    UnsafeMemory::Memcpy(dst, src, count * sizeof(T));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(const T* src, T* dst, size_t count) {
    UnsafeMemory::Memcpy(dst, src, count * sizeof(T));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromLittleEndian(const T* src, T* dst, size_t count) {
    while (count--) {
      *dst++ = Flip(*src++);
    }
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(const T* src, T* dst, size_t count) {
    while (count--) {
      *dst++ = Flip(*src++);
    }
  }
#else
  // LITTLE ENDIAN

  template <typename T>
  FUN_ALWAYS_INLINE static T ToBigEndian(T src) {
    return Flip(src);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T FromBigEndian(T src) {
    return Flip(src);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T ToLittleEndian(T src) {
    return src;
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T FromLittleEndian(T src) {
    return src;
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(T src, void* dst) {
    Flip(src, dst);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(T src, void* dst) {
    ToUnaligned(src, dst);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(const T* src, void* dst, size_t count) {
    uint8* ptr = (uint8*)dst;
    while (count--) {
      Flip(*src, ptr);
      ++src;
      ptr += sizeof(T);
    }
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(const T* src, void* dst, size_t count) {
    UnsafeMemory::Memcpy(dst, src, count * sizeof(T));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromBigEndian(T* array, size_t count) {
    Flip(array, count);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(T* array, size_t count) {
    Flip(array, count);
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromLittleEndian(T* array, size_t count) {
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(T* array, size_t count) {
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromBigEndian(const T* src, T* dst, size_t count) {
    while (count--) {
      *dst++ = Flip(*src++);
    }
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToBigEndian(const T* src, T* dst, size_t count) {
    while (count--) {
      *dst++ = Flip(*src++);
    }
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void FromLittleEndian(const T* src, T* dst, size_t count) {
    UnsafeMemory::Memcpy(dst, src, count * sizeof(T));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static void ToLittleEndian(const T* src, T* dst, size_t count) {
    UnsafeMemory::Memcpy(dst, src, count * sizeof(T));
  }
#endif

  template <typename T>
  FUN_ALWAYS_INLINE static T FromLittleEndian(const void* src) {
    return FromLittleEndian(FromUnaligned<T>(src));
  }

  template <typename T>
  FUN_ALWAYS_INLINE static T FromBigEndian(const void* src) {
    return FromBigEndian(FromUnaligned<T>(src));
  }

  // just for convenience
  template <>
  FUN_ALWAYS_INLINE static uint8 FromLittleEndian<uint8>(const void* src) {
    return static_cast<const uint8*>(src)[0];
  }

  template <>
  FUN_ALWAYS_INLINE static int8 FromLittleEndian<int8>(const void* src) {
    return static_cast<const int8*>(src)[0];
  }
};

} // namespace fun
