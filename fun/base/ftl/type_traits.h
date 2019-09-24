#pragma once

#include "fun/base/base.h"
#include <initializer_list>
#include <type_traits>

namespace fun {

template <typename T, T ConstValue>
struct IntegralConstant {
  static const T Value = ConstValue;

  using ValueType = T;
  using Type = IntegralConstant<T, ConstValue>;

  operator ValueType() const { return Value; }
};

template <bool BoolValue>
struct BoolConstant {
  enum { Value = BoolValue };
};

typedef BoolConstant<true>  TrueType;
typedef BoolConstant<false> FalseType;

//-----------------------------------------------------------------------------

template <bool Pred, typename ResultType = void> struct EnableIf;
template <typename ResultType> struct EnableIf<true,  ResultType> { using Type = ResultType; };
template <typename ResultType> struct EnableIf<false, ResultType> {};

template <bool Pred, typename Func> struct LazyEnableIf;
template <typename Func> struct LazyEnableIf<true,  Func> { using Type = typename Func::Type; };
template <typename Func> struct LazyEnableIf<false, Func> {};

//-----------------------------------------------------------------------------

/**
 * Chooses between two different classes based on a boolean.
 */
template <bool Pred, typename TrueType, typename FalseType>
struct Conditional;

template <typename TrueType, typename FalseType>
struct Conditional<true, TrueType, FalseType> {
  using Result = TrueType;
};

template <typename TrueType, typename FalseType>
struct Conditional<false, TrueType, FalseType> {
  using Result = FalseType;
};

template <bool Pred, typename TrueType, typename FalseType>
using Conditional_T = typename Conditional<Pred, TrueType, FalseType>::Result; // C++14

//-----------------------------------------------------------------------------

template <typename... Ts>
struct And;

template <bool LhsValue, typename... Rhs>
struct AndValue {
  enum { Value = And<Rhs...>::Value };
};

template <typename... Rhs>
struct AndValue<false, Rhs...> : FalseType {};

template <typename Lhs, typename... Rhs>
struct And<Lhs, Rhs...> : AndValue<Lhs::Value, Rhs...> {};

template <> struct And<> : TrueType {};

/**
 * Does a boolean OR of the ::Value static members of each type,
 * but short-circuits if any Type::Value == true.
 */
template <typename... Ts>
struct Or;

template <bool LhsValue, typename... Rhs>
struct OrValue {
  enum { Value = Or<Rhs...>::Value };
};

template <typename... Rhs>
struct OrValue<true, Rhs...> : TrueType {};

template <typename Lhs, typename... Rhs>
struct Or<Lhs, Rhs...> : OrValue<Lhs::Value, Rhs...> {};

template <> struct Or<> : FalseType {};

/**
 * Does a boolean NOT of the ::Value static members of the type.
 */
template <typename Type>
struct Not {
  enum { Value = !Type::Value };
};

//-----------------------------------------------------------------------------

//UnaryIdentityFunctor
//Projection를 지정할때 기본으로 아무런 동작도 안하고자 하는 경우에
//기본 인자로 사용하기 위함.
/**
 * A functor which returns whatever is passed to it.
 * Mainly used for generic composition.
 */
struct IdentityFunctor {
  template <typename T>
  FUN_ALWAYS_INLINE T&& operator()(T&& value) const {
    return (T&&)value;
  }
};

//-----------------------------------------------------------------------------

/**
 * Determines if T is constructible from a set of arguments.
 */
template <typename T, typename... Args>
struct IsConstructible {
  //enum { Value = __is_constructible(T, Args...) };
  enum { Value = std::is_constructible<T, Args...>::value };
};

template <typename T>
struct IsDestructible {
  enum { Value = __is_destructible(T) };
};

/**
 * TODO
 */
template <typename T>
struct HasTrivialConstructor {
  enum { Value = __has_trivial_constructor(T) };
};

/**
 * Traits class which tests if a type has a trivial destructor.
 */
template <typename T>
struct IsTriviallyDestructible {
  enum { Value = __has_trivial_destructor(T) };
};

/**
 * Traits class which tests if a type has a trivial copy assignment operator.
 */
template <typename T>
struct IsTriviallyCopyAssignable {
  enum { Value = OrValue<__has_trivial_assign(T), IsPOD<T>>::Value };
};

/**
 * Traits class which tests if a type has a trivial copy constructor.
 */
template <typename T>
struct IsTriviallyCopyConstructible {
  enum { Value = OrValue<__has_trivial_copy(T), IsPOD<T>>::Value };
};

/**
 * Traits class which tests if a type is trivial.
 */
template <typename T>
struct IsTrivial {
  enum { Value = And<IsTriviallyDestructible<T>, IsTriviallyCopyConstructible<T>, IsTriviallyCopyAssignable<T>>::Value };
};

//-----------------------------------------------------------------------------

/**
 * Traits class which tests if a type is POD.
 */
template <typename T>
struct IsPOD {
  enum { Value = std::is_pod<T>::value };
};

/**
 * std::is_abstract
 */
template <typename T>
struct IsAbstract {
  enum { Value = __is_abstract(T) };
};

/**
 * Traits class which tests if a type is arithmetic.
 */
template <typename T> struct IsArithmetic : FalseType {};
template <> struct IsArithmetic<float      > : TrueType {};
template <> struct IsArithmetic<double     > : TrueType {};
template <> struct IsArithmetic<long double> : TrueType {};
template <> struct IsArithmetic<uint8      > : TrueType {};
template <> struct IsArithmetic<uint16     > : TrueType {};
template <> struct IsArithmetic<uint32     > : TrueType {};
template <> struct IsArithmetic<uint64     > : TrueType {};
template <> struct IsArithmetic<int8       > : TrueType {};
template <> struct IsArithmetic<int16      > : TrueType {};
template <> struct IsArithmetic<int32      > : TrueType {};
template <> struct IsArithmetic<int64      > : TrueType {};
template <> struct IsArithmetic<bool       > : TrueType {};
template <> struct IsArithmetic<char       > : TrueType {};
template <> struct IsArithmetic<wchar_t    > : TrueType {};

//TODO 기타 유니코드 타입은?
template <typename T> struct IsArithmetic<const T         > { enum { Value = IsArithmetic<T>::Value }; };
template <typename T> struct IsArithmetic<volatile T      > { enum { Value = IsArithmetic<T>::Value }; };
template <typename T> struct IsArithmetic<const volatile T> { enum { Value = IsArithmetic<T>::Value }; };

/**
 * Traits class which tests if a type is a C++ array.
 */
template <typename T>           struct IsCppArray       : FalseType {};
template <typename T>           struct IsCppArray<T[]>  : TrueType {};
template <typename T, size_t N> struct IsCppArray<T[N]> : TrueType {};

/**
 * Traits class which tests if a type is a bounded C++ array.
 */
template <typename T>           struct IsBoundedCppArray       : FalseType {};
template <typename T, size_t N> struct IsBoundedCppArray<T[N]> : TrueType {};

/**
 * Traits class which tests if a type is an unbounded C++ array.
 */
template <typename T> struct IsUnboundedCppArray      : FalseType {};
template <typename T> struct IsUnboundedCppArray<T[]> : TrueType {};

/**
 * Removes one dimension of extents from an array type.
 */
template <typename T> struct RemoveExtent { using Type = T; };
template <typename T> struct RemoveExtent<T[]> { using Type = T; };
template <typename T, size_t N> struct RemoveExtent<T[N]> { using Type = T; };

template <typename T> struct RemoveAllExtents { using Type = T; };
template <typename T> struct RemoveAllExtents<T[]> { using Type = typename RemoveAllExtents<T>::Type; };
template <typename T, size_t N> struct RemoveAllExtents<T[N]>{ using Type = typename RemoveAllExtents<T>::Type; };

template <typename T>
using RemoveExtent_T = typename RemoveExtent<T>::Type; // C++14
template <typename T>
using RemoveAllExtents_T = typename RemoveAllExtents<T>::Type; // C++14

/**
 * Type trait which returns true if the type T is an array or a reference to an array of ArrType.
 */
template <typename T, typename ArrayType>
struct IsCppArrayOrRefOfType { enum { Value = false }; };

template <typename ArrayType> struct IsCppArrayOrRefOfType<               ArrayType[], ArrayType> { enum { Value = true }; };
template <typename ArrayType> struct IsCppArrayOrRefOfType<const          ArrayType[], ArrayType> { enum { Value = true }; };
template <typename ArrayType> struct IsCppArrayOrRefOfType<      volatile ArrayType[], ArrayType> { enum { Value = true }; };
template <typename ArrayType> struct IsCppArrayOrRefOfType<const volatile ArrayType[], ArrayType> { enum { Value = true }; };

template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<               ArrayType[N], ArrayType> { enum { Value = true }; };
template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<const          ArrayType[N], ArrayType> { enum { Value = true }; };
template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<      volatile ArrayType[N], ArrayType> { enum { Value = true }; };
template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<const volatile ArrayType[N], ArrayType> { enum { Value = true }; };

template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<               ArrayType(&)[N], ArrayType> { enum { Value = true }; };
template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<const          ArrayType(&)[N], ArrayType> { enum { Value = true }; };
template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<      volatile ArrayType(&)[N], ArrayType> { enum { Value = true }; };
template <typename ArrayType, size_t N> struct IsCppArrayOrRefOfType<const volatile ArrayType(&)[N], ArrayType> { enum { Value = true }; };

//-----------------------------------------------------------------------------

//TODO Rank...

/**
 * Traits class which tests if a type is a pointer.
 */
template <typename T> struct IsPointer : FalseType {};

template <typename T> struct IsPointer< T*              > : TrueType {};
template <typename T> struct IsPointer<const T*         > : TrueType {};
template <typename T> struct IsPointer<volatile T*      > : TrueType {};
template <typename T> struct IsPointer<const volatile T*> : TrueType {};

template <typename T> struct IsPointer<const T         > { enum { Value = IsPointer<T>::Value }; };
template <typename T> struct IsPointer<volatile T      > { enum { Value = IsPointer<T>::Value }; };
template <typename T> struct IsPointer<const volatile T> { enum { Value = IsPointer<T>::Value }; };

//-----------------------------------------------------------------------------

template <typename T> struct IsConstValue : FalseType {};
template <typename T> struct IsConstValue<const T*> : TrueType {};
template <typename T> struct IsConstValue<const volatile T*> : TrueType {};

template <typename T> struct IsConst : IsConstValue<T*> {};
template <typename T> struct IsConst<T&> : FalseType {};

template <typename T> struct IsVolatileValue : FalseType {};
template <typename T> struct IsVolatileValue<volatile T*> : TrueType {};
template <typename T> struct IsVolatileValue<const volatile T*> : TrueType {};

template <typename T> struct IsVolatile : IsVolatileValue<T*> {};
template <typename T> struct IsVolatile<T&> : FalseType {};

//-----------------------------------------------------------------------------

template <typename T>
struct IsEnum {
  enum { Value = __is_enum(T) };
};

//TODO 적용을 해야하나 말아야하나?
template <typename WrappedEnumType>
struct IsWrappedEnum : FalseType {
};

namespace IsEnumClass_internal {

template <typename T>
struct IsEnumConvertibleToInt {
  static char (&Resolve(int))[2];
  static char Resolve(...);

