#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/type_traits.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/integer_sequence.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/serialization/archive.h"
//TODO
//#include "fun/base/serialization/structured_archive.h"

namespace fun {

class Archive;

template <typename... Ts>
struct Tuple;

namespace Tuple_internal {
  template <int32 N, typename... Ts>
  struct NthTypeFromParameterPack;

  template <int32 N, typename T, typename... OtherTypes>
  struct NthTypeFromParameterPack<N, T, OtherTypes...> {
    using Type = typename NthTypeFromParameterPack<N - 1, OtherTypes...>::Type;
  };

  template <typename T, typename... OtherTypes>
  struct NthTypeFromParameterPack<0, T, OtherTypes...> {
    using Type = T;
  };

  template <typename T, typename... Ts>
  struct DecayedFrontOfParameterPackIsSameType {
    enum { Value = IsSame<T, typename Decay<typename NthTypeFromParameterPack<0, Ts...>::Type>::Type>::Value };
  };

  template <typename T, size_t Index>
  struct TupleElement {
    template <
      typename... Args,
      typename = typename EnableIf<
        AndValue<
          sizeof...(Args) != 0,
          OrValue<
            sizeof...(Args) != 1,
            Not<Tuple_internal::DecayedFrontOfParameterPackIsSameType<TupleElement, Args...>>
          >
        >::Value
      >::Type
    >
    explicit TupleElement(Args&&... args)
      : value(Forward<Args>(args)...) {
    }

    TupleElement()
      : value() {
    }

    TupleElement(TupleElement&&) = default;
    TupleElement(const TupleElement&) = default;
    TupleElement& operator = (TupleElement&&) = default;
    TupleElement& operator = (const TupleElement&) = default;

    T value;
  };

  template <size_t IterIndex, size_t Index, typename... Ts>
  struct TupleElementHelperImpl;

  template <size_t IterIndex, size_t Index, typename ElementType, typename... Ts>
  struct TupleElementHelperImpl<IterIndex, Index, ElementType, Ts...> : TupleElementHelperImpl<IterIndex + 1, Index, Ts...> {
  };

  template <size_t Index, typename ElementType, typename... Ts>
  struct TupleElementHelperImpl<Index, Index, ElementType, Ts...> {
    using Type = ElementType;

    template <typename TupleType>
    static FUN_ALWAYS_INLINE ElementType& Get(TupleType& tuple) {
      return static_cast<TupleElement<ElementType, Index>&>(tuple).value;
    }

    template <typename TupleType>
    static FUN_ALWAYS_INLINE const ElementType& Get(const TupleType& tuple) {
      return Get((TupleType&)tuple);
    }
  };

  template <size_t DesiredIndex, typename... Ts>
  struct TupleElementHelper : TupleElementHelperImpl<0, DesiredIndex, Ts...> {
  };

  template <size_t ArgCount, size_t ArgToCompare>
  struct EqualityHelper {
    template <typename TupleType>
    FUN_ALWAYS_INLINE static bool Compare(const TupleType& lhs, const TupleType& rhs) {
      return lhs.template Get<ArgToCompare>() == rhs.template Get<ArgToCompare>() && EqualityHelper<ArgCount, ArgToCompare + 1>::Compare(lhs, rhs);
    }
  };

  template <size_t ArgCount>
  struct EqualityHelper<ArgCount, ArgCount> {
    template <typename TupleType>
    FUN_ALWAYS_INLINE static bool Compare(const TupleType& lhs, const TupleType& rhs) {
      return true;
    }
  };

  template <size_t ArgCount, size_t ArgToCompare = 0, bool Last = ArgToCompare + 1 == ArgCount>
  struct LessThanHelper {
    template <typename TupleType>
    FUN_ALWAYS_INLINE static bool Do(const TupleType& lhs, const TupleType& rhs) {
      return lhs.template Get<ArgToCompare>() < rhs.template Get<ArgToCompare>() || (!(rhs.template Get<ArgToCompare>() < lhs.template Get<ArgToCompare>()) && LessThanHelper<ArgCount, ArgToCompare + 1>::Do(lhs, rhs));
    }
  };

  template <size_t ArgCount, size_t ArgToCompare>
  struct LessThanHelper<ArgCount, ArgToCompare, true> {
    template <typename TupleType>
    FUN_ALWAYS_INLINE static bool Do(const TupleType& lhs, const TupleType& rhs) {
      return lhs.template Get<ArgToCompare>() < rhs.template Get<ArgToCompare>();
    }
  };

