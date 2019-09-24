//TODO type_traits.h로 옮겨주자.

#pragma once

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type is arithmetic.
 */
template <typename T> struct IsArithmetic : FalseType {};

template <> struct IsArithmetic<float      > : TrueType {};
template <> struct IsArithmetic<double     > : TrueType {};
template <> struct IsArithmetic<long double> : TrueType {};
template <> struct IsArithmetic<uint8      > : TrueType {};
template <> struct IsArithmetic<uint16     > : TrueType {};
template <> struct IsArithmetic<uint32     > : TrueType {};
template <> struct IsArithmetic<uint64     > : TrueType {};
template <> struct IsArithmetic<int8       > : TrueType {};
template <> struct IsArithmetic<int16      > : TrueType {};
template <> struct IsArithmetic<int32      > : TrueType {};
template <> struct IsArithmetic<int64      > : TrueType {};
template <> struct IsArithmetic<bool       > : TrueType {};
template <> struct IsArithmetic<WIDECHAR   > : TrueType {};
template <> struct IsArithmetic<ANSICHAR   > : TrueType {};

//TODO 기타 유니코드 타입은?

template <typename T> struct IsArithmetic<const T         > { enum { Value = IsArithmetic<T>::Value }; };
template <typename T> struct IsArithmetic<volatile T      > { enum { Value = IsArithmetic<T>::Value }; };
template <typename T> struct IsArithmetic<const volatile T> { enum { Value = IsArithmetic<T>::Value }; };

} // namespace tl
} // namespace fun
