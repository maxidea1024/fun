#pragma once

#include "fun/base/base.h"
#include "fun/base/crc.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

template <typename Argument, typename Result>
struct UnarayFunction {
  using ArgumentType = Argument;
  using ResultType = Result;
};

template <typename Argument1, typename Argument2, typename Result>
struct BinaryFunction {
  using FirstArgument = Argument1;
  using SecondArgument = Argument2;
  using ResultType = Result;
};

//-----------------------------------------------------------------------------

/**
 * Default predicate class.
 *
 * Assumes < operator is defined for the template type.
 */
template <typename T = void>
struct Less : public BinaryFunction<T, T, bool> {
  FUN_ALWAYS_INLINE bool operator()(const T& a, const T& b) const {
    return a < b;
  }
};

template <>
struct Less<void> {
  template <typename A, typename B>
  FUN_ALWAYS_INLINE decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) < Forward<B>(b);
  }
};

/**
 * Utility predicate class.
 *
 * Assumes < operator is defined for the template type.
 */
template <typename T = void>
struct Greater : public BinaryFunction<T, T, bool> {
  FUN_ALWAYS_INLINE bool operator()(const T& a, const T& b) const {
    return b < a;
  }
};

template <>
struct Greater<void> {
  template <typename A, typename B>
  FUN_ALWAYS_INLINE decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<B>(b) < Forward<A>(a);
  }
};

//-----------------------------------------------------------------------------

template <typename T = void>
struct Plus : public BinaryFunction<T, T, T> {
  constexpr T operator()(const T& a, const T& b) const { return a + b; }
};

template <>
struct Plus<void> {
  template <typename A, typename B>
  constexpr decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) + Forward<B>(b);
  }
};

template <typename T = void>
struct Minus : public BinaryFunction<T, T, T> {
  constexpr T operator()(const T& a, const T& b) const { return a - b; }
};

template <>
struct Minus<void> {
  template <typename A, typename B>
  constexpr decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) - Forward<B>(b);
  }
};

template <typename T = void>
struct Multiplies : public BinaryFunction<T, T, T> {
  constexpr T operator()(const T& a, const T& b) const { return a * b; }
};

template <>
struct Multiplies<void> {
  template <typename A, typename B>
  constexpr decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) * Forward<B>(b);
  }
};

template <typename T = void>
struct Divides : public BinaryFunction<T, T, T> {
  constexpr T operator()(const T& a, const T& b) const { return a / b; }
};

template <>
struct Divides<void> {
  template <typename A, typename B>
  constexpr decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) / Forward<B>(b);
  }
};

template <typename T = void>
struct Modulus : public BinaryFunction<T, T, T> {
  constexpr T operator()(const T& a, const T& b) const { return a % b; }
};

template <>
struct Modulus<void> {
  template <typename A, typename B>
  constexpr decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) % Forward<B>(b);
  }
};

template <typename T = void>
struct Negate : public UnarayFunction<T, T> {
  constexpr T operator()(const T& a) const { return -a; }
};

template <>
struct Negate<void> {
  template <typename A>
  constexpr decltype(auto) operator()(A&& a) const {
    return -Forward<A>(a);
  }
};

template <typename T = void>
struct EqualTo : public BinaryFunction<T, T, bool> {
  constexpr bool operator()(const T& a, const T& b) const { return a == b; }
};

template <>
struct EqualTo<void> {
  template <typename A, typename B>
  constexpr decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) == Forward<B>(b);
  }
};

template <typename T, typename Compare>
bool ValidateEqualTo(const T& a, const T& b, Compare cmp) {
  return cmp(a, b) == cmp(b, a);
}

template <typename T = void>
struct NotEqualTo : public BinaryFunction<T, T, bool> {
  constexpr bool operator()(const T& a, const T& b) const { return a != b; }
};

template <>
struct NotEqualTo<void> {
  template <typename A, typename B>
  constexpr decltype(auto) operator()(A&& a, B&& b) const {
    return Forward<A>(a) != Forward<B>(b);
  }
};

template <typename T, typename Compare>
bool ValidateNotEqualTo(const T& a, const T& b, Compare cmp) {
  return cmp(a, b) != cmp(b, a);
}

template <typename T>
struct StrEqualTo : public BinaryFunction<T, T, bool> {
  constexpr bool operator()(T a, T b) const {
    while (*a && (*a == *b)) {
      ++a;
      ++b;
    }
    return *a == *b;
  }
};

