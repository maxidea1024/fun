#pragma once

#include "fun/base/base.h"

//TODO 이름을 겹치지 않게 변경해주어야할듯 한데...
//너무 일반적이다.
//namespace로 묶어주어야할까??

namespace fun {
namespace tl {

template <typename T, typename... Args>
struct Back
{
  using Type = typename Back<Args...>::Type;
};

template <typename T>
struct Back<T>
{
  using Type = T;
};

template <typename T, typename... Args>
struct Front
{
  using Type = T;
};

template <typename T1, T2, typename... Ts>
struct AreSameTypes
{
  static constexpr bool Value = IsSame<T1,T2>::Value ? true : AreSameTypes<T1,Ts...>::Value;
};

template <typename T1, typename T2>
struct AreSameTypes<T1,T2>
{
  static constexpr bool Value = IsSame<T1,T2>::Value;
};

template <typename T, typename... Args>
struct AreDifferentTypes
{
  static constexpr bool Value = AreSameTypes<T, Args...>::Value ? false : AreDifferentTypes<Args...>::Value;
};

template <typename T1>
struct AreDifferentTypes<T1>
{
  static constexpr bool Value = true;
};

} // namespace tl
} // namespace fun
