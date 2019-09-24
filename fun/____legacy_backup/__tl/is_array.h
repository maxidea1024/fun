//TODO type_traits.h로 옮겨주자.

#pragma once

#include "core_types.h"
#include "fun/base/ftl/integral_constant.h"

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type is a C++ array.
 */
template <typename T>           struct IsArray       : FalseType {};
template <typename T>           struct IsArray<T[]>  : TrueType {};
template <typename T, size_t N> struct IsArray<T[N]> : TrueType {};

/**
 * Traits class which tests if a type is a bounded C++ array.
 */
template <typename T>           struct IsBoundedArray       : FalseType {};
template <typename T, size_t N> struct IsBoundedArray<T[N]> : TrueType {};

/**
 * Traits class which tests if a type is an unbounded C++ array.
 */
template <typename T> struct IsUnboundedArray      : FalseType {};
template <typename T> struct IsUnboundedArray<T[]> : TrueType {};

} // namespace tl
} // namespace fun