  enum { Value = sizeof(Resolve(T())) - 1 };
};

} // IsEnumClass_internal

//TODO IsEnum와의 차이는 무엇인지??
//IsEnum은 정확하게 enum인지 체크하는것이고
//IsEnumClass는 enum으로 캐스팅이 가능한지 여부로??
/**
 * Traits class which tests if a type is arithmetic.
 */
template <typename T>
struct IsEnumClass {
  enum { Value = AndValue<__is_enum(T), Not<IsEnumClass_internal::IsEnumConvertibleToInt<T>>>::Value };
};

//-----------------------------------------------------------------------------

/**
 * Traits class which tests if a type is floating point.
 */
template <typename T> struct IsFloatingPoint : FalseType {};

template <> struct IsFloatingPoint<float      > : TrueType {};
template <> struct IsFloatingPoint<double     > : TrueType {};
template <> struct IsFloatingPoint<long double> : TrueType {};

template <typename T> struct IsFloatingPoint<const T         > { enum { Value = IsFloatingPoint<T>::Value }; };
template <typename T> struct IsFloatingPoint<volatile T      > { enum { Value = IsFloatingPoint<T>::Value }; };
template <typename T> struct IsFloatingPoint<const volatile T> { enum { Value = IsFloatingPoint<T>::Value }; };

/**
 * Traits class which tests if a type is integral.
 */
template <typename T> struct IsIntegral : FalseType {};

template <> struct IsIntegral<uint8   > : TrueType {};
template <> struct IsIntegral<uint16  > : TrueType {};
template <> struct IsIntegral<uint32  > : TrueType {};
template <> struct IsIntegral<uint64  > : TrueType {};
template <> struct IsIntegral<int8    > : TrueType {};
template <> struct IsIntegral<int16   > : TrueType {};
template <> struct IsIntegral<int32   > : TrueType {};
template <> struct IsIntegral<int64   > : TrueType {};
template <> struct IsIntegral<bool    > : TrueType {};
template <> struct IsIntegral<char    > : TrueType {};
template <> struct IsIntegral<wchar_t > : TrueType {};
template <> struct IsIntegral<char16_t> : TrueType {};
template <> struct IsIntegral<char32_t> : TrueType {};

template <typename T> struct IsIntegral<const T         > { enum { Value = IsIntegral<T>::Value }; };
template <typename T> struct IsIntegral<volatile T      > { enum { Value = IsIntegral<T>::Value }; };
template <typename T> struct IsIntegral<const volatile T> { enum { Value = IsIntegral<T>::Value }; };

/**
 * Traits class which tests if a type is a signed integral type.
 */
template <typename T> struct IsSigned : FalseType {};

template <> struct IsSigned<int8 > : TrueType {};
template <> struct IsSigned<int16> : TrueType {};
template <> struct IsSigned<int32> : TrueType {};
template <> struct IsSigned<int64> : TrueType {};

template <typename T> struct IsSigned<const T         > { enum { Value = IsSigned<T>::Value }; };
template <typename T> struct IsSigned<volatile T      > { enum { Value = IsSigned<T>::Value }; };
template <typename T> struct IsSigned<const volatile T> { enum { Value = IsSigned<T>::Value }; };

template <typename T> struct MakeSigned { using Type = T; };

template <> struct MakeSigned<uint8 > { using Type = int8 ; };
template <> struct MakeSigned<uint16> { using Type = int16; };
template <> struct MakeSigned<uint32> { using Type = int32; };
template <> struct MakeSigned<uint64> { using Type = int64; };

template <typename T> struct MakeUnsigned { using Type = T; };

template <> struct MakeUnsigned<int8 > { using Type = uint8; };
template <> struct MakeUnsigned<int16> { using Type = uint16; };
template <> struct MakeUnsigned<int32> { using Type = uint32; };
template <> struct MakeUnsigned<int64> { using Type = uint64; };

//-----------------------------------------------------------------------------

/**
 * Determines if T is struct/class type.
 */
template <typename T>
struct IsClass {
 public:
  enum { Value = !__is_union(T) && sizeof(Func<T>(0)) - 1 };

