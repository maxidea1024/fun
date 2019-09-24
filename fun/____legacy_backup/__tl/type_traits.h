#pragma once

namespace fun {
namespace tl {

/**
 * TODO
 */
template <typename T>
struct HasTrivialConstructor
{
  enum { value = __has_trivial_constructor(T); };
};

/**
 * Is type DerviedT inherited from BaseT.
 */
template <typename DerviedT, typename BaseT>
struct IsDerivedFrom
{
  // Different size types so we can compare their sizes later.
  typedef char No[1];
  typedef char Yes[2];

  // Overloading Test() s.t. only calling it with something that is
  // a BaseT (or inherited from the BaseT) will return a Yes.
  static Yes& Test(BaseT*);
  static Yes& Test(const BaseT*);
  static No& Test(...);

  // Makes a DerviedT ptr.
  static DerviedT* DerivedTypePtr() { return nullptr ;}

  // Test the derived type pointer. If it inherits from BaseT, the Test(BaseT*)
  // will be chosen. If it does not, Test(...) will be chosen.
  static const bool value = sizeof(Test(DerivedTypePtr())) == sizeof(Yes);
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
template <> struct IsCharType<ANSICHAR> : TrueType {};
template <> struct IsCharType<UCS2CHAR> : TrueType {};
template <> struct IsCharType<WIDECHAR> : TrueType {};

//TODO TFormatSpecifier

/**
 * IsReferenceType
 */
template <typename T> struct IsReferenceType : FalseType {};
template <typename T> struct IsReferenceType<T&> : TrueType {};
template <typename T> struct IsReferenceType<T&&> : TrueType {};

/**
 * IsLValueReferenceType
 */
template <typename T> struct IsLValueReferenceType : FalseType {};
template <typename T> struct IsLValueReferenceType<T&> : TrueType {};

/**
 * IsRValueReferenceType
 */
template <typename T> struct IsRValueReferenceType : FalseType {};
template <typename T> struct IsRValueReferenceType<T&&> : TrueType {};

/**
 * IsVoidType
 */
template <typename T> struct IsVoidType : FalseType {};
template <> struct IsVoidType<void> : TrueType {};
template <> struct IsVoidType<void const> : TrueType {};
template <> struct IsVoidType<void volatile> : TrueType {};
template <> struct IsVoidType<void const volatile> : TrueType {};

/**
 * IsFundamentalType
 */
template <typename T>
struct IsFundamentalType
{
  enum { Value = Or<IsArithmetic<T>, IsVoidType<T>>::Value };
};

/**
 * IsCppFunction
 */
template <typename T> struct IsCppFunction : FalseType {};

template <typename RetType, typename... Args>
struct IsCppFunction<RetType(Args...)> : TrueType {};

/**
 * IsZeroConstructible
 */
template <typename T>
struct IsZeroConstructible
{
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
template <typename T, bool TypeIsSmall>
struct CallTraitsParamTypeHelper
{
  typedef const T& ParamType;
  typedef const T& ConstParamType;
};

template <typename T>
struct CallTraitsParamTypeHelper<T, true>
{
  typedef const T ParamType;
  typedef const T ConstParamType;
};

template <typename T>
struct CallTraitsParamTypeHelper<T*, true>
{
  typedef T* ParamType;
  typedef const T* ConstParamType;
};


/**
 * RemoveConst
 */
template <typename T> struct RemoveConst { typedef T type; };
template <typename T> struct RemoveConst<const T> { typedef T type; };



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
struct CallTraitsBase
{
 private:
  enum { PassByValue = Or<AndValue<(sizeof(T) <= sizeof(void*)), IsPOD<T>>, IsArithmetic<T>, IsPointer<T>>::Value };

 public:
  typedef T ValueType;
  typedef T& Reference;
  typedef const T& ConstReference;
  typedef typename CallTraitsParamTypeHelper<T, PassByValue>::ParamType ParamType;
  typedef typename CallTraitsParamTypeHelper<T, PassByValue>::ConstParamType ConstPointerType;
};

template <typename T>
struct CallTraits : public CallTraitsBase<T> {};

// Fix reference-to-reference problems.
template <typename T>
struct CallTraits<T&>
{
  typedef T& ValueType;
  typedef T& Reference;
  typedef const T& ConstReference;
  typedef T& ParamType;
  typedef T& ConstPointerType;
};

// Array types
template <typename T, size_t N>
struct CallTraits<T[N]>
{
 private:
  typedef T ArrayType[N];

 public:
  typedef const T* ValueType;
  typedef ArrayType& Reference;
  typedef const ArrayType& ConstReference;
  typedef const T* const ParamType;
  typedef const T* const ConstPointerType;
};

// const array types
template <typename T, size_t N>
struct CallTraits<const T[N]>
{
 private:
  typedef const T ArrayType[N];

