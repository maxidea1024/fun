#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * String Class::ToString() const;
 */
template <typename T>
struct HasMethod_ToString {
 private:
  class ByteString;

  using String = ByteString;

  template <typename U, String (U::*)() const> struct Check;
  template <typename U> static char MemberTest(Check<U, &U::ToString>*);
  template <typename U> static int MemberTest(...);

 public:
  enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) };
};

} // namespace fun
