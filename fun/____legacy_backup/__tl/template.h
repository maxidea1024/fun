#pragma once

#include "fun/base/base.h"
#include "IntegralConstant.h"
#include "EnableIf.h"
#include "AndOrNot.h"
#include "PointerIsConvertibleFromTo.h"
#include "TypeWrapper.h"
#include "HAL/PlatformTypes.h"
#include "TypeTraits.h"
#include "Memory.h"
#include "AlignmentTemplates.h" //*NEW*

namespace fun {
namespace tl {

//
// Standard templates.
//

/**
 * Chooses between the two parameters based on whether the first is nullptr or not.
 *
 * \return If the first parameter provided is non-nullptr, it is returned; otherwise the second parameter is returned.
 */
template <typename ReferencedType>
inline ReferencedType* IfAThenAElseB(ReferencedType* a, ReferencedType* b)
{
  const intptr_t int_a = reinterpret_cast<intptr_t>(a);
  const intptr_t int_b = reinterpret_cast<intptr_t>(b);

  // Compute a mask which has all bits set if int_a is zero, and no bits set if it's non-zero.
  const intptr_t mask_b = -(!int_a);

  return reinterpret_cast<ReferencedType*>(int_a | (mask_b & int_b));
}

/**
 * branchless pointer selection based on predicate
 * return intptr_t(Predicate) ? a : b;
 */
template <typename PredicateType, typename ReferencedType>
inline ReferencedType* IfPThenAElseB(const PredicateType& pred, ReferencedType* a, ReferencedType* b)
{
  const intptr_t int_a = reinterpret_cast<intptr_t>(a);
  const intptr_t int_b = reinterpret_cast<intptr_t>(b);

  // Compute a mask which has all bits set if Predicate is zero, and no bits set if it's non-zero.
  const intptr_t mask_b = -(!intptr_t(pred));

  return reinterpret_cast<ReferencedType*>((int_a & ~mask_b) | (int_b & mask_b));
}

/**
 * a logical exclusive or function.
 */
inline bool XOR(bool x, bool y)
{
  return x != y;
}

/**
 * This is used to provide type specific behavior for a copy which cannot change the value of b.
 */
template <typename T>
inline void Move(T& x, typename MoveSupportTraits<T>::Copy y)
{
  // Destruct the previous value of a.
  x.~T();

  // Use placement new and a copy constructor so types with const members will work.
  new(&x) T(y);
}

/**
 * This is used to provide type specific behavior for a move which may change the value of b.
 */
template <typename T>
inline void Move(T& x, typename MoveSupportTraits<T>::Move y)
{
  // Destruct the previous value of a.
  x.~T();

  // Use placement new and a copy constructor so types with const members will work.
  new(&x) T(MoveTemp(y));
}

/**
 * Tests if an index is valid for a given array.
 *
 * \param array - The array to check against.
 * \param index - The index to check.
 *
 * \return true if the index is valid, false otherwise.
 */
template <typename T, size_t N>
static inline bool IsValidArrayIndex(const T(&array)[N], size_t index)
{
  return index < N;
}


//
// Standard macros.
//

//TODO 아래 코드는 표준으로 대체하는게 좋을듯...

template <typename T, uint32 N>
char (&ArrayCountHelper(const T (&)[N]))[N];

// Number of elements in an array.
#define countof(Array)  (sizeof(ArrayCountHelper(Array)) + 0)

// Offset of a struct member.
#ifndef FUN_CODE_ANALYZER
// JCA uses clang on Windows. According to C++11 standard, (which in this case clang follows and msvc doesn't)
// forbids using reinterpret_cast in constant expressions. msvc uses reinterpret_cast in offsetof macro,
// while clang uses compiler intrinsic. Calling static_assert(OFFSETOF(x, y) == SomeValue) causes compiler
// error when using clang on Windows (while including windows headers).
#define OFFSETOF(Struc, Member)  offsetof(Struc, Member)
#else
#define OFFSETOF(Struc, Member)  __builtin_offsetof(Struc, Member)
#endif

#if PLATFORM_VTABLE_AT_END_OF_CLASS
  #error need implementation
#else
  #define VTABLE_OFFSET(Class, MultipleInheritenceParent)  (((intptr_t)static_cast<MultipleInheritenceParent*>((Class*)1)) - 1)
#endif


//TODO 알고리즘으로 옮겨주는게 좋을듯...

/**
 * works just like std::min_element.
 */
template <typename ForwardIt> inline
ForwardIt MinElement(ForwardIt first, ForwardIt end)
{
  ForwardIt result = first;
  for (; ++first != end;) {
    if (*first < *result) {
      result = first;
    }
  }
  return result;
}

/**
 * works just like std::min_element.
 */
template <typename ForwardIt, typename Predicate> inline
ForwardIt MinElement(ForwardIt first, ForwardIt end, const Predicate& pred)
{
  ForwardIt result = first;
  for (; ++first != end;) {
    if (pred(*first, *result)) {
      result = first;
    }
  }
  return result;
}

/**
 * works just like std::max_element.
 */
template <typename ForwardIt> inline
ForwardIt MaxElement(ForwardIt first, ForwardIt end)
{
  ForwardIt result = first;
  for (; ++first != end;) {
    if (*result < *first) {
      result = first;
    }
  }
  return result;
}

/**
 * works just like std::max_element.
 */
template <typename ForwardIt, typename Predicate> inline
ForwardIt MaxElement(ForwardIt first, ForwardIt end, const Predicate& pred)
{
  ForwardIt result = first;
  for (; ++first != end;) {
    if (pred(*result, *first)) {
      result = first;
    }
  }
  return result;
}


/**
 * Utility template for a class that should not be copyable.
 * Derive from this class to make your class non-copyable
 */
class Noncopyable
{
 protected:
  /**
   * ensure the class cannot be constructed directly
   */
  Noncopyable() {}