 private:
  template <typename U> static uint16 Func(int U::*);
  template <typename U> static uint8 Func(...);
};

//-----------------------------------------------------------------------------

/**
 * Tests if a FromType* is convertible to a ToType*
 */
template <typename FromType, typename ToType>
struct PointerIsConvertibleFromTo {
 private:
  static uint8 Test(...);
  static uint16 Test(ToType*);

 public:
  enum { Value = sizeof(Test((FromType*)nullptr)) - 1 };
};

//-----------------------------------------------------------------------------

//TODO std::is_base_of 와 동일하게 처리하는게 좋을듯...?

/**
 * Is type DerivedType inherited from BaseType.
 */
template <typename DerivedType, typename BaseType>
struct IsDerivedFrom {
  // Different size types so we can compare their sizes later.
  typedef char No[1];
  typedef char Yes[2];

  // Overloading Test() s.t. only calling it with something that is
  // a BaseType (or inherited from the BaseType) will return a Yes.
  static Yes& Test(BaseType*);
  static Yes& Test(const BaseType*);
  static No& Test(...);

  // Makes a DerivedType ptr.
  static DerivedType* DerivedTypePtr() { return nullptr ;}

  // Test the derived type pointer. If it inherits from BaseType, the Test(BaseType*)
  // will be chosen. If it does not, Test(...) will be chosen.
  static const bool Value = sizeof(Test(DerivedTypePtr())) == sizeof(Yes);
};

/**
 * IsSame (std::is_same)
 */
template <typename A, typename B> struct IsSame : FalseType {};
template <typename T> struct IsSame<T, T> : TrueType {};

/**
 * IsCharType
 */
template <typename T> struct IsCharType : FalseType {};
template <> struct IsCharType<char    > : TrueType {};
template <> struct IsCharType<char16_t> : TrueType {};
template <> struct IsCharType<char32_t> : TrueType {};
template <> struct IsCharType<wchar_t > : TrueType {};

//TODO IsCharType CV

//TODO FormatSpecifier

/**
 * IsReferenceType
 */
template <typename T> struct IsReferenceType      : FalseType {};
template <typename T> struct IsReferenceType<T& > : TrueType {};
template <typename T> struct IsReferenceType<T&&> : TrueType {};

/**
 * IsLValueReferenceType
 */
template <typename T> struct IsLValueReferenceType     : FalseType {};
template <typename T> struct IsLValueReferenceType<T&> : TrueType {};

/**
 * IsRValueReferenceType
 */
template <typename T> struct IsRValueReferenceType      : FalseType {};
template <typename T> struct IsRValueReferenceType<T&&> : TrueType {};

/**
 * Wrapper for a type that yields a reference to that type.
 */
template <typename T> struct MakeReferenceTo { typedef T& Type; };
/**
 * Specialization for MakeReferenceTo<void>.
 */
template <> struct MakeReferenceTo<void> { typedef void Type; };

/**
 * IsVoidType
 */
template <typename T> struct IsVoidType : FalseType {};
template <> struct IsVoidType<void               > : TrueType {};
template <> struct IsVoidType<void const         > : TrueType {};
template <> struct IsVoidType<void volatile      > : TrueType {};
template <> struct IsVoidType<void const volatile> : TrueType {};

/**
 * IsFundamentalType
 */
template <typename T>
struct IsFundamentalType {
  enum { Value = Or<IsArithmetic<T>, IsVoidType<T>>::Value };
};

/**
 * IsCppFunction
 */
template <typename T> struct IsCppFunction : FalseType {};

template <typename ReturnType, typename... Args>
struct IsCppFunction<ReturnType (Args...)> : TrueType {};

/**
 * IsZeroConstructible
 */
template <typename T>
struct IsZeroConstructible {
  enum { Value = OrValue<IsEnum<T>::Value, IsArithmetic<T>, IsPointer<T>>::Value };
};

/**
 *  IsWeakPointerType
 */
template <typename T>
struct IsWeakPointerType : FalseType {};

//TODO NameOf

/**
 * Call traits - Modeled somewhat after boost's interfaces.
 */
template <typename T, bool IsSmallType>
struct CallTraitsParamTypeHelper {
  typedef const T& ParamType;
  typedef const T& ConstParamType;
};

template <typename T>
struct CallTraitsParamTypeHelper<T, true> {
  typedef const T ParamType;
  typedef const T ConstParamType;
};

template <typename T>
struct CallTraitsParamTypeHelper<T*, true> {
  typedef T* ParamType;
  typedef const T* ConstParamType;
};

//-----------------------------------------------------------------------------

/**
 * Removes one level of pointer from a type, e.g.:
 *
 * RemovePointer<      int32  >::Type == int32
 * RemovePointer<      int32* >::Type == int32
 * RemovePointer<      int32**>::Type == int32*
 * RemovePointer<const int32* >::Type == const int32
 */
template <typename T> struct RemovePointer     { using Type = T; };
template <typename T> struct RemovePointer<T*> { using Type = T; };

template <typename T>
using RemovePointer_T = typename RemovePointer<T>::Type; // C++14

/**
 * RemoveReference<type> will remove any references from a type.
 */
template <typename T> struct RemoveReference      { using Type = T; };
template <typename T> struct RemoveReference<T& > { using Type = T; };
template <typename T> struct RemoveReference<T&&> { using Type = T; };

template <typename T>
using RemoveReference_T = typename RemoveReference<T>::Type; // C++14

/**
 * RemoveConst
 */
template <typename T> struct RemoveConst                       { using Type = T; };
template <typename T> struct RemoveConst<const T>              { using Type = T; };
template <typename T> struct RemoveConst<const T[]>            { using Type = T[]; };
template <typename T, size_t N> struct RemoveConst<const T[N]> { using Type = T[N]; };

template <typename T>
using RemoveConst_T = typename RemoveConst<T>::Type; // C++14

/**
 * RemoveVolatile
 */
template <typename T> struct RemoveVolatile                          { using Type = T; };
template <typename T> struct RemoveVolatile<volatile T>              { using Type = T; };
template <typename T> struct RemoveVolatile<volatile T[]>            { using Type = T[]; };
template <typename T, size_t N> struct RemoveVolatile<volatile T[N]> { using Type = T[N]; };

/**
 * RemoveCV
 */
template <typename T>
struct RemoveCV {
  using Type = typename RemoveVolatile<typename RemoveConst<T>::Type>::Type;
};

template <typename T>
using RemoveCV_T = typename RemoveCV<T>::Type; // C++14

/**
 * RemoveCVRef
 */
template <typename T>
struct RemoveCVRef {
  using Type = typename RemoveVolatile<typename RemoveConst<typename RemoveReference<T>::Type>::Type>::Type;
};

template <typename T>
using RemoveCVRef_T = typename RemoveCVRef<T>::Type; // C++14


template <typename T> struct AddConst    { using Type = const T; };
template <typename T> struct AddVolatile { using Type = volatile T; };
template <typename T> struct AddCV       { using Type = const volatile T; };

template <typename T>
using AddConst_T = typename AddConst<T>::Type; // C++14
template <typename T>
using AddVolatile_T = typename AddVolatile<T>::Type; // C++14
template <typename T>
using AddCV_T = typename AddCV<T>::Type; // C++14

/**
 * RValueToLValueReference converts any rvalue reference type into
 * the equivalent lvalue reference, otherwise returns the same type.
 */
template <typename T> struct RValueToLValueReference      { using Type = T; };
template <typename T> struct RValueToLValueReference<T&&> { using Type = T&; };

template <typename T>
using RValueToLValueReference_T = typename RValueToLValueReference<T>::Type; // C++14

//-----------------------------------------------------------------------------

/**
 * CallTraits
 *
 * Same call traits as boost, though not with as complete a solution.
 *
 * The main member to note is ParamType, which specifies the optimal
 * form to pass the type as a parameter to a function.
 *
 * Has a small-value optimization when a type is a POD type and as small as a pointer.
 * -----------------------------------------------------------------------------
 *
 *
 * base class for call traits. Used to more easily refine portions when specializing
 */
template <typename T>
struct CallTraitsBase {
 private:
  enum { PassByValue = Or<AndValue<(sizeof(T) <= sizeof(void*)), IsPOD<T>>, IsArithmetic<T>, IsPointer<T>>::Value };

