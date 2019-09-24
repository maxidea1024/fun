#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/ftl/tuple_fwd.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

// TupleTypes helper
template <typename... Ts>
struct TupleTypes {};

// TupleSize helper
template <typename T>
struct TupleSize {};
template <typename T>
struct TupleSize<const T> : public TupleSize<T> {};
template <typename T>
struct TupleSize<volatile T> : public TupleSize<T> {};
template <typename T>
struct TupleSize<const volatile T> : public TupleSize<T> {};

template <typename... Ts>
struct TupleSize<TupleTypes<Ts...>>
    : public IntegralConstant<size_t, sizeof...(Ts)> {};
template <typename... Ts>
struct TupleSize<Tuple<Ts...>>
    : public IntegralConstant<size_t, sizeof...(Ts)> {};

template <typename T>
constexpr size_t TupleSize_V = TupleSize<T>::Value;

namespace internal {

template <typename TupleIndices, typename... Ts>
struct TupleImpl;

}  // namespace internal

template <typename Indices, typename... Ts>
class TupleSize<internal::TupleImpl<Indices, Ts...>>
    : public IntegralConstant<size_t, sizeof...(Ts)> {};

// TupleElement helper to be able to isolate a type given an index
template <size_t I, typename T>
class TupleElement {};

template <size_t I>
class TupleElement<I, TupleTypes<>> {
 public:
  static_assert(I != I, "TupleElement index out of range");
};

template <typename H, typename... Ts>
class TupleElement<0, TupleTypes<H, Ts...>> {
 public:
  typedef H Type;
};

template <size_t I, typename H, typename... Ts>
class TupleElement<I, TupleTypes<H, Ts...>> {
 public:
  typedef TupleElement_T<I - 1, TupleTypes<Ts...>> Type;
};

// specialization for Tuple
template <size_t I, typename... Ts>
class TupleElement<I, Tuple<Ts...>> {
 public:
  typedef TupleElement_T<I, TupleTypes<Ts...>> Type;
};

template <size_t I, typename... Ts>
class TupleElement<I, const Tuple<Ts...>> {
 public:
  typedef typename AddConst<TupleElement_T<I, TupleTypes<Ts...>>>::Type Type;
};

template <size_t I, typename... Ts>
class TupleElement<I, volatile Tuple<Ts...>> {
 public:
  typedef typename AddVolatile<TupleElement_T<I, TupleTypes<Ts...>>>::Type Type;
};

template <size_t I, typename... Ts>
class TupleElement<I, const volatile Tuple<Ts...>> {
 public:
  typedef typename AddCV<TupleElement_T<I, TupleTypes<Ts...>>>::Type Type;
};

// specialization for TupleImpl
template <size_t I, typename Indices, typename... Ts>
class TupleElement<I, internal::TupleImpl<Indices, Ts...>>
    : public TupleElement<I, Tuple<Ts...>> {};

template <size_t I, typename Indices, typename... Ts>
class TupleElement<I, const internal::TupleImpl<Indices, Ts...>>
    : public TupleElement<I, const Tuple<Ts...>> {};

template <size_t I, typename Indices, typename... Ts>
class TupleElement<I, volatile internal::TupleImpl<Indices, Ts...>>
    : public TupleElement<I, volatile Tuple<Ts...>> {};

template <size_t I, typename Indices, typename... Ts>
class TupleElement<I, const volatile internal::TupleImpl<Indices, Ts...>>
    : public TupleElement<I, const volatile Tuple<Ts...>> {};

// attempt to isolate index given a type
template <typename T, typename Tuple>
struct TupleIndex {};

template <typename T>
struct TupleIndex<T, TupleTypes<>> {
  typedef void DuplicateTypeCheck;
  TupleIndex() = delete;  // TupleIndex should only be used for compile-time
                          // assistance, and never be instantiated
  static const size_t Index = 0;
};

template <typename T, typename... TsRest>
struct TupleIndex<T, TupleTypes<T, TsRest...>> {
  typedef int DuplicateTypeCheck;
  // after finding type T in the list of types, try to find type T in TsRest.
  // If we stumble back into this version of TupleIndex, i.e. type T appears
  // twice in the list of types, then DuplicateTypeCheck will be of type int,
  // and the static_assert will fail. If we don't, then we'll go through the
  // version of TupleIndex above, where all of the types have been exhausted,
  // and DuplicateTypeCheck will be void.
  static_assert(IsVoid<typename TupleIndex<
                    T, TupleTypes<TsRest...>>::DuplicateTypeCheck>::Value,
                "duplicate type T in tuple_vector::Get<T>(); unique types must "
                "be provided in declaration, or only use Get<size_t>()");