  /**
   * the class should not be used polymorphically
   */
  ~Noncopyable() {}

 private:
  Noncopyable(const Noncopyable&);
  Noncopyable& operator = (const Noncopyable&);
};


/**
 * exception-safe guard around saving/restoring a value.
 * Commonly used to make sure a value is restored
 * even if the code early outs in the future.
 *
 * Usage:
 *      GuardValue<bool> guard_some_bool(some_bool_var, false); // Sets some_bool_var to false, and restores it in dtor.
 */
template <typename Type>
struct GuardValue : private Noncopyable
{
  GuardValue(Type& ref_value, const Type& new_value)
    : ref_value_(ref_value)
    , old_value_(ref_value)
  {
    ref_value_ = new_value;
  }

  ~GuardValue()
  {
    ref_value_ = old_value_;
  }

  /**
   * Overloaded dereference operator.
   * Provides read-only access to the original value of the data being tracked by this struct
   *
   * \return a const reference to the original data value
   */
  inline const Type& operator*() const
  {
    return old_value_;
  }

 private:
  Type& ref_value_;
  Type old_value_;
};


/**
 * Chooses between two different classes based on a boolean.
 */
template <bool pred, typename TrueType, typename FalseType>
struct Conditional;

template <typename TrueType, typename FalseType>
struct Conditional<true, TrueType, FalseType>
{
  typedef TrueType Result;
};

template <typename TrueType, typename FalseType>
struct Conditional<false, TrueType, FalseType>
{
  typedef FalseType Result;
};


/**
 * Helper class to make it easy to use key/value pairs with a container.
 */
template <typename KeyType, typename ValueType>
struct KeyValuePair
{
  KeyValuePair(const KeyType& key, const ValueType& value)
    : key(key)
    , value(value)
  {
  }

  KeyValuePair(const KeyType& key)
    : key(key)
  {
  }

  KeyValuePair()
  {
  }

  bool operator == (const KeyValuePair& other) const
  {
    return key == other.key;
  }

  bool operator != (const KeyValuePair& other) const
  {
    return key != other.key;
  }

  bool operator < (const KeyValuePair& other) const
  {
    return key < other.key;
  }

  inline bool operator()(const KeyValuePair& x, const KeyValuePair& y) const
  {
    return x.key < y.key;
  }