  template <size_t ArgCount>
  struct LessThanHelper<ArgCount, ArgCount, false> {
    template <typename TupleType>
    FUN_ALWAYS_INLINE static bool Do(const TupleType& lhs, const TupleType& rhs) {
      return false;
    }
  };

  template <typename Indices, typename... Ts>
  struct TupleStorage;

  template <size_t... Indices, typename... Ts>
  struct TupleStorage<IntegerSequence<size_t, Indices...>, Ts...> : TupleElement<Ts, Indices>... {
    template <
      typename... Args,
      typename = typename EnableIf<
        AndValue<
          sizeof...(Args) == sizeof...(Ts) && sizeof...(Args) != 0,
          OrValue<
            sizeof...(Args) != 1,
            Not<Tuple_internal::DecayedFrontOfParameterPackIsSameType<TupleStorage, Args...>>
          >
        >::Value
      >::Type
    >
    explicit TupleStorage(Args&&... args)
      : TupleElement<Ts, Indices>(Forward<Args>(args))... {
    }

    TupleStorage() = default;
    TupleStorage(TupleStorage&&) = default;
    TupleStorage(const TupleStorage&) = default;
    TupleStorage& operator = (TupleStorage&&) = default;
    TupleStorage& operator = (const TupleStorage&) = default;

    template <size_t Index> FUN_ALWAYS_INLINE const typename TupleElementHelper<Index, Ts...>::Type& Get() const { return TupleElementHelper<Index, Ts...>::Get(*this); }
    template <size_t Index> FUN_ALWAYS_INLINE       typename TupleElementHelper<Index, Ts...>::Type& Get()       { return TupleElementHelper<Index, Ts...>::Get(*this); }
  };

  // Specialization of 2-Tuple to give it the API of TPair.
  template <typename _KeyType, typename _ValueType>
  struct TupleStorage<IntegerSequence<size_t, 0, 1>, _KeyType, _ValueType> {
   public:
    using KeyType = _KeyType;
    using ValueType = _ValueType;

   private:
    // Dummy needed for partial template specialization workaround
    template <size_t Index, typename Dummy>
    struct GetHelper;

    template <typename Dummy>
    struct GetHelper<0, Dummy> {
      using ResultType = KeyType;

      static const KeyType& Get(const TupleStorage& tuple) { return tuple.key; }
      static       KeyType& Get(      TupleStorage& tuple) { return tuple.key; }
    };

    template <typename Dummy>
    struct GetHelper<1, Dummy> {
      using ResultType = ValueType;

      static const ValueType& Get(const TupleStorage& tuple) { return tuple.value; }
      static       ValueType& Get(      TupleStorage& tuple) { return tuple.value; }
    };

   public:
    template <typename KeyInitType, typename ValueInitType>
    explicit TupleStorage(KeyInitType&& key_init, ValueInitType&& value_init)
      : key(Forward<KeyInitType>(key_init))
      , value(Forward<ValueInitType>(value_init)) {
    }

    TupleStorage()
      : key()
      , value() {
    }

    TupleStorage(TupleStorage&&) = default;
    TupleStorage(const TupleStorage&) = default;
    TupleStorage& operator = (TupleStorage&&) = default;
    TupleStorage& operator = (const TupleStorage&) = default;

    template <size_t Index> FUN_ALWAYS_INLINE const typename GetHelper<Index, void>::ResultType& Get() const { return GetHelper<Index, void>::Get(*this); }
    template <size_t Index> FUN_ALWAYS_INLINE       typename GetHelper<Index, void>::ResultType& Get()       { return GetHelper<Index, void>::Get(*this); }

    KeyType key;
    ValueType value;
  };

  template <typename Indices, typename... Ts>
  struct TupleImpl;

  template <size_t... Indices, typename... Ts>
  struct TupleImpl<IntegerSequence<size_t, Indices...>, Ts...> : TupleStorage<IntegerSequence<size_t, Indices...>, Ts...> {
   private:
    using Super = TupleStorage<IntegerSequence<size_t, Indices...>, Ts...>;

   public:
    using Super::Get;

