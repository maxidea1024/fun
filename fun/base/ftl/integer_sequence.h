#pragma once

#include "fun/base/base.h"

namespace fun {

template <typename T, T... Indices>
struct IntegerSequence {};

#ifdef _MSC_VER

template <typename T, T N>
using MakeIntegerSequence = __make_integer_seq<IntegerSequence, T, N>;

#elif __has_builtin(__make_integer_seq)

template <typename T, T N>
using MakeIntegerSequence = __make_integer_seq<IntegerSequence, T, N>;

#else

namespace IntegerSequence_internal {
  template <typename T, size_t N>
  struct MakeIntegerSequenceImpl;
}

template <typename T, T N>
using MakeIntegerSequence = typename IntegerSequence_internal::MakeIntegerSequenceImpl<T, N>::Type;

namespace IntegerSequence_internal {
  template <size_t N, typename T1, typename T2>
  struct ConcatImpl;

  template <size_t N, typename T, T... Indices1, T... Indices2>
  struct ConcatImpl<N, IntegerSequence<T, Indices1...>, IntegerSequence<T, Indices2...>> : IntegerSequence<T, Indices1..., (T(N + Indices2))...> {
    using Type = IntegerSequence<T, Indices1..., (T(N + Indices2))...>;
  };

  template <size_t N, typename T1, typename T2>
  using Concat = typename ConcatImpl<N, T1, T2>::Type;

  template <typename T, size_t N>
  struct MakeIntegerSequenceImpl : Concat<N / 2, MakeIntegerSequence<T, N / 2>, MakeIntegerSequence<T, N - N / 2>> {
    using Type = Concat<N / 2, MakeIntegerSequence<T, N / 2>, MakeIntegerSequence<T, N - N / 2>>;
  };

  template <typename T>
  struct MakeIntegerSequenceImpl<T, 1> : IntegerSequence<T, T(0)> {
    using Type = IntegerSequence<T, T(0)>;
  };

  template <typename T>
  struct MakeIntegerSequenceImpl<T, 0> : IntegerSequence<T> {
    using Type = IntegerSequence<T>;
  };
};

#endif

} // namespace fun