  static const size_t Index = 0;
};

template <typename T, typename TsHead, typename... TsRest>
struct TupleIndex<T, TupleTypes<TsHead, TsRest...>> {
  typedef typename TupleIndex<T, TupleTypes<TsRest...>>::DuplicateTypeCheck
      DuplicateTypeCheck;
  static const size_t Index = TupleIndex<T, TupleTypes<TsRest...>>::Index + 1;
};

template <typename T, typename Indices, typename... Ts>
struct TupleIndex<T, internal::TupleImpl<Indices, Ts...>>
    : public TupleIndex<T, TupleTypes<Ts...>> {};

namespace internal {

// TupleLeaf
template <size_t I, typename ValueType,
          bool IsEmpty = fun::IsEmpty<ValueType>::Value>
class TupleLeaf;

template <size_t I, typename ValueType, bool IsEmpty>
FUN_ALWAYS_INLINE void Swap(TupleLeaf<I, ValueType, IsEmpty>& a,
                            TupleLeaf<I, ValueType, IsEmpty>& b) {
  fun::Swap(a.GetInternal(), b.GetInternal());
}

template <size_t I, typename ValueType, bool IsEmpty>
class TupleLeaf {
 public:
  TupleLeaf() : value_() {}
  TupleLeaf(const TupleLeaf&) = default;
  TupleLeaf& operator=(const TupleLeaf&) = delete;

  // We shouldn't need this explicit constructor as it should be handled by the
  // template below but OSX clang IsConstructible type trait incorrectly gives
  // false for IsConstructible<T&&, T&&>::Value
  explicit TupleLeaf(ValueType&& v) : value_(MoveTemp(v)) {}

  template <typename T, typename = typename EnableIf<
                            IsConstructible<ValueType, T&&>::Value>::Type>
  explicit TupleLeaf(T&& t) : value_(Forward<T>(t)) {}

  template <typename T>
  explicit TupleLeaf(const TupleLeaf<I, T>& t) : value_(t.GetInternal()) {}

  template <typename T>
  TupleLeaf& operator=(T&& t) {
    value_ = Forward<T>(t);
    return *this;
  }

  int Swap(TupleLeaf& t) {
    fun::internal::Swap(*this, t);
    return 0;
  }

  ValueType& GetInternal() { return value_; }
  const ValueType& GetInternal() const { return value_; }

 private:
  ValueType value_;
};

// Specialize for when ValueType is a reference
template <size_t I, typename ValueType, bool IsEmpty>
class TupleLeaf<I, ValueType&, IsEmpty> {
 public:
  TupleLeaf(const TupleLeaf&) = default;
  TupleLeaf& operator=(const TupleLeaf&) = delete;

  template <typename T, typename = typename EnableIf<
                            IsConstructible<ValueType, T&&>::Value>::Type>
  explicit TupleLeaf(T&& t) : value_(Forward<T>(t)) {}

  explicit TupleLeaf(ValueType& t) : value_(t) {}

  template <typename T>
  explicit TupleLeaf(const TupleLeaf<I, T>& t) : value_(t.GetInternal()) {}

  template <typename T>
  TupleLeaf& operator=(T&& t) {
    value_ = Forward<T>(t);
    return *this;
  }

  int Swap(TupleLeaf& t) {
    fun::internal::Swap(*this, t);
    return 0;
  }

  ValueType& GetInternal() { return value_; }
  const ValueType& GetInternal() const { return value_; }

 private:
  ValueType& value_;
};

// TupleLeaf partial specialization for when we can use the Empty Base Class
// Optimization
template <size_t I, typename ValueType>
class TupleLeaf<I, ValueType, true> : private ValueType {
 public:
  // TrueType / FalseType constructors for case where ValueType is default
  // constructible and should be value initialized and case where it is not
  TupleLeaf(const TupleLeaf&) = default;

  template <typename T, typename = typename EnableIf<
                            IsConstructible<ValueType, T&&>::Value>::Type>
  explicit TupleLeaf(T&& t) : ValueType(Forward<T>(t)) {}