 public:
  using ValueType = T;
  using Reference = T&;
  using ConstReference = const T&;
  using ParamType = typename CallTraitsParamTypeHelper<T, PassByValue>::ParamType;
  using ConstPointerType = typename CallTraitsParamTypeHelper<T, PassByValue>::ConstParamType;
};

template <typename T>
struct CallTraits : public CallTraitsBase<T> {};

// Fix reference-to-reference problems.
template <typename T>
struct CallTraits<T&> {
  using ValueType = T&;
  using Reference = T&;
  using ConstReference = const T&;
  using ParamType = T&;
  using ConstPointerType = T&;
};

// Array types
template <typename T, size_t N>
struct CallTraits<T[N]> {
 private:
  typedef T ArrayType[N];

 public:
  using ValueType = const T*;
  using Reference = ArrayType&;
  using ConstReference = const ArrayType&;
  using ParamType = const T* const;
  using ConstPointerType = const T* const;
};

// const array types
template <typename T, size_t N>
struct CallTraits<const T[N]> {
 private:
  typedef const T ArrayType[N];

 public:
  using ValueType = const T*;
  using Reference = ArrayType&;
  using ConstReference = const ArrayType&;
  using ParamType = const T* const;
  using ConstPointerType = const T* const;
};

/**
 * TODO
 */
template <typename T>
struct TypeTraitsBase {
  typedef typename CallTraits<T>::ParamType ConstInitType;
  typedef typename CallTraits<T>::ConstPointerType ConstPointerType;

