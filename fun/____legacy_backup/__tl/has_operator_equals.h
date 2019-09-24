﻿#pragma once

namespace fun {
namespace tl {

namespace HasOperatorEquals_internal {

  struct NotSpecified {};

  template <typename T>
  struct ReturnValueCheck
  {
    static char (&Func())[2];
  };

  template <>
  struct ReturnValueCheck<NotSpecified>
  {
    static char (&Func())[1];
  };

  template <typename T>
  NotSpecified operator == (const T&, const T&);

  template <typename T>
  NotSpecified operator != (const T&, const T&);

  template <typename T>
  const T& Make();

  template <typename T>
  struct Equals
  {
    enum { Value = sizeof(ReturnValueCheck<decltype(Make<T>() == Make<T>())>::Func()) == sizeof(char[2]) };
  };

  template <typename T>
  struct NotEquals
  {
    enum { Value = sizeof(ReturnValueCheck<decltype(Make<T>() != Make<T>())>::Func()) == sizeof(char[2]) };
  };

} // namespace HasOperatorEquals_internal


/**
 * Traits class which tests if a type has an operator== overload.
 */
template <typename T>
struct HasOperatorEquals
{
  enum { Value = HasOperatorEquals_internal::Equals<T>::Value };
};

/**
 * Traits class which tests if a type has an operator!= overload.
 */
template <typename T>
struct HasOperatorNotEquals
{
  enum { Value = HasOperatorEquals_internal::NotEquals<T>::Value };
};

} // namespace tl
} // namespace fun