  template <typename T>
  explicit TupleLeaf(const TupleLeaf<I, T>& t) : ValueType(t.GetInternal()) {}

  template <typename T>
  TupleLeaf& operator=(T&& t) {
    ValueType::operator=(Forward<T>(t));
    return *this;
  }

  int Swap(TupleLeaf& t) {
    fun::internal::Swap(*this, t);
    return 0;
  }

  ValueType& GetInternal() { return static_cast<ValueType&>(*this); }
  const ValueType& GetInternal() const {
    return static_cast<const ValueType&>(*this);
  }

 private:
  TupleLeaf& operator=(const TupleLeaf&) = delete;
};

// MakeTupleTypes

template <typename TupleTypes, typename Tuple, size_t Start, size_t End>
struct MakeTupleTypesImpl;

template <typename... Types, typename Tuple, size_t Start, size_t End>
struct MakeTupleTypesImpl<TupleTypes<Types...>, Tuple, Start, End> {
  typedef typename RemoveReference<Tuple>::Type TupleType;
  typedef typename MakeTupleTypesImpl<
      TupleTypes<Types...,
                 typename conditional<is_lvalue_reference<Tuple>::Value,
                                      // append ref if Tuple is ref
                                      TupleElement_T<Start, TupleType>&,
                                      // append non-ref otherwise
                                      TupleElement_T<Start, TupleType>>::Type>,
      Tuple, Start + 1, End>::Type Type;
};

template <typename... Types, typename Tuple, size_t End>
struct MakeTupleTypesImpl<TupleTypes<Types...>, Tuple, End, End> {
  typedef TupleTypes<Types...> Type;
};

template <typename Tuple>
using MakeTupleTypes_T = typename MakeTupleTypesImpl<
    TupleTypes<>, Tuple, 0,
    TupleSize<typename RemoveReference<Tuple>::Type>::Value>::Type;

// TupleImpl

template <typename... Ts>
void Swallow(Ts&&...) {}

template <size_t I, typename Indices, typename... Ts>
TupleElement_T<I, TupleImpl<Indices, Ts...>>& Get(TupleImpl<Indices, Ts...>& t);

template <size_t I, typename Indices, typename... Ts>
ConstTupleElement_T<I, TupleImpl<Indices, Ts...>>& Get(
    const TupleImpl<Indices, Ts...>& t);

template <size_t I, typename Indices, typename... Ts>
TupleElement_T<I, TupleImpl<Indices, Ts...>>&& Get(
    TupleImpl<Indices, Ts...>&& t);

template <typename T, typename Indices, typename... Ts>
T& Get(TupleImpl<Indices, Ts...>& t);

template <typename T, typename Indices, typename... Ts>
const T& Get(const TupleImpl<Indices, Ts...>& t);

template <typename T, typename Indices, typename... Ts>
T&& Get(TupleImpl<Indices, Ts...>&& t);

template <size_t... Indices, typename... Ts>
struct TupleImpl<IntegerSequence<size_t, Indices...>, Ts...>
    : public TupleLeaf<Indices, Ts>... {
  constexpr TupleImpl() = default;

  // IndexSequence changed to IntegerSequence due to issues described below in
  // VS2015 CTP 6.
  // https://connect.microsoft.com/VisualStudio/feedback/details/1126958/error-in-template-parameter-pack-expansion-of-std-index-sequence
  //
  template <typename... Us, typename... ValueTypes>
  explicit TupleImpl(IntegerSequence<size_t, Indices...>, TupleTypes<Us...>,
                     ValueTypes&&... values)
      : TupleLeaf<Indices, Ts>(Forward<ValueTypes>(values))... {}

  template <typename OtherTuple>
  TupleImpl(OtherTuple&& t)
      : TupleLeaf<Indices, Ts>(
            Forward<TupleElement_T<Indices, MakeTupleTypes_T<OtherTuple>>>(
                Get<Indices>(t)))... {}

  template <typename OtherTuple>
  TupleImpl& operator=(OtherTuple&& t) {
    Swallow(TupleLeaf<Indices, Ts>::operator=(
        Forward<TupleElement_T<Indices, MakeTupleTypes_T<OtherTuple>>>(
            Get<Indices>(t)))...);
    return *this;
  }

  TupleImpl& operator=(const TupleImpl& t) {
    Swallow(TupleLeaf<Indices, Ts>::operator=(
        static_cast<const TupleLeaf<Indices, Ts>&>(t).GetInternal())...);
    return *this;
  }

  void Swap(TupleImpl& t) {
    Swallow(TupleLeaf<Indices, Ts>::Swap(
        static_cast<TupleLeaf<Indices, Ts>&>(t))...);
  }
};

template <size_t I, typename Indices, typename... Ts>
FUN_ALWAYS_INLINE TupleElement_T<I, TupleImpl<Indices, Ts...>>& Get(
    TupleImpl<Indices, Ts...>& t) {
  typedef TupleElement_T<I, TupleImpl<Indices, Ts...>> Type;
  return static_cast<internal::TupleLeaf<I, Type>&>(t).GetInternal();
}

template <size_t I, typename Indices, typename... Ts>
FUN_ALWAYS_INLINE ConstTupleElement_T<I, TupleImpl<Indices, Ts...>>& Get(
    const TupleImpl<Indices, Ts...>& t) {
  typedef TupleElement_T<I, TupleImpl<Indices, Ts...>> Type;
  return static_cast<const internal::TupleLeaf<I, Type>&>(t).GetInternal();
}

template <size_t I, typename Indices, typename... Ts>
FUN_ALWAYS_INLINE TupleElement_T<I, TupleImpl<Indices, Ts...>>&& Get(
    TupleImpl<Indices, Ts...>&& t) {
  typedef TupleElement_T<I, TupleImpl<Indices, Ts...>> Type;
  return static_cast<Type&&>(
      static_cast<internal::TupleLeaf<I, Type>&>(t).GetInternal());
}

template <typename T, typename Indices, typename... Ts>
FUN_ALWAYS_INLINE T& Get(TupleImpl<Indices, Ts...>& t) {
  typedef TupleIndex<T, TupleImpl<Indices, Ts...>> Index;
  return static_cast<internal::TupleLeaf<Index::Index, T>&>(t).GetInternal();
}

template <typename T, typename Indices, typename... Ts>
FUN_ALWAYS_INLINE const T& Get(const TupleImpl<Indices, Ts...>& t) {
  typedef TupleIndex<T, TupleImpl<Indices, Ts...>> Index;
  return static_cast<const internal::TupleLeaf<Index::Index, T>&>(t)
      .GetInternal();
}

template <typename T, typename Indices, typename... Ts>
FUN_ALWAYS_INLINE T&& Get(TupleImpl<Indices, Ts...>&& t) {
  typedef TupleIndex<T, TupleImpl<Indices, Ts...>> Index;
  return static_cast<T&&>(
      static_cast<internal::TupleLeaf<Index::Index, T>&>(t).GetInternal());
}

// TupleLike

template <typename T>
struct TupleLike : public FalseType {};

template <typename T>
struct TupleLike<const T> : public TupleLike<T> {};

template <typename T>
struct TupleLike<volatile T> : public TupleLike<T> {};

template <typename T>
struct TupleLike<const volatile T> : public TupleLike<T> {};

template <typename... Ts>
struct TupleLike<Tuple<Ts...>> : public TrueType {};

template <typename First, typename Second>
struct TupleLike<fun::Pair<First, Second>> : public TrueType {};

// TupleConvertible

template <bool IsSameSize, typename From, typename To>
struct TupleConvertibleImpl : public FalseType {};

template <typename... FromTypes, typename... ToTypes>
struct TupleConvertibleImpl<true, TupleTypes<FromTypes...>,
                            TupleTypes<ToTypes...>>
    : public IntegralConstant<
          bool, conjunction<IsConvertible<FromTypes, ToTypes>...>::Value> {};

template <typename From, typename To,
          bool = TupleLike<typename RemoveReference<From>::Type>::Value,
          bool = TupleLike<typename RemoveReference<To>::Type>::Value>
struct TupleConvertible : public FalseType {};

template <typename From, typename To>
struct TupleConvertible<From, To, true, true>
    : public TupleConvertibleImpl<
          TupleSize<typename RemoveReference<From>::Type>::Value ==
              TupleSize<typename RemoveReference<To>::Type>::Value,
          MakeTupleTypes_T<From>, MakeTupleTypes_T<To>> {};

// TupleAssignable

template <bool IsSameSize, typename Target, typename From>
struct TupleAssignableImpl : public FalseType {};

template <typename... TargetTypes, typename... FromTypes>
struct TupleAssignableImpl<true, TupleTypes<TargetTypes...>,
                           TupleTypes<FromTypes...>>
    : public BoolConstant<
          conjunction<IsAssignable<TargetTypes, FromTypes>...>::Value> {};

template <typename Target, typename From,
          bool = TupleLike<typename RemoveReference<Target>::Type>::Value,
          bool = TupleLike<typename RemoveReference<From>::Type>::Value>
struct TupleAssignable : public FalseType {};

template <typename Target, typename From>
struct TupleAssignable<Target, From, true, true>
    : public TupleAssignableImpl<
          TupleSize<typename RemoveReference<Target>::Type>::Value ==
              TupleSize<typename RemoveReference<From>::Type>::Value,
          MakeTupleTypes_T<Target>, MakeTupleTypes_T<From>> {};

// TupleImplicitlyConvertible and TupleExplicitlyConvertible - helpers for
// constraining conditionally-explicit ctors

template <bool IsSameSize, typename TargetType, typename... FromTypes>
struct TupleImplicitlyConvertibleImpl : public FalseType {};

template <typename... TargetTypes, typename... FromTypes>
struct TupleImplicitlyConvertibleImpl<true, TupleTypes<TargetTypes...>,
                                      FromTypes...>
    : public conjunction<IsConstructible<TargetTypes, FromTypes>...,
                         IsConvertible<FromTypes, TargetTypes>...> {};

template <typename TargetTupleType, typename... FromTypes>
struct TupleImplicitlyConvertible
    : public TupleImplicitlyConvertibleImpl<
          TupleSize<TargetTupleType>::Value == sizeof...(FromTypes),
          MakeTupleTypes_T<TargetTupleType>, FromTypes...>::Type {};

template <typename TargetTupleType, typename... FromTypes>
using TupleImplicitlyConvertible_T =
    EnableIf_T<TupleImplicitlyConvertible<TargetTupleType, FromTypes...>::Value,
               bool>;

template <bool IsSameSize, typename TargetType, typename... FromTypes>
struct TupleExplicitlyConvertibleImpl : public FalseType {};

template <typename... TargetTypes, typename... FromTypes>
struct TupleExplicitlyConvertibleImpl<true, TupleTypes<TargetTypes...>,
                                      FromTypes...>
    : public conjunction<
          IsConstructible<TargetTypes, FromTypes>...,
          negation<conjunction<IsConvertible<FromTypes, TargetTypes>...>>> {};

template <typename TargetTupleType, typename... FromTypes>
struct TupleExplicitlyConvertible
    : public TupleExplicitlyConvertibleImpl<
          TupleSize<TargetTupleType>::Value == sizeof...(FromTypes),
          MakeTupleTypes_T<TargetTupleType>, FromTypes...>::Type {};

template <typename TargetTupleType, typename... FromTypes>
using TupleExplicitlyConvertible_T =
    EnableIf_T<TupleExplicitlyConvertible<TargetTupleType, FromTypes...>::Value,
               bool>;

// TupleEqual

template <size_t I>
struct TupleEqual {
  template <typename Tuple1, typename Tuple2>
  bool operator()(const Tuple1& t1, const Tuple2& t2) {
    static_assert(TupleSize<Tuple1>::Value == TupleSize<Tuple2>::Value,
                  "comparing tuples of different sizes.");
    return TupleEqual<I - 1>()(t1, t2) && Get<I - 1>(t1) == Get<I - 1>(t2);
  }
};

template <>
struct TupleEqual<0> {
  template <typename Tuple1, typename Tuple2>
  bool operator()(const Tuple1&, const Tuple2&) {
    return true;
  }
};

// TupleLess

template <size_t I>
struct TupleLess {
  template <typename Tuple1, typename Tuple2>
  bool operator()(const Tuple1& t1, const Tuple2& t2) {
    static_assert(TupleSize<Tuple1>::Value == TupleSize<Tuple2>::Value,
                  "comparing tuples of different sizes.");
    return TupleLess<I - 1>()(t1, t2) ||
           (!TupleLess<I - 1>()(t2, t1) && Get<I - 1>(t1) < Get<I - 1>(t2));
  }
};

template <>
struct TupleLess<0> {
  template <typename Tuple1, typename Tuple2>
  bool operator()(const Tuple1&, const Tuple2&) {
    return false;
  }
};

template <typename T>
struct MakeTupleReturnImpl {
  typedef T Type;
};

template <typename T>
struct MakeTupleReturnImpl<reference_wrapper<T>> {
  typedef T& Type;
};

template <typename T>
using MakeTupleReturn_T =
    typename MakeTupleReturnImpl<typename Decay<T>::Type>::Type;

struct ignore_t {
  ignore_t() {}