  // There's no good way of detecting this so we'll just assume it to be true for certain known types and expect
  // users to customize it for their custom types.
  enum { IsBytewiseComparable = OrValue<IsEnum<T>::Value, IsArithmetic<T>, IsPointer<T>>::Value };
};

/**
 * Traits for types.
 */
template <typename T>
struct TypeTraits : public TypeTraitsBase<T> {};

/**
 * Traits for containers.
 */
template <typename T>
struct ContainerTraitsBase {
  // This should be overridden by every container
  // that supports emptying its contents via a move operation.
  enum { MoveWillEmptyContainer = false };
};

template <typename T>
struct ContainerTraits : public ContainerTraitsBase<T> {};

/**
 * TODO
 */
template <typename T, typename U>
struct MoveSupportTraitsBase {
  // Param type is not an const lvalue reference,
  // which means it's pass-by-value, so we should
  // just provide a single type for copying.
  //
  // Move overloads will be ignored due to SFINAE.
  using Copy = U;
};

template <typename T>
struct MoveSupportTraitsBase<T, const T&> {
  // Param type is a const lvalue reference, so we can
  // provide an overload for moving.
  using Copy = const T&;
  using Move = T&&;
};

/**
 * This traits class is intended to be used in pairs to allow
 * efficient and correct move-aware overloads for generic types.
 *
 * For example:
 *
 * template <typename T>
 * void Func(typename MoveSupportTraits<T>::Copy obj) {
 *   // Copy obj here
 * }
 *
 * template <typename T>
 * void Func(typename MoveSupportTraits<T>::Move obj) {
 *   // Move from obj here as if it was passed as T&&
 * }
 *
 * Structuring things in this way will handle T being
 * a pass-by-value type (e.g. ints, floats, other 'small' types) which
 * should never have a reference overload.
 */
template <typename T>
struct MoveSupportTraits
  : MoveSupportTraitsBase<T, typename CallTraits<T>::ParamType> {};

/**
 * Tests if a type T is bitwise-constructible from a given
 * argument type U.  That is, whether or not
 * the U can be memcpy'd in order to produce an instance of T,
 * rather than having to go via a constructor.
 *
 * Examples:
 * IsBitwiseConstructible<PODType,    PODType   >::Value == true  // PODs can be trivially copied
 * IsBitwiseConstructible<const int*, int*      >::Value == true  // a non-const Derived pointer is trivially copyable as a const Base pointer
 * IsBitwiseConstructible<int*,       const int*>::Value == false // not legal the other way because it would be a const-correctness violation
 * IsBitwiseConstructible<int32,      uint32    >::Value == true  // signed integers can be memcpy'd as unsigned integers
 * IsBitwiseConstructible<uint32,     int32     >::Value == true  // and vice versa
 */
template <typename T, typename Arg>
struct IsBitwiseConstructible {
  static_assert(
    !IsReferenceType<T>::Value &&
    !IsReferenceType<Arg>::Value,
    "IsBitwiseConstructible is not designed to accept reference types");

