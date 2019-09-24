#pragma once

namespace fun {
namespace tl {

#define FUN_DEFINE_HAS_METHOD_CHECKER(method_name, result_type, const_modifier, ...) \
  template <typename T> \
  class HasMethod_ ## method_name \
  { \
    template <typename U, result_type (U::*)(__VA_ARGS__) const_modifier> struct Check; \
    template <typename U> static char MemberTest(Check<U, &U::method_name>*); \
    template <typename U> static int MemberTest(...); \
   public: \
    enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) }; \
  };

template <typename T>
class HasMethod_Swap
{
  template <typename U, void (U::*)(T&)> struct Check;
  template <typename U> static char MemberTest(Check<U, &U::Swap>*);
  template <typename U> static int MemberTest(...);

 public:
  enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) };
};

template <typename T>
class HasMethod_ToString
{
  template <typename U, String (U::*)() const> struct Check;
  template <typename U> static char MemberTest(Check<U, &U::ToString>*);
  template <typename U> static int MemberTest(...);

 public:
  enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) };
};

template <typename T>
class HasMethod_GetHash
{
  template <typename U, uint32 (U::*)() const> struct Check;
  template <typename U> static char MemberTest(Check<U, &U::GetHash>*);
  template <typename U> static int MemberTest(...);

 public:
  enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) };
};

} // namespace tl
} // namespace fun