  template <typename T>
  const ignore_t& operator=(const T&) const {
    return *this;
  }
};

// TupleCat helpers
template <typename Tuple1, typename Is1, typename Tuple2, typename Is2>
struct TupleCat2Impl;

template <typename... T1s, size_t... I1s, typename... T2s, size_t... I2s>
struct TupleCat2Impl<Tuple<T1s...>, IndexSequence<I1s...>, Tuple<T2s...>,
                     IndexSequence<I2s...>> {
  typedef Tuple<T1s..., T2s...> ResultType;

  template <typename Tuple1, typename Tuple2>
  static FUN_ALWAYS_INLINE ResultType DoCat2(Tuple1&& t1, Tuple2&& t2) {
    return ResultType(Get<I1s>(Forward<Tuple1>(t1))...,
                      Get<I2s>(Forward<Tuple2>(t2))...);
  }
};

template <typename Tuple1, typename Tuple2>
struct TupleCat2;

template <typename... T1s, typename... T2s>
struct TupleCat2<Tuple<T1s...>, Tuple<T2s...>> {
  typedef MakeIndexSequence<sizeof...(T1s)> Is1;
  typedef MakeIndexSequence<sizeof...(T2s)> Is2;
  typedef TupleCat2Impl<Tuple<T1s...>, Is1, Tuple<T2s...>, Is2> TCI;
  typedef typename TCI::ResultType ResultType;

