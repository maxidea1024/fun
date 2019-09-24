#pragma once

#include "fun/base/base.h"

namespace fun {

// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
#define FUN_DEFINE_HAS_METHOD_DETECTOR(MethodName, ResultType, ConstModifier, \
                                       ...)                                   \
  template <typename T>                                                       \
  struct HasMethod_##MethodName {                                             \
   private:                                                                   \
    template <typename U, ResultType (U::*)(__VA_ARGS__) ConstModifier>       \
    struct Check;                                                             \
    template <typename U>                                                     \
    static char MemberTest(Check<U, &U::MethodName>*);                        \
    template <typename U>                                                     \
    static int MemberTest(...);                                               \
                                                                              \
   public:                                                                    \
    static constexpr bool Value =                                             \
        sizeof(MemberTest<T>(nullptr)) == sizeof(char);                       \
  };

}  // namespace fun