 public:
  typedef const T* ValueType;
  typedef ArrayType& Reference;
  typedef const ArrayType& ConstReference;
  typedef const T* const ParamType;
  typedef const T* const ConstPointerType;
};


/**
 * TODO
 */
template <typename T>
struct TypeTraitsBase
{
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
struct ContainerTraitsBase
{
  // This should be overridden by every container that supports emptying its contents via a move operation.
  enum { MoveWillEmptyContainer = false };
};

template <typename T>
struct ContainerTraits : public ContainerTraitsBase<T> {};

/**
 * TODO
 */
template <typename T, typename U>
struct MoveSupportTraitsBase
{
  // Param type is not an const lvalue reference, which means it's pass-by-value, so we should just provide a single type for copying.
  // Move overloads will be ignored due to SFINAE.
  typedef U Copy;
};

template <typename T>
struct MoveSupportTraitsBase<T, const T&>
{
  // Param type is a const lvalue reference, so we can provide an overload for moving.
  typedef const T& Copy;
  typedef T&& Move;
};

/**
 * This traits class is intended to be used in pairs to allow efficient and correct move-aware overloads for generic types.
 * For example:
 *
 * template <typename T>
 * void Func(typename MoveSupportTraits<T>::Copy obj)
 * {
 *     // Copy obj here
 * }
 *
 * template <typename T>
 * void Func(typename MoveSupportTraits<T>::Move obj)
 * {
 *     // Move from obj here as if it was passed as T&&
 * }
 *
 * Structuring things in this way will handle T being a pass-by-value type (e.g. ints, floats, other 'small' types) which
 * should never have a reference overload.
 */
template <typename T>
struct MoveSupportTraits
  : MoveSupportTraitsBase<T, typename CallTraits<T>::ParamType>
{
};

/**
 * Tests if a type T is bitwise-constructible from a given argument type U.  That is, whether or not
 * the U can be memcpy'd in order to produce an instance of T, rather than having to go
 * via a constructor.
 *
 * Examples:
 * IsBitwiseConstructible<PODType,    PODType   >::Value == true  // PODs can be trivially copied
 * IsBitwiseConstructible<const int*, int*      >::Value == true  // a non-const Derived pointer is trivially copyable as a const Base pointer
 * IsBitwiseConstructible<int*,       const int*>::Value == false // not legal the other way because it would be a const-correctness violation
 * IsBitwiseConstructible<int32,      uint32    >::Value == true  // signed integers can be memcpy'd as unsigned integers
 * IsBitwiseConstructible<uint32,     int32     >::Value == true  // and vice versa
 */
template <typename T, typename Arg>
struct IsBitwiseConstructible
{
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
struct IsBitwiseConstructible<T, T>
{
  // Ts can always be bitwise constructed from itself if it is trivially copyable.
  enum { Value = IsTriviallyCopyConstructible<T>::Value };
};

template <typename T, typename U>
struct IsBitwiseConstructible<const T, U> : IsBitwiseConstructible<T, U>
{
  // Constructing a const T is the same as constructing a T
};

// Const pointers can be bitwise constructed from non-const pointers.
// This is not true for pointer conversions in general, e.g. where an offset may need to be applied in the case
// of multiple inheritance, but there is no way of detecting that at compile-time.
template <typename T>
struct IsBitwiseConstructible<const T*, T*> : TrueType {};

// Unsigned types can be bitwise converted to their signed equivalents, and vice versa.
// (assuming two's-complement, which we are)
template <> struct IsBitwiseConstructible< uint8, int8  > : TrueType {};
template <> struct IsBitwiseConstructible<  int8, uint8 > : TrueType {};
template <> struct IsBitwiseConstructible<uint16, int16 > : TrueType {};
template <> struct IsBitwiseConstructible< int16, uint16> : TrueType {};
template <> struct IsBitwiseConstructible<uint32, int32 > : TrueType {};
template <> struct IsBitwiseConstructible< int32, uint32> : TrueType {};
template <> struct IsBitwiseConstructible<uint64, int64 > : TrueType {};
template <> struct IsBitwiseConstructible< int64, uint64> : TrueType {};


/*

class MyClass_without_Swap
{
 public:
};

class MyClass_with_Swap
{
 public:
  MyClass_with_Swap(int val)
  {
    value_ = val;
  }

  void Swap(MyClass_with_Swap& other)
  {
    std::swap(value_, other.value_);
  }

  int value_;
};


FUN_DEFINE_HAS_METHOD_CHECKER(Swap, void, , MyClass_with_Swap&);

*/
#define FUN_DEFINE_HAS_METHOD_CHECKER(MemberName, Result, ConstModifier, ...) \
  template <typename T> \
  class HasMethod_ ## MemberName \
  { \
    template <typename U, Result(U::*)(__VA_ARGS__) ConstModifier> struct Check; \
    template <typename U> static char MemberTest(Check<U, &U::MemberName> *); \
    template <typename U> static int MemberTest(...); \
   public: \
    enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) }; \
  };

} // namespace tl
} // namespace fun
