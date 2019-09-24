#pragma once

#include "fun/base/base.h"
#include "fun/base/serialization/archive.h"

namespace fun {

namespace HasSerializerOperator_internal {

struct NotSpecified {};

template <typename T>
struct ReturnValueCheck {
  static char (&Func())[2];
};

template <>
struct ReturnValueCheck<NotSpecified> {
  static char (&Func())[1];
};

template <typename T>
NotSpecified operator & (Archive&&, T&&);

template <typename T>
T&& Make();

template <typename T, typename = decltype(Make<Archive&>() & Make<T&>())>
struct HasSerializerOperator {
  static constexpr bool Value = true;
};

template <typename T>
struct HasSerializerOperator<T, NotSpecified> {
  static constexpr bool Value = false;
};

} // HasSerializerOperator_internal

/**
 * Traits class which tests if a type has an `operator&` overload between two types.
 */
template <typename T>
struct HasSerializerOperator {
  static constexpr bool Value = HasSerializerOperator_internal::HasSerializerOperator<T>::Value;
};

} // namespace
