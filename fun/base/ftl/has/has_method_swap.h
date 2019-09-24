#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * void Class::Swap(Class& other);
 */
template <typename T>
struct HasMethod_Swap {
 private:
  template <typename U, void (U::*)(T&)> struct Check;
  template <typename U> static char MemberTest(Check<U, &U::Swap>*);
  template <typename U> static int  MemberTest(...);

 public:
  static constexpr bool Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char);
};

} // namespace fun
