//TODO type_traits.h로 옮겨주자.

#pragma once

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type is integral.
 */
template <typename T> struct IsIntegral : FalseType {};

template <> struct IsIntegral<uint8   > : TrueType {};
template <> struct IsIntegral<uint16  > : TrueType {};
template <> struct IsIntegral<uint32  > : TrueType {};
template <> struct IsIntegral<uint64  > : TrueType {};
template <> struct IsIntegral<int8    > : TrueType {};
template <> struct IsIntegral<int16   > : TrueType {};
template <> struct IsIntegral<int32   > : TrueType {};
template <> struct IsIntegral<int64   > : TrueType {};
template <> struct IsIntegral<bool    > : TrueType {};
template <> struct IsIntegral<WIDECHAR> : TrueType {};
template <> struct IsIntegral<ANSICHAR> : TrueType {};

template <typename T> struct IsIntegral<const T         > { enum { Value = IsIntegral<T>::Value }; };
template <typename T> struct IsIntegral<volatile T      > { enum { Value = IsIntegral<T>::Value }; };
template <typename T> struct IsIntegral<const volatile T> { enum { Value = IsIntegral<T>::Value }; };

} // namespace tl
} // namespace fun
