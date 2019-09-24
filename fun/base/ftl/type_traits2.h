#pragma once

namespace fun {

template <typename T>
struct IsPointer : FalseType {};

template <typename T>
struct IsPointer<T*> : TrueType {};

template <typename T>
struct IsPointer<T* const> : TrueType {};

template <typename T>
struct IsPointer<T* volatile> : TrueType {};

template <typename T>
struct IsPointer<T* const volatile> : TrueType {};

template <typename T>
struct IsNullPointer
  : BoolConstant<IsSame<typename RemoveCV<T>::Type, decltype(nullptr)> {};

template <typename T>
struct IsUnion : BoolConstant<__is_union(T)> {};

template <typename T>
struct IsClass : BoolConstant<__is_class(T)> {};

template <typename T>
struct IsFundamental
  : BoolConstant<
        IsArithmetic<T>::Value ||
        IsVoid<T>::Value ||
        IsNullPointer<T>::Value
      > {};

template <typename T>
struct IsObject
  : BoolConstant<
        !IsCppFunction<T>::Value &&
        !IsReference<T>::Value &&
        !IsVoid<T>::Value
      > {};

template <typename From, typename To>
struct IsConvertible : BoolConstant<__is_convertible_to(From, To)> {};

template <typename T>
struct IsEnum : BoolConstant<__is_enum(T)> {};

template <typename T>
struct IsCompound : BoolConstant<!IsFundamental<T>::Value> {};

template <typename T>
struct IsMemberPointer
  : BoolConstant<std::is_member_pointer_v<T>> {};

template <typename T>
struct IsScalar
  : BoolConstant<
        IsArithmetic<T>::Value ||
        IsEnum<T>::Value ||
        IsPointer<T>::Value ||
        IsMemberPointer<T>::Value ||
        IsNullPointer<T>::Value
      > {};

template <typename T>
struct IsConst : FalseType {};

template <typename T>
struct IsConst<const T> : TrueType {};

template <typename T>
struct IsVolatile : FalseType {};

template <typename T>
struct IsVolatile<volatile T> : FalseType {};

template <typename T>
struct IsPOD : BoolConstant<__is_pod(T)> {};

template <typename T>
struct IsEmpty : BoolConstant<__is_empty(T))> {};

template <typename T>
struct IsPolymorphic : BoolConstant<__is_polymorphic(T)> {};

template <typename T>
struct IsAbstract : BoolConstant<__is_abstract(T)> {};

template <typename T>
struct IsFinal : BoolConstant<__is_final(T)> {};

template <typename T>
struct IsStandardLayout : BoolConstant<__is_standard_layout(T)> {};

template <typename T>
struct IsLiteralType : BoolConstant<__is_literal_type(T)> {};

template <typename T>
struct IsTrivial : BoolConstant<__is_trivial(T)> {};

template <typename T>
struct IsTriviallyCopyable : BoolConstant<__is_trivially_copyable(T)> {};

template <typename T>
struct HasVirtualDestructor : BoolConstant<__has_virtual_destructor(T)> {};


#if _HAS_CXX17

template <typename T>
struct HasUniqueObjectRepresentations
  : BoolConstant<__has_unique_object_representations(T)> {};

template <typename T>
struct IsAggregate : BoolConstant<__is_aggregate(T)> {};

#endif //_HAS_CXX17


template <typename T, typename... Args>
struct IsConstructible : BoolConstant<__is_constructible(T, Args...)> {};

template <typename T>
struct IsCopyConstructible
  : BoolConstant<__is_constructible(T, typename AddLValueReference<const T>::Type)> {};

template <typename T>
struct IsDefaultConstructible : BoolConstant<__is_constructible(T)> {};

template <typename T, typename = void>
struct _IsImplicitlyDefaultConstructible : FalseType {};

template <typename T>
void _ImplicitlyDefaultConstruct(const T&);

template <typename T>
struct _IsImplicitlyDefaultConstructible<T, Void_T<decltype(_ImplicitlyDefaultConstruct<T>({}))>> {};

template <typename T>
struct IsMoveConsructible : BoolConstant<__is_constructible(T, T)> {};

template <typename To, typename From>
struct IsAssignable : BoolConstant<__is_assignable(To , From)> {};

template <typename T>
struct IsCopyAssignable
  : BoolConstant<
      __is_assignable(
          typename AddLValueReference<T>::Type,
          typename AddLValueReference<T>::Type)
      > {};

template <typename T>
struct IsMoveAssignable
  : BoolConstant<__is_assignable(typename AddLValueReference<T>::Type, T)> {};

template <typename T>
struct IsDestructible : BoolConstant<__is_destructible(T)> {};

template <typename T, typename... Args>
struct IsTriviallyConstructible
  : BoolConstant<__is_trivially_constructible(T, Args...)> {};

template <typename T>
struct IsTriviallyCopyConstructible
  : BoolConstant<__is_trivially_constructible(T, typename AddLValueReference<const T>::Type)> {};

template <typename T>
struct IstriviallyDefaultConstructible
  : BoolConstant<__is_trivially_constructible(T)> {};

template <typename T>
struct IsTriviallyMoveConstructible
  : BoolConstant<__is_trivially_constructible(T, T)> {};

template <typename To, typename From>
struct IsTriviallyAssignable
  : BoolConstant<__is_trivially_assignable(To, From)> {};

template <typename T>
struct IsTriviallyCopyAssignable
  : BoolConstant<
        __is_trivially_assignable(
            typename AddLValueReference<T>::Type,
            typename AddLValueReference<const T>::Type>
          )
      > {};

template <typename T>
struct IsTriviallyMoveAssignable
  : BoolConstant<__is_trivially_assignable(typename AddLValueReference<T>::Type, T)> {};

template <typename T>
struct IsTriviallyDestructible
  : BoolConstant<__is_trivially_destructible(T)> {};


//TODO 더 추가해야하나...

//is_nothrow_constructible

//is_nothrow_copy_constructible

//is_nothrow_default_constructible

//is_nothrow_move_constructible

//is_nothrow_assignable

//is_nothrow_copy_assignable

//is_nothrow_move_assignable
template <typename T>
struct IsNothrowMoveAssignable
  : BoolConstant<
      __is_nothrow_assignable(
          typename AddLValueReference<T>::Type,
          T)
    > {};

//is_nothrow_destructible
template <typename T>
struct IsNothrowDestructible : BoolConstant<__is_nothrow_destructible(T)> {};


} // namespace fun