    template <
      typename... Args,
      typename = typename EnableIf<
        AndValue<
          sizeof...(Args) == sizeof...(Ts) && sizeof...(Args) != 0,
          OrValue<
            sizeof...(Args) != 1,
            Not<Tuple_internal::DecayedFrontOfParameterPackIsSameType<TupleImpl, Args...>>
          >
        >::Value
      >::Type
    >
    explicit TupleImpl(Args&&... args)
      : Super(Forward<Args>(args)...) {
    }

    TupleImpl() = default;
    TupleImpl(TupleImpl&& other) = default;
    TupleImpl(const TupleImpl& other) = default;
    TupleImpl& operator = (TupleImpl&& other) = default;
    TupleImpl& operator = (const TupleImpl& other) = default;

    //TODO ApplyBefore / ApplyAfter 무슨 차이가 있는건가??

    template <typename FuncType, typename... Args>
    decltype(auto) ApplyAfter(FuncType&& func, Args&&... args) const {
      return func(Forward<Args>(args)..., this->template Get<Indices>()...);
    }

    template <typename FuncType, typename... Args>
    decltype(auto) ApplyBefore(FuncType&& func, Args&&... args) const {
      return func(this->template Get<Indices>()..., Forward<Args>(args)...);
    }

    FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, TupleImpl& tuple) {
      // This should be implemented with a fold expression when our compilers support it
      int tmp[] = { 0, (ar & tuple.template Get<Indices>(), 0)... };
      (void)tmp;
      return ar;
    }

    //TODO
    //FUN_ALWAYS_INLINE friend void operator & (StructuredArchive::Slot slot, TupleImpl& tuple) {
    //  // This should be implemented with a fold expression when our compilers support it
    //  StructuredArchive::Stream stream = slot.EnterStream();
    //  int tmp[] = { 0, (stream.EnterElement() & tuple.template Get<Indices>(), 0)... };
    //  (void)tmp;
    //}

    FUN_ALWAYS_INLINE friend bool operator == (const TupleImpl& lhs, const TupleImpl& rhs) {
      // This could be implemented with a fold expression when our compilers support it
      return EqualityHelper<sizeof...(Ts), 0>::Compare(lhs, rhs);
    }

    FUN_ALWAYS_INLINE friend bool operator != (const TupleImpl& lhs, const TupleImpl& rhs) {
      return !(lhs == rhs);
    }

    FUN_ALWAYS_INLINE friend bool operator < (const TupleImpl& lhs, const TupleImpl& rhs) {
      return LessThanHelper<sizeof...(Ts)>::Do(lhs, rhs);
    }

    FUN_ALWAYS_INLINE friend bool operator <= (const TupleImpl& lhs, const TupleImpl& rhs) {
      return !(rhs < lhs);
    }

    FUN_ALWAYS_INLINE friend bool operator > (const TupleImpl& lhs, const TupleImpl& rhs) {
      return rhs < lhs;
    }

    FUN_ALWAYS_INLINE friend bool operator >= (const TupleImpl& lhs, const TupleImpl& rhs) {
      return !(lhs < rhs);
    }
  };

//#ifdef _MSC_VER
//
//  // Not strictly necessary, but some VC versions give a 'syntax error: <fake-expression>' error
//  // for empty tuples.
//  template <>
//  struct TupleImpl<IntegerSequence<size_t>> {
//    explicit TupleImpl() {}
//
//    // Doesn't matter what these return, or even have a function body,
//    // but they need to be declared
//    template <size_t Index> FUN_ALWAYS_INLINE const int32& Get() const;
//    template <size_t Index> FUN_ALWAYS_INLINE       int32& Get();
//
//    template <typename FuncType, typename... Args>
//    decltype(auto) ApplyAfter(FuncType&& func, Args&&... args) const {
//      return func(Forward<Args>(args)...);
//    }
//
//    template <typename FuncType, typename... Args>
//    decltype(auto) ApplyBefore(FuncType&& func, Args&&... args) const {
//      return func(Forward<Args>(args)...);
//    }
//
//    FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, TupleImpl& tuple) {
//      return ar;
//    }
//
//    FUN_ALWAYS_INLINE friend bool operator == (const TupleImpl& lhs, const TupleImpl& rhs) {
//      return true;
//    }
//
//    FUN_ALWAYS_INLINE friend bool operator != (const TupleImpl& lhs, const TupleImpl& rhs) {
//      return false;
//    }
//
//    FUN_ALWAYS_INLINE friend bool operator < (const TupleImpl& lhs, const TupleImpl& rhs) {
//      return false;
//    }
//
//    FUN_ALWAYS_INLINE friend bool operator <= (const TupleImpl& lhs, const TupleImpl& rhs) {
//      return true;
//    }
//
//    FUN_ALWAYS_INLINE friend bool operator > (const TupleImpl& lhs, const TupleImpl& rhs) {
//      return false;
//    }
//
//    FUN_ALWAYS_INLINE friend bool operator >= (const TupleImpl& lhs, const TupleImpl& rhs) {
//      return true;
//    }
//  };
//
//#endif // _MSC_VER