  template <typename Tuple1, typename Tuple2>
  static FUN_ALWAYS_INLINE ResultType DoCat2(Tuple1&& t1, Tuple2&& t2) {
    return TCI::DoCat2(Forward<Tuple1>(t1), Forward<Tuple2>(t2));
  }
};

template <typename... Tuples>
struct TupleCat;

template <typename Tuple1, typename Tuple2, typename... TuplesRest>
struct TupleCat<Tuple1, Tuple2, TuplesRest...> {
  typedef typename TupleCat2<Tuple1, Tuple2>::ResultType FirstResultType;
  typedef
      typename TupleCat<FirstResultType, TuplesRest...>::ResultType ResultType;

  template <typename TupleArg1, typename TupleArg2, typename... TupleArgsRest>
  static FUN_ALWAYS_INLINE ResultType DoCat(TupleArg1&& t1, TupleArg2&& t2,
                                            TupleArgsRest&&... ts) {
    return TupleCat<FirstResultType, TuplesRest...>::DoCat(
        TupleCat2<TupleArg1, TupleArg2>::DoCat2(Forward<TupleArg1>(t1),
                                                Forward<TupleArg2>(t2)),
        Forward<TupleArgsRest>(ts)...);
  }
};

template <typename Tuple1, typename Tuple2>
struct TupleCat<Tuple1, Tuple2> {
  typedef typename TupleCat2<Tuple1, Tuple2>::ResultType ResultType;

