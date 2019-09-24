#pragma once

#include "fun/base/base.h"

namespace fun {

namespace HashOperator_LeftShift_internal {

struct NotSpecified {};

template <typename T>
struct ReturnValueCheck {
  static char (&Func())[2];
};

template <>
struct ReturnValueCheck<NotSpecified> {
  static char (&Func())[1];
};

template <typename Dest, typename T>
NotSpecified operator << (Dest&&, T&&);

template <typename T>
T&& Make();

template <typename Dest, typename T, typename = decltype(Make<Dest&>() << Make<T&>())>
struct HashOperator_LeftShift {
  enum { Value = true };
};

template <typename Dest, typename T>
struct HashOperator_LeftShift<Dest, T, NotSpecified> {
  enum { Value = false };
};

} // HashOperator_LeftShift_internal

/**
 * Traits class which tests if a type has an `operator<<` overload between two types.
 */
template <typename Dest, typename T>
struct HashOperator_LeftShift {
  enum { Value = HashOperator_LeftShift_internal::HashOperator_LeftShift<Dest, T>::Value };
};

} // namespace
