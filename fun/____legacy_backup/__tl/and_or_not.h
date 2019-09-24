//TODO 이름을 logical로 바꿔야하나??
//TODO 구지 필요할까??

#pragma once

namespace fun {
namespace tl {

template <typename... Types>
struct And;

template <bool LhsValue, typename... rhs>
struct AndValue
{
  enum { value = And<rhs...>::Value };
};

template <typename... rhs>
struct AndValue<false, rhs...> : FalseType {};

template <typename lhs, typename... rhs>
struct And<lhs, rhs...> : AndValue<lhs::value, rhs...>
{
};

template <> struct And<> : TrueType {};

/**
 * Does a boolean OR of the ::value static members of each type, but short-circuits if any Type::value == true.
 */
template <typename... Types>
struct Or;

template <bool LhsValue, typename... rhs>
struct OrValue
{
  enum { value = Or<rhs...>::Value };
};

template <typename... rhs>
struct OrValue<true, rhs...> : TrueType {};

template <typename lhs, typename... rhs>
struct Or<lhs, rhs...> : OrValue<lhs::value, rhs...>
{
};

template <> struct Or<> : FalseType {};

/**
 * Does a boolean NOT of the ::value static members of the type.
 */
template <typename Type>
struct Not
{
  enum { value = !Type::value };
};

} // namespace tl
} // namespace fun
