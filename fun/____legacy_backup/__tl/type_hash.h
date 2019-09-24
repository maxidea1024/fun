#pragma once

#include "fun/base/crc.h"

namespace fun {
namespace tl {

/**
 * Combines two hash values to get a third.
 * Note - this function is not commutative.
 */
inline uint32 HashCombine(uint32 a, uint32 c)
{
  uint32 b = 0x9e3779b9;
  a += b;

  a -= b; a -= c; a ^= (c >> 13);
  b -= c; b -= a; b ^= (a << 8);
  c -= a; c -= b; c ^= (b >> 13);
  a -= b; a -= c; a ^= (c >> 12);
  b -= c; b -= a; b ^= (a << 16);
  c -= a; c -= b; c ^= (b >> 5);
  a -= b; a -= c; a ^= (c >> 3);
  b -= c; b -= a; b ^= (a << 10);
  c -= a; c -= b; c ^= (b >> 15);

  return c;
}

inline uint32 PointerHash(const void* key, uint32 c = 0)
{
  // Avoid lhs stalls on PS3 and Xbox 360
#if FUN_PTR_IS_64_BIT
  // Ignoring the lower 4 bits since they are likely zero anyway.
  // Higher bits are more significant in 64 bit builds.
  auto p = reinterpret_cast<uintptr_t>(key) >> 4;
#else
  auto p = reinterpret_cast<uintptr_t>(key);
#endif

  return HashCombine(p, c);
}


//
// Hash functions for common types.
//

inline uint32 HashOf(const uint8 x)
{
  return x;
}

inline uint32 HashOf(const int8 x)
{
  return x;
}

inline uint32 HashOf(const uint16 x)
{
  return x;
}

inline uint32 HashOf(const int16 x)
{
  return x;
}

inline uint32 HashOf(const int32 x)
{
  return x;
}

inline uint32 HashOf(const uint32 x)
{
  return x;
}

inline uint32 HashOf(const uint64 x)
{
  return (uint32)x + ((uint32)(x >> 32) * 23);
}

inline uint32 HashOf(const int64 x)
{
  return (uint32)x + ((uint32)(x >> 32) * 23);
}

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
inline uint32 HashOf(const __uint128_t x)
{
  const uint64 lo = (uint64)x;
  const uint64 hi = (uint64)(x >> 64);
  return HashOf(lo) ^ HashOf(hi);
}
#endif

inline uint32 HashOf(float x)
{
  return *(uint32*)&x;
}

inline uint32 HashOf(double x)
{
  return HashOf(*(uint64*)&x);
}

inline uint32 HashOf(const char* x)
{
  return Crc::StringCrc32(x);
}

inline uint32 HashOf(const UNICHAR* x)
{
  return Crc::StringCrc32(x);
}

inline uint32 HashOf(const void* x)
{
  return PointerHash(x);
}

inline uint32 HashOf(void* x)
{
  return PointerHash(x);
}

template <typename EnumType> inline
typename EnableIf<IsEnum<EnumType>::Value, uint32>::Type HashOf(EnumType e)
{
  return HashOf((__underlying_type(EnumType))e);
}

//template <typename WrappedEnumType> inline
//typename EnableIf<IsWrappedEnum<WrappedEnumType>::Value, uint32>::Type HashOf(WrappedEnumType e)
//{
//  return HashOf(e.value);
//}

} // namespace tl
} // namespace fun