  template <typename TupleArg1, typename TupleArg2>
  static FUN_ALWAYS_INLINE ResultType DoCat(TupleArg1&& t1, TupleArg2&& t2) {
    return TupleCat2<TupleArg1, TupleArg2>::DoCat2(Forward<TupleArg1>(t1),
                                                   Forward<TupleArg2>(t2));
  }
};

}  // namespace internal

template <typename... Ts>
class Tuple;

template <typename T, typename... Ts>
class Tuple<T, Ts...> {
 public:
  constexpr Tuple() = default;

  template <typename T2 = T, internal::TupleImplicitlyConvertible_T<
                                 Tuple, const T2&, const Ts&...> = 0>
  constexpr Tuple(const T& t, const Ts&... ts)
      : impl_(MakeIndexSequence<sizeof...(Ts) + 1>{},
              internal::MakeTupleTypes_T<Tuple>{}, t, ts...) {}

  template <typename T2 = T, internal::TupleExplicitlyConvertible_T<
                                 Tuple, const T2&, const Ts&...> = 0>
  explicit constexpr Tuple(const T& t, const Ts&... ts)
      : impl_(MakeIndexSequence<sizeof...(Ts) + 1>{},
              internal::MakeTupleTypes_T<Tuple>{}, t, ts...) {}

