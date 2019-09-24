//TODO type_traits.h로 옮겨주자.

#pragma once

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type is floating point.
 */
template <typename T> struct IsFloatingPoint : FalseType {};

template <> struct IsFloatingPoint<float      > : TrueType {};
template <> struct IsFloatingPoint<double     > : TrueType {};
template <> struct IsFloatingPoint<long double> : TrueType {};

template <typename T> struct IsFloatingPoint<const T         > { enum { Value = IsFloatingPoint<T>::Value }; };
template <typename T> struct IsFloatingPoint<volatile T      > { enum { Value = IsFloatingPoint<T>::Value }; };
template <typename T> struct IsFloatingPoint<const volatile T> { enum { Value = IsFloatingPoint<T>::Value }; };

} // namespace tl
} // namespace fun
