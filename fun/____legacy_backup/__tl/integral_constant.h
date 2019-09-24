#pragma once

namespace fun {
namespace tl {

template <typename T, T ConstValue>
struct IntegralConstant
{
  static const T Value = ConstValue;

  typedef T ValueType;
  typedef IntegralConstant<T,ConstValue> Type;

  operator ValueType() const
  {
    return Value;
  }
};

template <bool BoolValue>
struct BoolConstant
{
  enum { Value = BoolValue };
};

typedef BoolConstant<true>  TrueType;
typedef BoolConstant<false> FalseType;

} // namespace tl
} // namespace fun
