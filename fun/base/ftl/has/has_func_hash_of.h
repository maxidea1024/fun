#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

namespace HasHashOf_internal {

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
NotSpecified HashOf(const T&);

template <typename T>
const T& Make();

template <typename T, bool IsHashableScalarType = Or<IsArithmetic<T>, IsPointer<T>, IsEnum<T>>::Value>
struct HashOfQuery {
  // All arithmetic, pointer and enums types are hashable
  static constexpr bool Value = true;
};

template <typename T>
struct HashOfQuery<T, false> {
  static constexpr bool Value = sizeof(ReturnValueCheck<decltype(HashOf(Make<T>()))>::Func()) == sizeof(char[2]);
};

} // namespace HasHashOf_internal

//TODO 반환값은 체크하지 못하는건가?? 뭐 딱히 필요는 없어보이지만...

/**
 * Traits class which tests if a type has a HashOf(const T&) overload.
 */
template <typename T>
struct HasFunc_HashOf {
  static constexpr bool Value = HasHashOf_internal::HashOfQuery<T>::Value;
};

} // namespace fun