  static_assert(
    IsSame<T, typename RemoveCV<T>::Type>::Value &&
    IsSame<Arg, typename RemoveCV<Arg>::Type>::Value,
    "IsBitwiseConstructible is not designed to accept qualified types");

  // Assume no bitwise construction in general
  enum { Value = false };
};

template <typename T>
struct IsBitwiseConstructible<T, T> {
  // Ts can always be bitwise constructed from itself if it is trivially copyable.
  enum { Value = IsTriviallyCopyConstructible<T>::Value };
};

template <typename T, typename U>
struct IsBitwiseConstructible<const T, U> : IsBitwiseConstructible<T, U> {
  // Constructing a const T is the same as constructing a T
};

// Const pointers can be bitwise constructed from non-const pointers.
// This is not true for pointer conversions in general,
// e.g. where an offset may need to be applied in the case
// of multiple inheritance, but there is no way of detecting that at compile-time.
template <typename T>
struct IsBitwiseConstructible<const T*, T*> : TrueType {};

// Unsigned types can be bitwise converted to their signed equivalents, and vice versa.
// (assuming two's-complement, which we are)
template <> struct IsBitwiseConstructible< uint8,  int8 > : TrueType {};
template <> struct IsBitwiseConstructible<  int8,  uint8> : TrueType {};
template <> struct IsBitwiseConstructible<uint16,  int16> : TrueType {};
template <> struct IsBitwiseConstructible< int16, uint16> : TrueType {};
template <> struct IsBitwiseConstructible<uint32,  int32> : TrueType {};
template <> struct IsBitwiseConstructible< int32, uint32> : TrueType {};
template <> struct IsBitwiseConstructible<uint64,  int64> : TrueType {};
template <> struct IsBitwiseConstructible< int64, uint64> : TrueType {};

//-----------------------------------------------------------------------------

/**
 * Wraps a type.
 *
 * The intended use of this template is to allow types to be passed around
 * where the unwrapped type would give different behavior.
 *
 * An example of this is when you want a template specialization to
 * refer to the primary template, but doing that would cause
 * infinite recursion through the specialization, e.g.:
 *
 * // Before
 * template <typename T>
 * struct Thing {
 *   void f(T t) {
 *      DoSomething(t);
 *   }
 * };
 *
 * template <>
 * struct Thing<int> {
 *   void f(int t) {
 *     DoSomethingElseFirst(t);
 *     Thing<int>::f(t); // Infinite recursion
 *   }
 * };
 *
 * // After
 * template <typename T>
 * struct Thing {
 *   typedef typename UnwrapType<T>::Type RealType;
 *
 *   void f(RealType t) {
 *      DoSomething(t);
 *   }
 * };
 *
 * template <>
 * struct Thing<int> {
 *   void f(int t) {
 *     DoSomethingElseFirst(t);
 *     Thing<TypeWrapper<int>>::f(t); // works
 *   }
 * };
 */
template <typename T>
struct TypeWrapper;

template <typename T>
struct UnwrapType { using Type = T; };

template <typename T>
struct UnwrapType<TypeWrapper<T>> { using Type = T; };

template <typename T>
using UnwrapType_T = typename UnwrapType<T>::Type; // C++14

//-----------------------------------------------------------------------------

template <typename T, typename... Rest>
struct Front {
  using Type = T;
};

template <typename T, typename... Rest>
struct Back {
  using Type = typename Back<Rest...>::Type;
};

template <typename T>
struct Back<T> {
  using Type = T;
};

template <typename T>
using Front_T = typename Front<T>::Type; // C++14
template <typename T>
using Back_T = typename Back<T>::Type; // C++14

template <typename T1, typename T2, typename... Rest>
struct AreSameTypes {
  static constexpr bool Value = IsSame<T1, T2>::Value ? true : AreSameTypes<T1, Rest...>::Value;
};

template <typename T1, typename T2>
struct AreSameTypes<T1, T2> {
  static constexpr bool Value = IsSame<T1,T2>::Value;
};

template <typename T, typename... Rest>
struct AreDifferentTypes {
  static constexpr bool Value = AreSameTypes<T, Rest...>::Value ? false : AreDifferentTypes<Rest...>::Value;
};

template <typename T1>
struct AreDifferentTypes<T1> {
  static constexpr bool Value = true;
};

//-----------------------------------------------------------------------------

/**
 * Copies the cv-qualifiers from one type to another, e.g.:
 *
 * CopyQualifiersFromTo<const T1, T2>::Type           == const T2
 * CopyQualifiersFromTo<volatile T1, const T2>::Type  == const volatile T2
 */
template <typename From, typename To> struct CopyQualifiersFromTo { typedef To Type; };
template <typename From, typename To> struct CopyQualifiersFromTo<const From, To> { typedef const To Type; };
template <typename From, typename To> struct CopyQualifiersFromTo<volatile From, To> { typedef volatile To Type; };
template <typename From, typename To> struct CopyQualifiersFromTo<const volatile From, To> { typedef const volatile To Type; };

template <typename From, typename To>
using CopyQualifiersFromTo_T = typename CopyQualifiersFromTo<From, To>::Type; // C++14

/**
 * Tests if qualifiers are lost between one type and another, e.g.:
 *
 * LosesQualifiersFromTo<const T1, T2>::Value                    == true
 * LosesQualifiersFromTo<volatile T1, const volatile T2>::Value  == false
 */
template <typename From, typename To>
struct LosesQualifiersFromTo {
  enum { Value = !IsSame<typename CopyQualifiersFromTo<From, To>::Type, To>::Value };
};

/**
 * Returns the same type passed to it.  This is useful in a few cases,
 * but mainly for inhibiting template argument deduction
 * in function arguments, e.g.:
 *
 * template <typename T>
 * void Func1(T val); // Can be called like Func(123) or Func<int>(123);
 *
 * template <typename T>
 * void Func2(typename Identity<T>::Type val); // Must be called like Func<int>(123)
 */
template <typename T>
struct Identity {
  using Type = T;
};

template <typename T>
using Identity_T = typename Identity<T>::Type; // C++14

/**
 * Equivalent to std::declval.
 *
 * Note that this function is unimplemented, and is only intended to be used in
 * unevaluated contexts, like sizeof and trait expressions.
 */
template <typename T>
T&& DeclVal();

//-----------------------------------------------------------------------------

/**
 * Returns the decayed type of T, meaning it removes all references, qualifiers and
 * applies array-to-pointer and function-to-pointer conversions.
 *
 * http://en.cppreference.com/w/cpp/types/decay
 */
namespace Decay_internal {

template <typename T>
struct DecayNonReference {
  using Type = typename RemoveCV<T>::Type;
};

// unbounded C++ array
template <typename T>
struct DecayNonReference<T[]> {
  using Type = T*;
};

// bounded C++ array
template <typename T, size_t N>
struct DecayNonReference<T[N]> {
  using Type = T*;
};

// C++ function
template <typename R, typename... Args>
struct DecayNonReference<R (Args...)> {
  typedef R (*Type)(Args...);
};

} // namespace Decay_internal

template <typename T>
struct Decay {
  using Type = typename Decay_internal::DecayNonReference<typename RemoveReference<T>::Type>::Type;
};

template <typename T>
using Decay_T = typename Decay<T>::Type; // C++14

//-----------------------------------------------------------------------------

/**
 * Helper class for dereferencing pointer types in Sort function
 */
template <typename T, typename Predicate>
struct DereferenceWrapper {
  DereferenceWrapper(const Predicate& Pred) : pred_(Pred) {}

