//TODO type_traits.h로 옮겨주자.

#pragma once

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type is a signed integral type.
 */
template <typename T> struct IsSigned : FalseType {};

template <> struct IsSigned<int8 > : TrueType {};
template <> struct IsSigned<int16> : TrueType {};
template <> struct IsSigned<int32> : TrueType {};
template <> struct IsSigned<int64> : TrueType {};

template <typename T> struct IsSigned<const T         > { enum { Value = IsSigned<T>::Value }; };
template <typename T> struct IsSigned<volatile T      > { enum { Value = IsSigned<T>::Value }; };
template <typename T> struct IsSigned<const volatile T> { enum { Value = IsSigned<T>::Value }; };

} // namespace tl
} // namespace fun