  KeyType key;
  ValueType value;
};


// Macros that can be used to specify multiple template parameters in a macro parameter.
// This is necessary to prevent the macro parsing from interpreting the template parameter
// delimiting comma as a macro parameter delimiter.

#define TEMPLATE_PARAMETERS2(X, Y)  X, Y


/**
 * Removes one level of pointer from a type, e.g.:
 *
 * RemovePointer<      int32  >::Type == int32
 * RemovePointer<      int32* >::Type == int32
 * RemovePointer<      int32**>::Type == int32*
 * RemovePointer<const int32* >::Type == const int32
 */
template <typename T> struct RemovePointer     { typedef T Type; };
template <typename T> struct RemovePointer<T*> { typedef T Type; };

/**
 * RemoveReference<type> will remove any references from a type.
 */
template <typename T> struct RemoveReference      { typedef T Type; };
template <typename T> struct RemoveReference<T& > { typedef T Type; };
template <typename T> struct RemoveReference<T&&> { typedef T Type; };

/**
 * MoveTemp will cast a reference to an rvalue reference.
 * This is FUN's equivalent of std::move.
 */
template <typename T>
inline typename RemoveReference<T>::Type&& MoveTemp(T&& obj)
{
  return (typename RemoveReference<T>::Type&&)obj;
}

/**
 * CopyTemp will enforce the creation of an rvalue which can bind to rvalue reference parameters.
 * Unlike MoveTemp, the source object will never be modifed. (i.e. a copy will be made)
 * There is no std:: equivalent.
 */
template <typename T>
inline T CopyTemp(T& val)
{
  return const_cast<const T&>(val);
}

template <typename T>
inline T CopyTemp(const T& val)
{
  return val;
}

template <typename T>
inline T&& CopyTemp(T&& val)
{
  // If we already have an rvalue, just return it unchanged, rather than
  // needlessly creating yet another rvalue from it.
  return MoveTemp(val);
}


/**
 * Forward will cast a reference to an rvalue reference.
 * This is FUN's equivalent of std::forward.
 */
template <typename T>
inline T&& Forward(typename RemoveReference<T>::Type& obj)
{
  return (T&&)obj;
}

template <typename T>
inline T&& Forward(typename RemoveReference<T>::Type&& obj)
{
  return (T&&)obj;
}


/**
 * a traits class which specifies whether a Swap of a given type should
 * swap the bits or use a traditional value-based swap.
 */
template <typename T>
struct UseBitwiseSwap
{
  enum { Value = !OrValue<__is_enum(T), IsPointer<T>, IsArithmetic<T>>::Value };
};

template <typename T>
inline typename EnableIf<UseBitwiseSwap<T>::Value>::Type Swap(T& x, T& y)
{
  TypeCompatibleStorage<T> tmp;
  UnsafeMemory::Memcpy(&tmp, &x, sizeof(T));
  UnsafeMemory::Memcpy(&x, &y, sizeof(T));
  UnsafeMemory::Memcpy(&y, &tmp, sizeof(T));
}

template <typename T>
inline typename EnableIf<!UseBitwiseSwap<T>::Value>::Type Swap(T& x, T& y)
{
  T tmp = MoveTemp(x);
  x = MoveTemp(y);
  y = MoveTemp(tmp);
}

/**
 * This exists to avoid a Visual Studio bug where using a cast to forward an rvalue reference array argument
 * to a pointer parameter will cause bad code generation.  Wrapping the cast in a function causes the correct
 * code to be generated.
 */
template <typename T, typename ArgType>
inline T StaticCast(ArgType&& arg)
{
  return static_cast<T>(arg);
}

/**
 * RValueToLValueReference converts any rvalue reference type into
 * the equivalent lvalue reference, otherwise returns the same type.
 */
template <typename T> struct RValueToLValueReference      { typedef T  Type; };
template <typename T> struct RValueToLValueReference<T&&> { typedef T& Type; };

/**
 * a traits class which tests if a type is a C++ array.
 */
template <typename T> struct IsCPPArray { enum { Value = false }; };
template <typename T, size_t N> struct IsCPPArray<T[N]> { enum { Value = true }; };

/**
 * Removes one dimension of extents from an array type.
 */
template <typename T> struct RemoveExtent { typedef T Type; };
template <typename T> struct RemoveExtent<T[]> { typedef T Type; };
template <typename T, size_t N> struct RemoveExtent<T[N]> { typedef T Type; };

/**
 * Returns the decayed type of T, meaning it removes all references, qualifiers and
 * applies array-to-pointer and function-to-pointer conversions.
 *
 * http://en.cppreference.com/w/cpp/types/decay
 */
template <typename T>
struct Decay
{
 private:
  typedef typename RemoveReference<T>::Type NoRefs;

 public:
  typedef typename Conditional<
    IsCPPArray<NoRefs>::Value,
    typename RemoveExtent<NoRefs>::Type*,
    typename Conditional<
      IsCppFunction<NoRefs>::Value,
      NoRefs*,
      typename RemoveCV<NoRefs>::Type
    >::Result
  >::Result Type;
};

/**
 * Reverses the order of the bits of a value.
 * This is an EnableIf'd template to ensure that no undesirable conversions occur.
 * Overloads for other types can be added in the same way.
 *
 * \param bits - The value to bit-swap.
 *
 * \return The bit-swapped value.
 */
template <typename T>
inline typename EnableIf<IsSame<T, uint32>::Value, T>::Type ReverseBits(T bits)
{
  bits = (bits << 16) | (bits >> 16);
  bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
  bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
  bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
  bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
  return bits;
}


/**
 * Template for initializing a singleton at the boot.
 */
template <typename T>
struct ForceInitAtBoot
{
  ForceInitAtBoot()
  {
    T::Get();
  }
};


/**
 * Used to avoid cluttering code with ifdefs.
 */
struct NoopStruct
{
  NoopStruct() {}
  ~NoopStruct() {}
};


/**
 * Copies the cv-qualifiers from one type to another, e.g.:
 *
 * CopyQualifiersFromTo<const T1, T2>::Type == const T2
 * CopyQualifiersFromTo<volatile T1, const T2>::Type == const volatile T2
 */
template <typename From, typename To> struct CopyQualifiersFromTo { typedef To Type; };
template <typename From, typename To> struct CopyQualifiersFromTo<const From, To> { typedef const To Type; };
template <typename From, typename To> struct CopyQualifiersFromTo<volatile From, To> { typedef volatile To Type; };
template <typename From, typename To> struct CopyQualifiersFromTo<const volatile From, To> { typedef const volatile To Type; };

/**
 * Tests if qualifiers are lost between one type and another, e.g.:
 *
 * CopyQualifiersFromTo<const T1, T2>::Value == true
 * CopyQualifiersFromTo<volatile T1, const volatile T2>::Value == false
 */
template <typename From, typename To>
struct LosesQualifiersFromTo
{
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
struct Identity
{
  typedef T Type;
};


/**
 * Equivalent to std::declval.
 *
 * Note that this function is unimplemented, and is only intended to be used in
 * unevaluated contexts, like sizeof and trait expressions.
 */
template <typename T>
T&& DeclVal();

} // namespace tl
} // namespace fun