  template <typename... Ts>
  FUN_ALWAYS_INLINE Tuple<typename Decay<Ts>::Type...> MakeTupleImpl(Ts&&... args) {
    return Tuple<typename Decay<Ts>::Type...>(Forward<Ts>(args)...);
  }

  template <typename IntegerSequence>
  struct TransformTuple_Impl;

  template <size_t... Indices>
  struct TransformTuple_Impl<IntegerSequence<size_t, Indices...>> {
    template <typename TupleType, typename FuncType>
    static decltype(auto) Do(TupleType&& tuple, FuncType func) {
      return MakeTupleImpl(func(Forward<TupleType>(tuple).template Get<Indices>())...);
    }
  };

  template <typename IntegerSequence>
  struct VisitTupleElements_Impl;

  template <size_t... Indices>
  struct VisitTupleElements_Impl<IntegerSequence<size_t, Indices...>> {
    // We need a second function to do the invocation for a particular index,
    // to avoid the pack expansion being
    // attempted on the indices and tuples simultaneously.
    template <size_t Index, typename FuncType, typename... TupleTypes>
    FUN_ALWAYS_INLINE static void InvokeFunc(FuncType&& func, TupleTypes&&... tuples) {
      Invoke(Forward<FuncType>(func), Forward<TupleTypes>(tuples).template Get<Index>()...);
    }

    template <typename FuncType, typename... TupleTypes>
    static void Do(FuncType&& func, TupleTypes&&... tuples) {
      // This should be implemented with a fold expression when our compilers support it
      int tmp[] = { 0, (InvokeFunc<Indices>(Forward<FuncType>(func), Forward<TupleTypes>(tuples)...), 0)... };
      (void)tmp;
    }
  };

  template <typename TupleType>
  struct CVTupleArity;

  template <typename... Ts>
  struct CVTupleArity<const volatile Tuple<Ts...>> {
    enum { Value = sizeof...(Ts) };
  };
}

template <typename... Ts>
struct Tuple : Tuple_internal::TupleImpl<MakeIntegerSequence<size_t, sizeof...(Ts)>, Ts...> {
 private:
  using Super = Tuple_internal::TupleImpl<MakeIntegerSequence<size_t, sizeof...(Ts)>, Ts...>;

 public:
  template <
    typename... Args,
    typename = typename EnableIf<
      AndValue<
        sizeof...(Args) == sizeof...(Ts) && sizeof...(Args) != 0,
        OrValue<
          sizeof...(Args) != 1,
          Not<Tuple_internal::DecayedFrontOfParameterPackIsSameType<Tuple, Args...>>
        >
      >::Value
    >::Type
  >
  explicit Tuple(Args&&... args)
    : Super(Forward<Args>(args)...) {
    // This constructor is disabled for Tuple and zero parameters
    // because VC is incorrectly instantiating it
    // as a move/copy/default constructor.
  }

  Tuple() = default;
  Tuple(Tuple&&) = default;
  Tuple(const Tuple&) = default;
  Tuple& operator = (Tuple&&) = default;
  Tuple& operator = (const Tuple&) = default;
};


/**
 * Traits class which calculates the number of elements in a tuple.
 */
template <typename TupleType>
struct TupleArity : Tuple_internal::CVTupleArity<const volatile TupleType> {
};

/**
 * Makes a Tuple from some arguments.
 * The type of the Tuple elements are the decayed versions of the arguments.
 *
 * @param args  The arguments used to construct the tuple.
 * @return A tuple containing a copy of the arguments.
 *
 * Example:
 *
 * void func(const int32 a, String&& b) {
 *   // Equivalent to:
 *   // Tuple<int32, const char*, String> my_tuple(a, "Hello", MoveTemp(b));
 *   auto my_tuple = MakeTuple(a, "Hello", MoveTemp(b));
 * }
 */