// template <typename T = void>
// struct Greater : public BinaryFunction<T, T, bool> {
//  constexpr bool operator()(const T& a, const T& b) const {
//    return a > b;
//  }
//};
//
// template <>
// struct Greater<void> {
//  template <typename A, typename B>
//  constexpr decltype(auto) operator()(A&& a, B&& b) const {
//    return Forward<A>(a) > Forward<B>(b);
//  }
//};

template <typename T, typename Compare>
bool ValidateGreater(const T& a, const T& b, Compare cmp) {
  return !cmp(a, b) || !cmp(b, a);
}

template <typename T, typename Compare>
bool ValidateLess(const T& a, const T& b, Compare cmp) {
  return !cmp(a, b) || !cmp(b, a);
}

//-----------------------------------------------------------------------------

/**
 * Helper class to reverse a predicate.
 *
 * Performs Predicate(b, a)
 */
template <typename Predicate>
struct ReversePredicate {
  ReversePredicate(const Predicate& pred) : pred_(pred) {}

  template <typename T>
  FUN_ALWAYS_INLINE bool operator()(T&& a, T&& b) const {
    return pred_(Forward<T>(a), Forward<T>(b));
  }

 private:
  const Predicate& pred_;
};

//-----------------------------------------------------------------------------

namespace Invoke_internal {

template <typename Base, typename Callable>
FUN_ALWAYS_INLINE auto DereferenceIfNecessary(Callable&& callable) ->
    typename EnableIf<
        PointerIsConvertibleFromTo<typename Decay<Callable>::Type,
                                   typename Decay<Base>::Type>::Value,
        decltype((Callable &&) callable)>::Type {
  return (Callable &&) callable;
}

template <typename Base, typename Callable>
FUN_ALWAYS_INLINE auto DereferenceIfNecessary(Callable&& callable) ->
    typename EnableIf<
        !PointerIsConvertibleFromTo<typename Decay<Callable>::Type,
                                    typename Decay<Base>::Type>::Value,
        decltype(*(Callable &&) callable)>::Type {
  return *(Callable &&) callable;
}

}  // namespace Invoke_internal

/**
 * Invokes a callable with a set of arguments.  Allows the following:
 *
 * - Calling a functor object given a set of arguments.
 * - Calling a function pointer given a set of arguments.
 * - Calling a member function given a reference to an object and a set of
 * arguments.
 * - Calling a member function given a pointer (including smart pointers) to an
 * object and a set of arguments.
 * - Projecting via a data member pointer given a reference to an object.
 * - Projecting via a data member pointer given a pointer (including smart
 * pointers) to an object.
 *
 * See: http://en.cppreference.com/w/cpp/utility/functional/invoke
 */
template <typename FunctionType, typename... Args>
FUN_ALWAYS_INLINE decltype(auto) Invoke(FunctionType&& func, Args&&... args) {
  return Forward<FunctionType>(func)(Forward<Args>(args)...);
}

template <typename R, typename C, typename Callable>
FUN_ALWAYS_INLINE decltype(auto) Invoke(R C::*pdm, Callable&& callable) {
  return Invoke_internal::DereferenceIfNecessary<C>(
             Forward<Callable>(callable)).*
         pdm;
}

template <typename R, typename C, typename... PMFArgs, typename Callable,
          typename... Args>
FUN_ALWAYS_INLINE decltype(auto) Invoke(R (C::*method)(PMFArgs...),
                                        Callable&& callable, Args&&... args) {
  return (
      Invoke_internal::DereferenceIfNecessary<C>(Forward<Callable>(callable)).*
      method)(Forward<Args>(args)...);
}

template <typename R, typename C, typename... PMFArgs, typename Callable,
          typename... Args>
FUN_ALWAYS_INLINE decltype(auto) Invoke(R (C::*method)(PMFArgs...) const,
                                        Callable&& callable, Args&&... args) {
  return (
      Invoke_internal::DereferenceIfNecessary<C>(Forward<Callable>(callable)).*
      method)(Forward<Args>(args)...);
}

namespace IsInvocable_internal {

template <typename T>
T&& DeclVal();

template <typename T>
struct Void {
  typedef void Type;
};

template <typename, typename Callable, typename... Args>
struct IsInvocable_impl {
  enum { Value = false };
};

template <typename Callable, typename... Args>
struct IsInvocable_impl<typename Void<decltype(Invoke(
                            DeclVal<Callable>(), DeclVal<Args>()...))>::Type,
                        Callable, Args...> {
  enum { Value = true };
};

}  // namespace IsInvocable_internal