  /**
   * Pass through for non-pointer types
   */
  FUN_ALWAYS_INLINE bool operator()(T& a, T& b) { return pred_(a, b); }
  FUN_ALWAYS_INLINE bool operator()(const T& a, const T& b) const { return pred_(a, b); }

 private:
  const Predicate& pred_;
};

/**
 * Partially specialized version of the above class
 */
template <typename T, typename Predicate>
struct DereferenceWrapper<T*, Predicate> {
  DereferenceWrapper(const Predicate& Pred) : pred_(Pred) {}

  /**
   * Dereference pointers
   */
  FUN_ALWAYS_INLINE bool operator()(T* a, T* b) const {
    //note: 단순히 포인터 비교시에는 문제가 될 수 있음.  그러나, 좀더 고민이 필요해보임!!
    return pred_(*a, *b);
    //아니네, 포인터 타입을 Predicate에 넘길수가 없구나... 어쩐다... 흠...
    //return Pred(a, b);
  }

 private:
  const Predicate& pred_;
};


//-----------------------------------------------------------------------------

/**
 * Finds the maximum sizeof the supplied types.
 */
template <typename...>
struct MaxSizeof;

template <>
struct MaxSizeof<> {
  static const uint32 Value = 0;
};

template <typename T, typename... Rest>
struct MaxSizeof<T, Rest...> {
  static const uint32 Value = sizeof(T) > MaxSizeof<Rest...>::Value ? sizeof(T) : MaxSizeof<Rest...>::Value;
};

//-----------------------------------------------------------------------------

template <typename T>
struct UnderlyingType {
  using Type = __underlying_type(T);
};

template <typename T>
using UnderlyingType_T = typename UnderlyingType<T>::Type; // C++14

//-----------------------------------------------------------------------------

//TODO 아래 코드는 별도로 두도록 하자. 그게 바람직해보임.
/*

class MyClass_without_Swap {
 public:
};

class MyClass_with_Swap {
 public:
  MyClass_with_Swap(int val) {
    value_ = val;
  }

  void Swap(MyClass_with_Swap& other) {
    std::swap(value_, other.value_);
  }

  int value_;
};


FUN_DEFINE_HAS_METHOD_CHECKER(Swap, void, , MyClass_with_Swap&);

*/
#define FUN_DEFINE_HAS_METHOD_CHECKER(MemberName, ResultType, ConstModifier, ...) \
  template <typename T> \
  class HasMethod_ ## MemberName { \
    template <typename U, ResultType (U::*)(__VA_ARGS__) ConstModifier> struct Check; \
    template <typename U> static char MemberTest(Check<U, &U::MemberName> *); \
    template <typename U> static int MemberTest(...); \
   public: \
    enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) }; \
  };

