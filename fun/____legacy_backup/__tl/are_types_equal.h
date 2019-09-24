#pragma once

namespace fun {
namespace tl {

//TODO IsSame하고 다른건지??

/**
 * Tests whether two typenames refer to the same type.
 */
template <typename A, typename B>
struct IsSame;

template <typename, typename>
struct IsSame : FalseType {};

template <typename A>
struct IsSame<A, A> : TrueType {};

#define ARE_TYPES_EQUAL(A, B)  IsSame<A, B>::Value

} // namespace tl
} // namespace fun