  template <typename U, typename... Us,
            internal::TupleImplicitlyConvertible_T<Tuple, U, Us...> = 0>
  constexpr Tuple(U&& u, Us&&... us)
      : impl_(MakeIndexSequence<sizeof...(Us) + 1>{},
              internal::MakeTupleTypes_T<Tuple>{}, Forward<U>(u),
              Forward<Us>(us)...) {}

  template <typename U, typename... Us,
            internal::TupleExplicitlyConvertible_T<Tuple, U, Us...> = 0>
  explicit constexpr Tuple(U&& u, Us&&... us)
      : impl_(MakeIndexSequence<sizeof...(Us) + 1>{},
              internal::MakeTupleTypes_T<Tuple>{}, Forward<U>(u),
              Forward<Us>(us)...) {}

  template <
      typename OtherTuple,
      typename EnableIf<internal::TupleConvertible<OtherTuple, Tuple>::Value,
                        bool>::Type = false>
  Tuple(OtherTuple&& t) : impl_(Forward<OtherTuple>(t)) {}

  template <
      typename OtherTuple,
      typename EnableIf<internal::TupleAssignable<Tuple, OtherTuple>::Value,
                        bool>::Type = false>
  Tuple& operator=(OtherTuple&& t) {
    impl_.operator=(Forward<OtherTuple>(t));
    return *this;
  }

  void Swap(Tuple& t) { impl_.Swap(t.impl_); }

 private:
  typedef internal::TupleImpl<MakeIndexSequence<sizeof...(Ts) + 1>, T, Ts...>
      Impl;
  Impl impl_;

  template <size_t I, typename... Ts_>
  friend TupleElement_T<I, Tuple<Ts_...>>& Get(Tuple<Ts_...>& t);

  template <size_t I, typename... Ts_>
  friend ConstTupleElement_T<I, Tuple<Ts_...>>& Get(const Tuple<Ts_...>& t);

  template <size_t I, typename... Ts_>
  friend TupleElement_T<I, Tuple<Ts_...>>&& Get(Tuple<Ts_...>&& t);

  template <typename T_, typename... ts_>
  friend T_& Get(Tuple<ts_...>& t);

  template <typename T_, typename... ts_>
  friend const T_& Get(const Tuple<ts_...>& t);

  template <typename T_, typename... ts_>
  friend T_&& Get(Tuple<ts_...>&& t);
};

template <>
class Tuple<> {
 public:
  void Swap(Tuple&) {}
};

template <size_t I, typename... Ts>
FUN_ALWAYS_INLINE TupleElement_T<I, Tuple<Ts...>>& Get(Tuple<Ts...>& t) {
  return Get<I>(t.impl_);
}

template <size_t I, typename... Ts>
FUN_ALWAYS_INLINE ConstTupleElement_T<I, Tuple<Ts...>>& Get(
    const Tuple<Ts...>& t) {
  return Get<I>(t.impl_);
}

template <size_t I, typename... Ts>
FUN_ALWAYS_INLINE TupleElement_T<I, Tuple<Ts...>>&& Get(Tuple<Ts...>&& t) {
  return Get<I>(MoveTemp(t.impl_));
}

template <typename T, typename... Ts>
FUN_ALWAYS_INLINE T& Get(Tuple<Ts...>& t) {
  return Get<T>(t.impl_);
}

template <typename T, typename... Ts>
FUN_ALWAYS_INLINE const T& Get(const Tuple<Ts...>& t) {
  return Get<T>(t.impl_);
}

template <typename T, typename... Ts>
FUN_ALWAYS_INLINE T&& Get(Tuple<Ts...>&& t) {
  return Get<T>(MoveTemp(t.impl_));
}

template <typename... Ts>
FUN_ALWAYS_INLINE void Swap(Tuple<Ts...>& a, Tuple<Ts...>& b) {
  a.Swap(b);
}

template <typename... T1s, typename... T2s>
FUN_ALWAYS_INLINE bool operator==(const Tuple<T1s...>& t1,
                                  const Tuple<T2s...>& t2) {
  return internal::TupleEqual<sizeof...(T1s)>()(t1, t2);
}