//-----------------------------------------------------------------------------

/**
 * Traits class which tests if a type is a contiguous container.
 * Requires:
 *    [ &Container[0], &Container[0] + Num ) is a valid range
 */
template <typename T>
struct IsContiguousContainer {
  enum { Value = false };
};

template <typename T> struct IsContiguousContainer<             T& > : IsContiguousContainer<T> {};
template <typename T> struct IsContiguousContainer<             T&&> : IsContiguousContainer<T> {};
template <typename T> struct IsContiguousContainer<const          T> : IsContiguousContainer<T> {};
template <typename T> struct IsContiguousContainer<      volatile T> : IsContiguousContainer<T> {};
template <typename T> struct IsContiguousContainer<const volatile T> : IsContiguousContainer<T> {};

/**
 * Specialization for C arrays (always contiguous)
 */
template <typename T, size_t N> struct IsContiguousContainer<               T[N]> { enum { Value = true }; };
template <typename T, size_t N> struct IsContiguousContainer<const          T[N]> { enum { Value = true }; };
template <typename T, size_t N> struct IsContiguousContainer<      volatile T[N]> { enum { Value = true }; };
template <typename T, size_t N> struct IsContiguousContainer<const volatile T[N]> { enum { Value = true }; };

/**
 * Specialization for initializer lists (also always contiguous)
 */
template <typename T>
struct IsContiguousContainer<std::initializer_list<T>> {
  enum { Value = true };
};

/**
 * Generically gets the data pointer of a contiguous container
 */
template <typename T, typename = typename EnableIf<IsContiguousContainer<T>::Value>::Type>
decltype(auto) GetMutableData(T&& container) {
  return container.MutableData();
}

template <typename T, size_t N>
constexpr T* GetMutableData(T (&container)[N]) {
  return container;
}

template <typename T>
constexpr T* GetMutableData(std::initializer_list<T> list) {
  return list.begin();
}

/**
 * Generically gets the data pointer of a contiguous container
 */
template <typename T, typename = typename EnableIf<IsContiguousContainer<T>::Value>::Type>
decltype(auto) GetConstData(T&& container) {
  return container.ConstData();
}

template <typename T, size_t N>
constexpr const T* GetConstData(T (&container)[N]) {
  return container;
}

template <typename T>
constexpr const T* GetConstData(std::initializer_list<T> list) {
  return list.begin();
}

/**
 * Generically gets the number of items in a contiguous container
 */
template <typename T, typename = typename EnableIf<IsContiguousContainer<T>::Value>::Type>
size_t GetCount(T&& container) {
  return (size_t)container.Count();
}

template <typename T, size_t N>
constexpr size_t GetCount(T (&container)[N]) {
  return N;
}

template <typename T>
constexpr size_t GetCount(std::initializer_list<T> list) {
  return list.size();
}

/**
 * Returns a non-const pointer type as const.
 */
template <typename T>
FUN_ALWAYS_INLINE const T* AsConst(T* ptr) {
  return ptr;
}

/**
 * Returns a non-const reference type as const.
 */
template <typename T>
FUN_ALWAYS_INLINE const T& AsConst(T& ref) {
  return ref;
}

} // namespace fun
