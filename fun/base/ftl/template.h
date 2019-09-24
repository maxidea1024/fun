//TODO 파일명을 utils.h로 변경하는게 좋을까??

#pragma once

#include "fun/base/base.h"
#include "fun/base/memory.h"
#include "fun/base/ftl/type_traits.h"
#include "fun/base/ftl/type_compatible_storage.h"

namespace fun {

/**
 * Chooses between the two parameters based on whether the first is nullptr or not.
 *
 * \return If the first parameter provided is non-nullptr, it is returned; otherwise the second parameter is returned.
 */
template <typename ReferencedType>
FUN_ALWAYS_INLINE ReferencedType*
IfAThenAElseB(ReferencedType* a, ReferencedType* b) {
  const intptr_t int_a = reinterpret_cast<intptr_t>(a);
  const intptr_t int_b = reinterpret_cast<intptr_t>(b);

  // Compute a mask which has all bits set if int_a is zero, and no bits set if it's non-zero.
  const intptr_t mask_b = -(!int_a);

  return reinterpret_cast<ReferencedType*>(int_a | (mask_b & int_b));
}

/**
 * Branchless pointer selection based on predicate
 * return intptr_t(pred) ? a : b;
 */
template <typename Predicate, typename ReferencedType>
FUN_ALWAYS_INLINE ReferencedType*
IfPThenAElseB(const Predicate& pred, ReferencedType* a, ReferencedType* b) {
  const intptr_t int_a = reinterpret_cast<intptr_t>(a);
  const intptr_t int_b = reinterpret_cast<intptr_t>(b);

  // Compute a mask which has all bits set if pred is zero, and no bits set if it's non-zero.
  const intptr_t mask_b = -(!intptr_t(pred));

  return reinterpret_cast<ReferencedType*>((int_a & ~mask_b) | (int_b & mask_b));
}

/**
 * a logical exclusive or function.
 */
FUN_ALWAYS_INLINE bool XOR(bool a, bool b) {
  return a != b;
}

/**
 * This is used to provide type specific behavior for a copy which cannot change the value of b.
 */
template <typename T>
FUN_ALWAYS_INLINE void Move(T& a, typename MoveSupportTraits<T>::Copy b) {
  // Destruct the previous value of a.
  a.~T();

  // Use placement new and a copy constructor so types with const members will work.
  new(&a) T(b);
}

/**
 * This is used to provide type specific behavior for a move which may change the value of b.
 */
template <typename T>
FUN_ALWAYS_INLINE void Move(T& a, typename MoveSupportTraits<T>::Move b) {
  // Destruct the previous value of a.
  a.~T();

  // Use placement new and a copy constructor so types with const members will work.
  new(&a) T(MoveTemp(b));
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
static FUN_ALWAYS_INLINE bool
IsValidArrayIndex(const T(&array)[N], size_t index) {
  return index < N;
}


//
// Standard macros.
//

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


/**
 * Utility template for a class that should not be copyable.
 * Derive from this class to make your class non-copyable
 */
class Noncopyable {
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
  Noncopyable(const Noncopyable&) = delete;
  Noncopyable& operator = (const Noncopyable&) = delete;
};


/**
 * exception-safe guard around saving/restoring a value.
 * Commonly used to make sure a value is restored
 * even if the code early outs in the future.
 *
 * Usage:
 *      GuardValue<bool> GuardSomeBool(some_bool_var, false); // Sets some_bool_var to false, and restores it in dtor.
 */
template <typename T>
struct GuardValue : private Noncopyable {
  GuardValue(T& ref_value, const T& new_value)
    : ref_value_(ref_value),
      old_value_(ref_value) {
    ref_value_ = new_value;
  }

  ~GuardValue() {
    ref_value_ = old_value_;
  }

  /**
   * Overloaded dereference operator.
   * Provides read-only access to the original value of the data
   * being tracked by this struct
   *
   * \return a const reference to the original data value
   */
  FUN_ALWAYS_INLINE const T& operator * () const {
    return old_value_;
  }

 private:
  T& ref_value_;
  T old_value_;
};


/**
 * Helper class to make it easy to use key/value pairs with a container.
 */
template <typename KeyType, typename ValueType>
struct KeyValuePair {
  KeyValuePair(const KeyType& key, const ValueType& value)
    : key(key),
      value(value) {}

  KeyValuePair(const KeyType& key)
    : key(key) {}

  KeyValuePair() {}

  bool operator == (const KeyValuePair& other) const {
    return key == other.key;
  }

  bool operator != (const KeyValuePair& other) const {
    return key != other.key;
  }

  bool operator < (const KeyValuePair& other) const {
    return key < other.key;
  }

  FUN_ALWAYS_INLINE bool operator()(const KeyValuePair& x, const KeyValuePair& y) const {
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
 * MoveTemp will cast a reference to an rvalue reference.
 * This is FUN's equivalent of std::move.
 */
template <typename T>
FUN_ALWAYS_INLINE typename RemoveReference<T>::Type&&
MoveTemp(T&& obj) {
  typedef typename RemoveReference<T>::Type CastType;

  // Validate that we're not being passed an rvalue or a const object
  // - the former is redundant, the latter is almost certainly a mistake
  static_assert(IsLValueReferenceType<T>::Value, "MoveTemp called on an rvalue");
  static_assert(!IsSame<CastType&, const CastType&>::Value, "MoveTemp called on a const object");

  return (CastType&&)obj;
}

/**
 * MoveTemp will cast a reference to an rvalue reference.
 * This is FUN's equivalent of std::move.
 * It doesn't static assert like MoveTemp, because it is useful in
 * templates or macros where it's not obvious what the argument is,
 * but you want to take advantage of move semantics
 * where you can but not stop compilation.
 */
template <typename T>
FUN_ALWAYS_INLINE typename RemoveReference<T>::Type&&
MoveTempIfPossible(T&& obj) {
  typedef typename RemoveReference<T>::Type CastType;
  return (CastType&&)obj;
}

/**
 * CopyTemp will enforce the creation of an rvalue which can bind to
 * rvalue reference parameters.
 *
 * Unlike MoveTemp, the source object will never be modifed.
 * (i.e. a copy will be made)
 * There is no std:: equivalent.
 */
template <typename T>
FUN_ALWAYS_INLINE T CopyTemp(T& val) {
  return const_cast<const T&>(val);
}

template <typename T>
FUN_ALWAYS_INLINE T CopyTemp(const T& val) {
  return val;
}

template <typename T>
FUN_ALWAYS_INLINE T&& CopyTemp(T&& val) {
  // If we already have an rvalue, just return it unchanged, rather than
  // needlessly creating yet another rvalue from it.
  return MoveTemp(val);
}

/**
 * Forward will cast a reference to an rvalue reference.
 * This is FUN's equivalent of std::forward.
 */
template <typename T>
FUN_ALWAYS_INLINE T&& Forward(typename RemoveReference<T>::Type& obj) {
  return (T&&)obj;
}

template <typename T>
FUN_ALWAYS_INLINE T&& Forward(typename RemoveReference<T>::Type&& obj) {
  return (T&&)obj;
}

/**
 * a traits class which specifies whether a Swap of a given type should
 * swap the bits or use a traditional value-based swap.
 */
template <typename T>
struct UseBitwiseSwap {
  enum { Value = !OrValue<__is_enum(T), IsPointer<T>, IsArithmetic<T>>::Value };
};

template <typename T>
FUN_ALWAYS_INLINE typename EnableIf<UseBitwiseSwap<T>::Value>::Type
Swap(T& x, T& y) {
  TypeCompatibleStorage<T> tmp;
  UnsafeMemory::Memcpy(&tmp, &x, sizeof(T));
  UnsafeMemory::Memcpy(&x, &y, sizeof(T));
  UnsafeMemory::Memcpy(&y, &tmp, sizeof(T));
}

template <typename T>
FUN_ALWAYS_INLINE typename EnableIf<!UseBitwiseSwap<T>::Value>::Type
Swap(T& x, T& y) {
  T tmp = MoveTemp(x);
  x = MoveTemp(y);
  y = MoveTemp(tmp);
}

/**
 * This exists to avoid a Visual Studio bug where using a cast to
 * forward an rvalue reference array argument
 * to a pointer parameter will cause bad code generation.
 * Wrapping the cast in a function causes the correct
 * code to be generated.
 */
template <typename T, typename ArgType>
FUN_ALWAYS_INLINE T StaticCast(ArgType&& arg) {
  return static_cast<T>(arg);
}

/**
 * Uses implicit conversion to create an instance of a specific type.
 * Useful to make things clearer or circumvent unintended type deduction in templates.
 * Safer than C casts and static_casts, e.g. does not allow down-casts
 *
 * @param obj - The object (usually pointer or reference) to convert.
 *
 * @return The object converted to the specified type.
 */
template <typename T>
FUN_ALWAYS_INLINE T ImplicitConv(typename Identity<T>::Type obj) {
  return obj;
}

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
FUN_ALWAYS_INLINE typename EnableIf<IsSame<T, uint32>::Value, T>::Type
ReverseBits(T bits) {
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
struct ForceInitAtBoot {
  ForceInitAtBoot() {
    T::Get();
  }
};

/**
 * Used to avoid cluttering code with ifdefs.
 */
struct NoopStruct {
  NoopStruct() {}
  ~NoopStruct() {}
};

} // namespace fun