/**
 * Traits class which tests if an instance of Callable can be invoked with
 * a list of the arguments of the types provided.
 *
 * Examples:
 *  IsInvocable<void ()>::Value                                 == true
 *  IsInvocable<void (), String>::Value                         == false
 *  IsInvocable<void (String), String>::Value                   == true
 *  IsInvocable<void (String), const char*>::Value              == true
 *  IsInvocable<void (String), int32>::Value                    == false
 *  IsInvocable<void (char, float, bool), int, int, int>::Value == true
 *  IsInvocable<Function<void (String)>, String>::Value         == true
 *  IsInvocable<Function<void (String)>, int32>::Value          == false
 */
template <typename Callable, typename... Args>
struct IsInvocable
    : IsInvocable_internal::IsInvocable_impl<void, Callable, Args...> {};

//-----------------------------------------------------------------------------

// TODO crc32 함수를 참조해야하므로 별도로 빼주는게 좋을까??

/**
 * Combines two hash values to get a third.
 * Note - this function is not commutative.
 */
FUN_ALWAYS_INLINE uint32 HashCombine(uint32 a, uint32 c) {
  uint32 b = 0x9e3779b9;
  a += b;
  a -= b;
  a -= c;
  a ^= (c >> 13);
  b -= c;
  b -= a;
  b ^= (a << 8);
  c -= a;
  c -= b;
  c ^= (b >> 13);
  a -= b;
  a -= c;
  a ^= (c >> 12);
  b -= c;
  b -= a;
  b ^= (a << 16);
  c -= a;
  c -= b;
  c ^= (b >> 5);
  a -= b;
  a -= c;
  a ^= (c >> 3);
  b -= c;
  b -= a;
  b ^= (a << 10);
  c -= a;
  c -= b;
  c ^= (b >> 15);
  return c;
}

FUN_ALWAYS_INLINE uint32 PointerHash(const void* key, uint32 c = 0) {
#if FUN_PTR_IS_64_BIT
  // 상위 4비트는 거의 항상 0이므로 좀더 의미 있는 값들을 취하기 위해 제거.
  auto p = reinterpret_cast<uintptr_t>(key) >> 4;
#else
  auto p = reinterpret_cast<uintptr_t>(key);
#endif

  return HashCombine(static_cast<uint32>(p), c);
}

//
// Hash functions for common types.
//

FUN_ALWAYS_INLINE uint32 HashOf(const uint8 x) { return x; }

FUN_ALWAYS_INLINE uint32 HashOf(const int8 x) { return x; }

FUN_ALWAYS_INLINE uint32 HashOf(const uint16 x) { return x; }

FUN_ALWAYS_INLINE uint32 HashOf(const int16 x) { return x; }

FUN_ALWAYS_INLINE uint32 HashOf(const int32 x) { return x; }

FUN_ALWAYS_INLINE uint32 HashOf(const uint32 x) { return x; }

FUN_ALWAYS_INLINE uint32 HashOf(const uint64 x) {
  return (uint32)x + ((uint32)(x >> 32) * 23);
}

FUN_ALWAYS_INLINE uint32 HashOf(const int64 x) {
  return (uint32)x + ((uint32)(x >> 32) * 23);
}

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
FUN_ALWAYS_INLINE uint32 HashOf(const __uint128_t x) {
  const uint64 lo = (uint64)x;
  const uint64 hi = (uint64)(x >> 64);
  return HashOf(lo) ^ HashOf(hi);
}
#endif

FUN_ALWAYS_INLINE uint32 HashOf(float x) {
  union {
    float f;
    uint32 i;
  };
  f = x;
  return HashOf(i);
}

FUN_ALWAYS_INLINE uint32 HashOf(double x) {
  union {
    double d;
    uint64 i;
  };
  d = x;
  return HashOf(i);
}

FUN_ALWAYS_INLINE uint32 HashOf(const char* x) { return Crc::StringCrc32(x); }

FUN_ALWAYS_INLINE uint32 HashOf(const UNICHAR* x) {
  return Crc::StringCrc32(x);
}

FUN_ALWAYS_INLINE uint32 HashOf(const void* x) { return PointerHash(x); }

FUN_ALWAYS_INLINE uint32 HashOf(void* x) { return PointerHash(x); }

template <typename Enum>
FUN_ALWAYS_INLINE typename EnableIf<IsEnum<Enum>::Value, uint32>::Type HashOf(
    Enum e) {
  return HashOf((__underlying_type(Enum))e);
}

// template <typename WrappedEnum> FUN_ALWAYS_INLINE
// typename EnableIf<IsWrappedEnum<WrappedEnum>::Value, uint32>::Type
// HashOf(WrappedEnum e)
//{
//  return HashOf(e.value);
//}

}  // namespace fun