template <typename... Ts>
FUN_ALWAYS_INLINE Tuple<typename Decay<Ts>::Type...> MakeTuple(Ts&&... args) {
  return Tuple_internal::MakeTupleImpl(Forward<Ts>(args)...);
}

template <typename... Ts>
FUN_ALWAYS_INLINE constexpr Tuple<Ts&...> Tie(Ts&... args) noexcept {
  return Tuple<Ts&...>(args...);
}


/**
 * Creates a new Tuple by applying a functor to each of the elements.
 *
 * @param Tuple  The tuple to apply the functor to.
 * @param func   The functor to apply.
 *
 * @return A new tuple of the transformed elements.
 *
 * Example:
 *
 * float       Overloaded(int32 arg);
 * char        Overloaded(const char* arg);
 * const char* Overloaded(const String& arg);
 *
 * void func(const Tuple<int32, const char*, String>& my_tuple) {
 *   // Equivalent to:
 *   // Tuple<float, char, const char*> transformed_tuple(Overloaded(my_tuple.Get<0>()), Overloaded(my_tuple.Get<1>()), Overloaded(my_tuple.Get<2>())));
 *   auto transformed_tuple = TransformTuple(my_tuple, [](const auto& arg) { return Overloaded(arg); });
 * }
 */
template <typename FuncType, typename... Ts>
FUN_ALWAYS_INLINE decltype(auto) TransformTuple(Tuple<Ts...>&& tuple, FuncType func) {
  return Tuple_internal::TransformTuple_Impl<MakeIntegerSequence<size_t, sizeof...(Ts)>>::Do(MoveTemp(tuple), MoveTemp(func));
}

template <typename FuncType, typename... Ts>
FUN_ALWAYS_INLINE decltype(auto) TransformTuple(const Tuple<Ts...>& tuple, FuncType func) {
  return Tuple_internal::TransformTuple_Impl<MakeIntegerSequence<size_t, sizeof...(Ts)>>::Do(tuple, MoveTemp(func));
}


/**
 * Visits each element in the specified tuples in parallel and applies them as arguments to the functor.
 * All specified tuples must have the same number of elements.
 *
 * @param func    The functor to apply.
 * @param tuples  The tuples whose elements are to be applied to the functor.
 *
 * Example:
 *
 * void func(const Tuple<int32, const char*, String>& tuple1, const Tuple<bool, float, String>& tuple2) {
 *   // Equivalent to:
 *   // functor(tuple1.Get<0>(), tuple2.Get<0>());
 *   // functor(tuple1.Get<1>(), tuple2.Get<1>());
 *   // functor(tuple1.Get<2>(), tuple2.Get<2>());
 *   VisitTupleElements(functor, tuple1, tuple2);
 * }
 */
template <typename FuncType, typename FirstTupleType, typename... TupleTypes>
FUN_ALWAYS_INLINE void VisitTupleElements(FuncType&& func, FirstTupleType&& first_tuple, TupleTypes&&... tuples) {
  Tuple_internal::VisitTupleElements_Impl<MakeIntegerSequence<size_t, TupleArity<typename Decay<FirstTupleType>::Type>::Value>>::Do(Forward<FuncType>(func), Forward<FirstTupleType>(first_tuple), Forward<TupleTypes>(tuples)...);
}


//
// HashOf
//

namespace TupleHashOf_internal {
template <size_t ArgToCombine, size_t ArgCount>
struct TupleHashOfHelper   {
  template <typename TupleType>
  FUN_ALWAYS_INLINE static uint32 Do(uint32 hash, const TupleType& tuple) {
    return TupleHashOfHelper<ArgToCombine + 1, ArgCount>::Do(HashCombine(hash, HashOf(tuple.template Get<ArgToCombine>())), tuple);
  }
};

template <size_t ArgIndex>
struct TupleHashOfHelper<ArgIndex, ArgIndex> {
  template <typename TupleType>
  FUN_ALWAYS_INLINE static uint32 Do(uint32 hash, const TupleType& tuple) {
    return hash;
  }
};
} // namespace TupleHashOf_internal

template <typename... Ts>
FUN_ALWAYS_INLINE uint32 HashOf(const Tuple<Ts...>& tuple) {
  return TupleHashOf_internal::TupleHashOfHelper<1u, sizeof...(Ts)>::Do(HashOf(tuple.template Get<0>()), tuple);
}

FUN_ALWAYS_INLINE uint32 HashOf(const Tuple<>& tuple) {
  return 0;
}

} // namespace fun