template <typename... T1s, typename... T2s>
FUN_ALWAYS_INLINE bool operator!=(const Tuple<T1s...>& t1,
                                  const Tuple<T2s...>& t2) {
  return !(t1 == t2);
}

template <typename... T1s, typename... T2s>
FUN_ALWAYS_INLINE bool operator<(const Tuple<T1s...>& t1,
                                 const Tuple<T2s...>& t2) {
  return internal::TupleLess<sizeof...(T1s)>()(t1, t2);
}

template <typename... T1s, typename... T2s>
FUN_ALWAYS_INLINE bool operator>(const Tuple<T1s...>& t1,
                                 const Tuple<T2s...>& t2) {
  return t2 < t1;
}

template <typename... T1s, typename... T2s>
FUN_ALWAYS_INLINE bool operator<=(const Tuple<T1s...>& t1,
                                  const Tuple<T2s...>& t2) {
  return !(t2 < t1);
}

template <typename... T1s, typename... T2s>
FUN_ALWAYS_INLINE bool operator>=(const Tuple<T1s...>& t1,
                                  const Tuple<T2s...>& t2) {
  return !(t1 < t2);
}

// Tuple helper functions

template <typename... Ts>
FUN_ALWAYS_INLINE constexpr Tuple<internal::MakeTupleReturn_T<Ts>...> MakeTuple(
    Ts&&... values) {
  return Tuple<internal::MakeTupleReturn_T<Ts>...>(Forward<Ts>(values)...);
}

template <typename... Ts>
FUN_ALWAYS_INLINE constexpr Tuple<Ts&&...> ForwardAsTuple(Ts&&... ts) noexcept {
  return Tuple<Ts&&...>(Forward<Ts&&>(ts)...);
}

// Specialize ignore_t IsAssignable type trait due to yet another VS2013 type
// traits bug
template <typename U>
struct IsAssignable<const internal::ignore_t&, U> : public TrueType {};

static const internal::ignore_t ignore;

template <typename... Ts>
FUN_ALWAYS_INLINE constexpr Tuple<Ts&...> Tie(Ts&... ts) noexcept {
  return Tuple<Ts&...>(ts...);
}

template <typename... Tuples>
FUN_ALWAYS_INLINE typename internal::TupleCat<Tuples...>::ResultType TupleCat(
    Tuples&&... ts) {
  return internal::TupleCat<Tuples...>::DoCat(Forward<Tuples>(ts)...);
}

// Apply
//
// Invoke a callable object using a Tuple to supply the arguments.
//
// http://en.cppreference.com/w/cpp/utility/apply
//
namespace detail {

template <typename F, typename Tuple, size_t... I>
constexpr decltype(auto) ApplyImpl(F&& f, Tuple&& t, IndexSequence<I...>) {
  return Invoke(Forward<F>(f), Get<I>(Forward<Tuple>(t))...);
}

}  // namespace detail

template <typename F, typename Tuple>
constexpr decltype(auto) Apply(F&& f, Tuple&& t) {
  return detail::ApplyImpl(
      Forward<F>(f), Forward<Tuple>(t),
      MakeIndexSequence<TupleSize_V<RemoveReference_T<Tuple>>>{});
}

// HashOf

namespace internal {

template <size_t ArgToCombine, size_t ArgCount>
struct TupleHashOfHelper {
  template <typename TupleType>
  FUN_ALWAYS_INLINE static uint32 Do(uint32 hash, const TupleType& tuple) {
    return TupleHashOfHelper<ArgToCombine+1, ArgCount>::Do(HashCombine(hash, HashOf(Get<ArgToCombine>(tuple)), tuple);
  }
};

template <size_t ArgIndex>
struct TupleHashOfHelper {
  template <typename TupleType>
  FUN_ALWAYS_INLINE static uint32 Do(uint32 hash, const TupleType& tuple) {
    return hash;
  }
};

}  // namespace internal

template <typename... Ts>
FUN_ALWAYS_INLINE uint32 HashOf(const Tuple<Ts...>& tuple) {
  return internal::TupleHashOfHelper<1u, sizeof...(Ts)>::Do(
      HashOf(Get<0>(tuple)), tuple);
}

}  // namespace fun
