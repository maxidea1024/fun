#pragma once

#include "fun/base/base.h"
#include "fun/base/format.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/ordered_map.h"
#include "fun/base/ordered_set.h"
#include "fun/base/dynamic/var_holder.h"
#include "fun/base/dynamic/var_iterator.h"

#include <typeinfo>
#include <map>
#include <set>

namespace fun {
namespace dynamic {

template <typename K, typename M, typename S>
class Struct;

/**
 * Var allows to store data of different types and to Convert between these types transparently.
 * Var puts forth the best effort to provide intuitive and reasonable conversion semantics and prevent
 * unexpected data loss, particularly when performing narrowing or signedness conversions of numeric data types.
 *
 * An attempt to Convert or Extract from a non-initialized ("empty") Var variable shall result
 * in an exception being thrown.
 *
 * Loss of signedness is not allowed for numeric values. This means that if an attempt is made to Convert
 * the internal value which is a negative signed integer to an unsigned integer type storage, a RangeException is thrown.
 * Overflow is not allowed, so if the internal value is a larger number than the target numeric type size can accomodate,
 * a RangeException is thrown.
 *
 * Precision loss, such as in conversion from floating-point types to integers or from double to float on platforms
 * where they differ in size (provided internal actual value fits in float min/max range), is allowed.
 *
 * String truncation is allowed -- it is possible to Convert between string and character when string length is
 * greater than 1. An empty string gets converted to the char '\0', a non-empty string is truncated to the first character.
 *
 * Boolean conversion is performed as follows:
 *
 * A string value "false" (not case sensitive), "0" or "" (empty string) can be converted to a boolean value false,
 * any other string not being false by the above criteria evaluates to true (e.g: "hi" -> true).
 * Integer 0 values are false, everything else is true.
 * Floating point values equal to the minimal FP representation on a given platform are false, everything else is true.
 *
 * Arithmetic operations with POD types as well as between Var's are supported, subject to following
 * limitations:
 *
 *  - for String and const char* values, only '+' and '+=' operations are supported
 *
 *  - for integral and floating point numeric values, following operations are supported:
 *    '+', '+=', '-', '-=', '*', '*=' , '/' and '/='
 *
 *  - for integral values, following operations are supported:
 *      prefix and postfix Increment (++) and Decrement (--)
 *
 *  - for all other types, InvalidArgumentException is thrown upon attempt of an arithmetic operation
 *
 * A Var can be created from and converted to a value of any type for which a specialization of
 * VarHolderImpl is available. For supported types, see VarHolder documentation.
 */
class FUN_BASE_API Var {
 public:
  typedef SharedPtr<Var> Ptr;
  typedef VarIterator Iterator;
  typedef const VarIterator ConstIterator;

  /**
   * Creates an empty Var.
   */
  Var();

  /**
   * Creates the Var from the given value.
   */
  template <typename T>
  Var(const T& val)
#ifdef FUN_NO_SOO
    : holder_(new VarHolderImpl<T>(val))
  {
  }
#else
  {
    Construct(val);
  }
#endif

  // Convenience constructor for const char* which gets mapped to a String internally, i.e. val is deep-copied.
  Var(const char* val);

  /**
   * Copy constructor.
   */
  Var(const Var& other);

  /**
   * Destroys the Var.
   */
  ~Var();

  /**
   * Swaps the content of the this Var with the other Var.
   */
  void Swap(Var& other);

  /**
   * Returns the const Var iterator.
   */
  ConstIterator begin() const;

  /**
   * Returns the const Var iterator.
   */
  ConstIterator end() const;

  /**
   * Returns the Var iterator.
   */
  Iterator begin();

  /**
   * Returns the Var iterator.
   */
  Iterator end();

  /**
   * Invoke this method to perform a safe conversion.
   *
   * Example usage:
   *     Var any("42");
   *     int i;
   *     any.Convert(i);
   *
   * Throws a RangeException if the value does not fit
   * into the result variable.
   * Throws a NotImplementedException if conversion is
   * not available for the given type.
   * Throws InvalidAccessException if Var is empty.
   */
  template <typename T>
  void Convert(T& val) const {
    VarHolder* holder = GetContent();
    if (!holder) {
      throw InvalidAccessException("Can not convert empty value.");
    }

    holder->Convert(val);
  }

  /**
   * Invoke this method to perform a safe conversion.
   *
   * Example usage:
   *     Var any("42");
   *     int i = any.Convert<int>();
   *
   * Throws a RangeException if the value does not fit
   * into the result variable.
   * Throws a NotImplementedException if conversion is
   * not available for the given type.
   * Throws InvalidAccessException if Var is empty.
   */
  template <typename T>
  T Convert() const {
    VarHolder* holder = GetContent();
    if (!holder) {
      throw InvalidAccessException("Can not convert empty value.");
    }

    // 타입이 정확히 일치할 경우에는 conversion이 아닌,
    // extraction으로 가볍게 처리.
    if (typeid(T) == holder->Type()) {
      return Extract<T>();
    }

    T result;
    holder->Convert(result);
    return result;
  }

  /**
   * Safe conversion operator for implicit type
   * conversions. If the requested type T is same as the
   * type being held, the operation performed is direct
   * extraction, otherwise it is the conversion of the value
   * from type currently held to the one requested.
   *
   * Throws a RangeException if the value does not fit
   * into the result variable.
   * Throws a NotImplementedException if conversion is
   * not available for the given type.
   * Throws InvalidAccessException if Var is empty.
   */
  template <typename T>
  operator T () const {
    VarHolder* holder = GetContent();
    if (!holder) {
      throw InvalidAccessException("Can not convert empty value.");
    }

    if (typeid(T) == holder->Type()) {
      return Extract<T>();
    } else {
      T result;
      holder->Convert(result);
      return result;
    }
  }

  /**
   * Returns a const reference to the actual value.
   *
   * Must be instantiated with the exact type of
   * the stored value, otherwise a BadCastException
   * is thrown.
   * Throws InvalidAccessException if Var is empty.
   */
  template <typename T>
  const T& Extract() const {
    VarHolder* holder = GetContent();

    if (holder && holder->Type() == typeid(T)) {
      VarHolderImpl<T>* holder_impl = static_cast<VarHolderImpl<T>*>(holder);
      return holder_impl->Value();
    } else if (!holder) {
      throw InvalidAccessException("Can not extract empty value.");
    } else {
      throw BadCastException(fun::Format("Can not convert %s to %s.",
                            String(holder->Type().name()),
                            String(typeid(T).name())));
    }
  }

  /**
   * Assignment operator for assigning POD to Var
   */
  template <typename T>
  Var& operator = (const T& other) {
#ifdef FUN_NO_SOO
    Var tmp(other);
    Swap(tmp);
#else
    Construct(other);
#endif
    return *this;
  }

  /**
   * Logical NOT operator.
   */
  bool operator ! () const;

  /**
   * Assignment operator specialization for Var
   */
  Var& operator = (const Var& other);

  /**
   * Addition operator for adding POD to Var
   */
  template <typename T>
  const Var operator + (const T& other) const {
    return Convert<T>() + other;
  }

  /**
   * Addition operator specialization for Var
   */
  const Var operator + (const Var& other) const;

  /**
   * Addition operator specialization for adding const char* to Var
   */
  const Var operator + (const char* other) const;

  /**
   * Pre-Increment operator
   */
  Var& operator ++ ();

  /**
   * Post-Increment operator
   */
  const Var operator ++ (int);

  /**
   * Pre-Decrement operator
   */
  Var& operator -- ();

  /**
   * Post-Decrement operator
   */
  const Var operator -- (int);

  /**
   * Addition asignment operator for addition/assignment of POD to Var.
   */
  template <typename T>
  Var& operator += (const T& other) {
    return *this = Convert<T>() + other;
  }

  /**
   * Addition asignment operator overload for Var
   */
  Var& operator += (const Var& other);

  /**
   * Addition asignment operator overload for const char*
   */
  Var& operator += (const char* other);

  /**
   * Subtraction operator for subtracting POD from Var
   */
  template <typename T>
  const Var operator - (const T& other) const {
    return Convert<T>() - other;
  }

  /**
   * Subtraction operator overload for Var
   */
  const Var operator - (const Var& other) const;

  /**
   * Subtraction asignment operator
   */
  template <typename T>
  Var& operator -= (const T& other) {
    return *this = Convert<T>() - other;
  }

  /**
   * Subtraction asignment operator overload for Var
   */
  Var& operator -= (const Var& other);

  /**
   * Multiplication operator for multiplying Var with POD
   */
  template <typename T>
  const Var operator * (const T& other) const {
    return Convert<T>() * other;
  }

  /**
   * Multiplication operator overload for Var
   */
  const Var operator * (const Var& other) const;

  /**
   * Multiplication asignment operator
   */
  template <typename T>
  Var& operator *= (const T& other) {
    return *this = Convert<T>() * other;
  }

  /**
   * Multiplication asignment operator overload for Var
   */
  Var& operator *= (const Var& other);

  /**
   * Division operator for dividing Var with POD
   */
  template <typename T>
  const Var operator / (const T& other) const {
    return Convert<T>() / other;
  }

  /**
   * Division operator overload for Var
   */
  const Var operator / (const Var& other) const;

  /**
   * Division asignment operator
   */
  template <typename T>
  Var& operator /= (const T& other) {
    return *this = Convert<T>() / other;
  }

  /**
   * Division asignment operator specialization for Var
   */
  Var& operator /= (const Var& other);

  /**
   * Equality operator
   */
  template <typename T>
  bool operator == (const T& other) const {
    if (IsEmpty()) {
      return false;
    }

    return Convert<T>() == other;
  }

  /**
   * Equality operator overload for const char*
   */
  bool operator == (const char* other) const;

  /**
   * Equality operator overload for Var
   */
  bool operator == (const Var& other) const;

  /**
   * Inequality operator
   */
  template <typename T>
  bool operator != (const T& other) const {
    if (IsEmpty()) {
      return true;
    }

    return Convert<T>() != other;
  }

  /**
   * Inequality operator overload for Var
   */
  bool operator != (const Var& other) const;

  /**
   * Inequality operator overload for const char*
   */
  bool operator != (const char* other) const;

  /**
   * Less than operator
   */
  template <typename T>
  bool operator < (const T& other) const {
    if (IsEmpty()) {
      return false;
    }

    return Convert<T>() < other;
  }

  /**
   * Less than operator overload for Var
   */
  bool operator < (const Var& other) const;

  /**
   * Less than or equal operator
   */
  template <typename T>
  bool operator <= (const T& other) const {
    if (IsEmpty()) {
      return false;
    }

    return Convert<T>() <= other;
  }

  /**
   * Less than or equal operator overload for Var
   */
  bool operator <= (const Var& other) const;

  /**
   * Greater than operator
   */
  template <typename T>
  bool operator > (const T& other) const {
    if (IsEmpty()) {
      return false;
    }

    return Convert<T>() > other;
  }

  /**
   * Greater than operator overload for Var
   */
  bool operator > (const Var& other) const;

  /**
   * Greater than or equal operator
   */
  template <typename T>
  bool operator >= (const T& other) const {
    if (IsEmpty()) {
      return false;
    }

    return Convert<T>() >= other;
  }

  /**
   * Greater than or equal operator overload for Var
   */
  bool operator >= (const Var& other) const;

  /**
   * Logical OR operator
   */
  template <typename T>
  bool operator || (const T& other) const {
    if (IsEmpty()) {
      return false;
    }

    return Convert<bool>() || other;
  }

  /**
   * Logical OR operator operator overload for Var
   */
  bool operator || (const Var& other) const;

  /**
   * Logical AND operator.
   */
  template <typename T>
  bool operator && (const T& other) const {
    if (IsEmpty()) {
      return false;
    }

    return Convert<bool>() && other;
  }

  /**
   * Logical AND operator operator overload for Var.
   */
  bool operator && (const Var& other) const;

  /**
   * Returns true if Var is an array.
   */
  bool IsArray() const;

  /**
   * Returns true if Var represents a vector.
   */
  bool IsVector() const;

  /**
   * Returns true if Var represents a list.
   */
  bool IsList() const;

  /**
   * Returns true if Var represents a deque.
   */
  bool IsDeque() const;

  /**
   * Returns true if Var represents a struct.
   */
  bool IsStruct() const;

  /**
   * Returns true if Var represents an ordered struct,
   * false if struct is sorted.
   */
  bool IsOrdered() const;

  /**
   * Returns character at position n. This function only works with
   * Var containing a String.
   */
  char& At(size_t n);

  template <typename T>
  Var& operator [] (const T& n) {
    return GetAt(n);
  }

  template <typename T>
  const Var& operator [] (const T& n) const {
    return const_cast<Var*>(this)->GetAt(n);
  }

  /**
   * Index operator by name, only use on Vars where IsStruct
   * returns true! In all other cases InvalidAccessException is thrown.
   */
  Var& operator [] (const String& name);

  /**
   * Index operator by name, only use on Vars where IsStruct
   * returns true! In all other cases InvalidAccessException is thrown.
   */
  const Var& operator [] (const String& name) const;

  /**
   * Returns the type information of the stored content.
   */
  const std::type_info& Type() const;

  /**
   * Empties Var.
   */
  void Clear();

  /**
   * Returns true if empty.
   */
  bool IsEmpty() const;

  /**
   * Returns true if stored value is integer.
   */
  bool IsInteger() const;

  /**
   * Returns true if stored value is signed.
   */
  bool IsSigned() const;

  /**
   * Returns true if stored value is numeric.
   * Returns false for numeric strings (e.g. "123" is string, not number)
   */
  bool IsNumeric() const;

  /**
   * Returns true if stored value is boolean.
   * Returns false for boolean strings (e.g. "true" is string, not number)
   */
  bool IsBoolean() const;

  /**
   * Returns true if stored value is String.
   */
  bool IsString() const;

  /**
   * Returns true if stored value represents a date.
   */
  bool IsDate() const;

  /**
   * Returns true if stored value represents time or date/time.
   */
  bool IsTime() const;

  /**
   * Returns true if stored value represents a date/time.
   */
  bool IsDateTime() const;

  /**
   * Returns the size of this Var.
   * This function returns 0 when Var is empty, 1 for POD or the size (i.e. length)
   * for held container.
   */
  size_t size() const;

  /**
   * Returns the stored value as string.
   */
  String ToString() const {
    VarHolder* holder = GetContent();

    if (!holder) {
      throw InvalidAccessException("Can not convert empty value.");
    }

    if (typeid(String) == holder->Type()) {
      return Extract<String>();
    } else {
      String result;
      holder->Convert(result);
      return result;
    }
  }

  /**
   * Parses the string which must be in JSON format
   */
  static Var Parse(const String& val);

  /**
   * Converts the Var to a string in JSON format. Note that ToString(const Var&) will return
   * a different result than Var::Convert<String>() and Var::ToString()!
   */
  static String ToString(const Var& var);

 private:
  Var& GetAt(size_t n);
  Var& GetAt(const String& n);

  /**
   * Parses the string which must be in JSON format
   */
  static Var Parse(const String& val, String::size_type& offset);

  static Var ParseObject(const String& val, String::size_type& pos);
  static Var ParseArray(const String& val, String::size_type& pos);
  static String ParseString(const String& val, String::size_type& pos);
  static String ParseJsonString(const String& val, String::size_type& pos);
  static void SkipWhitespaces(const String& val, String::size_type& pos);

  template <typename T>
  T Add(const Var& other) const {
    return Convert<T>() + other.Convert<T>();
  }

  template <typename T>
  T Subtract(const Var& other) const {
    return Convert<T>() - other.Convert<T>();
  }

  template <typename T>
  T Multiply(const Var& other) const {
    return Convert<T>() * other.Convert<T>();
  }

  template <typename T>
  T Divide(const Var& other) const {
    return Convert<T>() / other.Convert<T>();
  }

  template <typename T, typename E>
  VarHolderImpl<T>* HolderImpl(const String error_message = "") const {
    VarHolder* holder = GetContent();

    if (holder && holder->Type() == typeid(T)) {
      return static_cast<VarHolderImpl<T>*>(holder);
    } else if (!holder) {
      throw InvalidAccessException("Can not access empty value.");
    } else {
      throw E(error_message);
    }
  }

  template <typename T, typename N>
  Var& StructIndexOperator(T* pStr, N n) const {
    return pStr->operator[](n);
  }

#ifdef FUN_NO_SOO

  VarHolder* GetContent() const {
    return holder_;
  }

  void Destruct() {
    if (!IsEmpty()) {
      delete GetContent();
    }
  }

  VarHolder* holder_;

#else

  VarHolder* GetContent() const {
    return placeholder_.GetContent();
  }

  template<typename ValueType>
  void Construct(const ValueType& value) {
    if (sizeof(VarHolderImpl<ValueType>) <= Placeholder<ValueType>::Size::Value) {
      new(reinterpret_cast<VarHolder*>(placeholder_.holder)) VarHolderImpl<ValueType>(value);
      placeholder_.SetLocal(true);
    } else {
      placeholder_.holder = new VarHolderImpl<ValueType>(value);
      placeholder_.SetLocal(false);
    }
  }

  void Construct(const char* value) {
    String val(value);
    if (sizeof(VarHolderImpl<String>) <= Placeholder<String>::Size::Value) {
      new(reinterpret_cast<VarHolder*>(placeholder_.holder)) VarHolderImpl<String>(val);
      placeholder_.SetLocal(true);
    } else {
      placeholder_.holder = new VarHolderImpl<String>(val);
      placeholder_.SetLocal(false);
    }
  }

  void Construct(const Var& other) {
    if (!other.IsEmpty()) {
      other.GetContent()->Clone(&placeholder_);
    } else {
      placeholder_.Clear();
    }
  }

  void Destruct() {
    if (!IsEmpty()) {
      if (placeholder_.IsLocal()) {
        GetContent()->~VarHolder();
      } else {
        delete GetContent();
      }
    }
  }

  Placeholder<VarHolder> placeholder_;

#endif // FUN_NO_SOO
};


//
// inlines
//


//
// Var members
//

inline void Var::Swap(Var& other) {
#ifdef FUN_NO_SOO

  fun::Swap(holder_, other.holder_);

#else

  if (this == &other) {
    return;
  }

  if (!placeholder_.IsLocal() && !other.placeholder_.IsLocal()) {
    fun::Swap(placeholder_.holder, other.placeholder_.holder);
  } else {
    Var tmp(*this);
    try {
      if (placeholder_.IsLocal()) {
        Destruct();
      }
      Construct(other);
      other = tmp;
    } catch (...) {
      Construct(tmp);
      throw;
    }
  }

#endif
}

inline const std::type_info& Var::Type() const {
  VarHolder* holder = GetContent();
  return holder ? holder->Type() : typeid(void);
}

inline Var::ConstIterator Var::begin() const {
  if (IsEmpty()) {
    return ConstIterator(const_cast<Var*>(this), true);
  }

  return ConstIterator(const_cast<Var*>(this), false);
}

inline Var::ConstIterator Var::end() const {
  return ConstIterator(const_cast<Var*>(this), true);
}

inline Var::Iterator Var::begin() {
  if (IsEmpty()) {
    return Iterator(const_cast<Var*>(this), true);
  }

  return Iterator(const_cast<Var*>(this), false);
}

inline Var::Iterator Var::end() {
  return Iterator(this, true);
}

inline Var& Var::operator [] (const String& name) {
  return GetAt(name);
}

inline const Var& Var::operator [] (const String& name) const {
  return const_cast<Var*>(this)->GetAt(name);
}

inline const Var Var::operator + (const char* other) const {
  return Convert<String>() + other;
}

inline Var& Var::operator += (const char* other) {
  return *this = Convert<String>() + other;
}

inline bool Var::operator ! () const {
  return !Convert<bool>();
}

inline bool Var::IsEmpty() const {
  return 0 == GetContent();
}

inline bool Var::IsArray() const {
  if (IsEmpty() || IsString()) {
      return false;
  }

  VarHolder* holder = GetContent();
  return holder ? holder->IsArray() : false;
}

inline bool Var::IsVector() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsVector() : false;
}

inline bool Var::IsList() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsList() : false;
}

inline bool Var::IsDeque() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsDeque() : false;
}

inline bool Var::IsStruct() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsStruct() : false;
}

inline bool Var::IsOrdered() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsOrdered() : false;
}

inline bool Var::IsInteger() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsInteger() : false;
}

inline bool Var::IsSigned() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsSigned() : false;
}

inline bool Var::IsNumeric() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsNumeric() : false;
}

inline bool Var::IsBoolean() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsBoolean() : false;
}

inline bool Var::IsString() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsString() : false;
}

inline bool Var::IsDate() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsDate() : false;
}

inline bool Var::IsTime() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsTime() : false;
}

inline bool Var::IsDateTime() const {
  VarHolder* holder = GetContent();
  return holder ? holder->IsDateTime() : false;
}

inline size_t Var::size() const {
  VarHolder* holder = GetContent();
  return holder ? holder->size() : 0;
}


//
// Var non-member functions
//

inline const Var operator + (const char* other, const Var& da) {
  String tmp = other;
  return tmp + da.Convert<String>();
}

inline char operator + (const char& other, const Var& da) {
  return other + da.Convert<char>();
}

inline char operator - (const char& other, const Var& da) {
  return other - da.Convert<char>();
}

inline char operator * (const char& other, const Var& da) {
  return other * da.Convert<char>();
}

inline char operator / (const char& other, const Var& da) {
  return other / da.Convert<char>();
}

inline char operator += (char& other, const Var& da) {
  return other += da.Convert<char>();
}

inline char operator -= (char& other, const Var& da) {
  return other -= da.Convert<char>();
}

inline char operator *= (char& other, const Var& da) {
  return other *= da.Convert<char>();
}

inline char operator /= (char& other, const Var& da) {
  return other /= da.Convert<char>();
}

inline bool operator == (const char& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<char>();
}

inline bool operator != (const char& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<char>();
}

inline bool operator < (const char& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<char>();
}

inline bool operator <= (const char& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<char>();
}

inline bool operator > (const char& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<char>();
}

inline bool operator >= (const char& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<char>();
}

inline fun::int8 operator + (const fun::int8& other, const Var& da) {
  return other + da.Convert<fun::int8>();
}

inline fun::int8 operator - (const fun::int8& other, const Var& da) {
  return other - da.Convert<fun::int8>();
}

inline fun::int8 operator * (const fun::int8& other, const Var& da) {
  return other * da.Convert<fun::int8>();
}

inline fun::int8 operator / (const fun::int8& other, const Var& da) {
  return other / da.Convert<fun::int8>();
}

inline fun::int8 operator += (fun::int8& other, const Var& da) {
  return other += da.Convert<fun::int8>();
}

inline fun::int8 operator -= (fun::int8& other, const Var& da) {
  return other -= da.Convert<fun::int8>();
}

inline fun::int8 operator *= (fun::int8& other, const Var& da) {
  return other *= da.Convert<fun::int8>();
}

inline fun::int8 operator /= (fun::int8& other, const Var& da) {
  return other /= da.Convert<fun::int8>();
}

inline bool operator == (const fun::int8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<fun::int8>();
}

inline bool operator != (const fun::int8& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<fun::int8>();
}

inline bool operator < (const fun::int8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<fun::int8>();
}

inline bool operator <= (const fun::int8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<fun::int8>();
}

inline bool operator > (const fun::int8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<fun::int8>();
}

inline bool operator >= (const fun::int8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<fun::int8>();
}

inline fun::uint8 operator + (const fun::uint8& other, const Var& da) {
  return other + da.Convert<fun::uint8>();
}

inline fun::uint8 operator - (const fun::uint8& other, const Var& da) {
  return other - da.Convert<fun::uint8>();
}

inline fun::uint8 operator * (const fun::uint8& other, const Var& da) {
  return other * da.Convert<fun::uint8>();
}

inline fun::uint8 operator / (const fun::uint8& other, const Var& da) {
  return other / da.Convert<fun::uint8>();
}

inline fun::uint8 operator += (fun::uint8& other, const Var& da) {
  return other += da.Convert<fun::uint8>();
}

inline fun::uint8 operator -= (fun::uint8& other, const Var& da) {
  return other -= da.Convert<fun::uint8>();
}

inline fun::uint8 operator *= (fun::uint8& other, const Var& da) {
  return other *= da.Convert<fun::uint8>();
}

inline fun::uint8 operator /= (fun::uint8& other, const Var& da) {
  return other /= da.Convert<fun::uint8>();
}

inline bool operator == (const fun::uint8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<fun::uint8>();
}

inline bool operator != (const fun::uint8& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<fun::uint8>();
}

inline bool operator < (const fun::uint8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<fun::uint8>();
}

inline bool operator <= (const fun::uint8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<fun::uint8>();
}

inline bool operator > (const fun::uint8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<fun::uint8>();
}

inline bool operator >= (const fun::uint8& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<fun::uint8>();
}

inline fun::int16 operator + (const fun::int16& other, const Var& da) {
  return other + da.Convert<fun::int16>();
}

inline fun::int16 operator - (const fun::int16& other, const Var& da) {
  return other - da.Convert<fun::int16>();
}

inline fun::int16 operator * (const fun::int16& other, const Var& da) {
  return other * da.Convert<fun::int16>();
}

inline fun::int16 operator / (const fun::int16& other, const Var& da) {
  return other / da.Convert<fun::int16>();
}

inline fun::int16 operator += (fun::int16& other, const Var& da) {
  return other += da.Convert<fun::int16>();
}

inline fun::int16 operator -= (fun::int16& other, const Var& da) {
  return other -= da.Convert<fun::int16>();
}

inline fun::int16 operator *= (fun::int16& other, const Var& da) {
  return other *= da.Convert<fun::int16>();
}

inline fun::int16 operator /= (fun::int16& other, const Var& da) {
  return other /= da.Convert<fun::int16>();
}

inline bool operator == (const fun::int16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<fun::int16>();
}

inline bool operator != (const fun::int16& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<fun::int16>();
}

inline bool operator < (const fun::int16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<fun::int16>();
}

inline bool operator <= (const fun::int16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<fun::int16>();
}

inline bool operator > (const fun::int16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<fun::int16>();
}

inline bool operator >= (const fun::int16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<fun::int16>();
}

inline uint16 operator + (const uint16& other, const Var& da) {
  return other + da.Convert<uint16>();
}

inline uint16 operator - (const uint16& other, const Var& da) {
  return other - da.Convert<uint16>();
}

inline uint16 operator * (const uint16& other, const Var& da) {
  return other * da.Convert<uint16>();
}

inline uint16 operator / (const uint16& other, const Var& da) {
  return other / da.Convert<uint16>();
}

inline uint16 operator += (uint16& other, const Var& da) {
  return other += da.Convert<uint16>();
}

inline uint16 operator -= (uint16& other, const Var& da) {
  return other -= da.Convert<uint16>();
}

inline uint16 operator *= (uint16& other, const Var& da) {
  return other *= da.Convert<uint16>();
}

inline uint16 operator /= (uint16& other, const Var& da) {
  return other /= da.Convert<uint16>();
}

inline bool operator == (const uint16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<uint16>();
}

inline bool operator != (const uint16& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<uint16>();
}

inline bool operator < (const uint16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<uint16>();
}

inline bool operator <= (const uint16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<uint16>();
}

inline bool operator > (const uint16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<uint16>();
}

inline bool operator >= (const uint16& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<uint16>();
}

inline int32 operator + (const int32& other, const Var& da) {
  return other + da.Convert<int32>();
}

inline int32 operator - (const int32& other, const Var& da) {
  return other - da.Convert<int32>();
}

inline int32 operator * (const int32& other, const Var& da) {
  return other * da.Convert<int32>();
}

inline int32 operator / (const int32& other, const Var& da) {
  return other / da.Convert<int32>();
}

inline int32 operator += (int32& other, const Var& da) {
  return other += da.Convert<int32>();
}

inline int32 operator -= (int32& other, const Var& da) {
  return other -= da.Convert<int32>();
}

inline int32 operator *= (int32& other, const Var& da) {
  return other *= da.Convert<int32>();
}

inline int32 operator /= (int32& other, const Var& da) {
  return other /= da.Convert<int32>();
}

inline bool operator == (const int32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<int32>();
}

inline bool operator != (const int32& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<int32>();
}

inline bool operator < (const int32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<int32>();
}

inline bool operator <= (const int32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<int32>();
}

inline bool operator > (const int32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<int32>();
}

inline bool operator >= (const int32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<int32>();
}

inline uint32 operator + (const uint32& other, const Var& da) {
  return other + da.Convert<uint32>();
}

inline uint32 operator - (const uint32& other, const Var& da) {
  return other - da.Convert<uint32>();
}

inline uint32 operator * (const uint32& other, const Var& da) {
  return other * da.Convert<uint32>();
}

inline uint32 operator / (const uint32& other, const Var& da) {
  return other / da.Convert<uint32>();
}

inline uint32 operator += (uint32& other, const Var& da) {
  return other += da.Convert<uint32>();
}

inline uint32 operator -= (uint32& other, const Var& da) {
  return other -= da.Convert<uint32>();
}

inline uint32 operator *= (uint32& other, const Var& da) {
  return other *= da.Convert<uint32>();
}

inline uint32 operator /= (uint32& other, const Var& da) {
  return other /= da.Convert<uint32>();
}

inline bool operator == (const uint32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<uint32>();
}

inline bool operator != (const uint32& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<uint32>();
}

inline bool operator < (const uint32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<uint32>();
}

inline bool operator <= (const uint32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<uint32>();
}

inline bool operator > (const uint32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<uint32>();
}

inline bool operator >= (const uint32& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<uint32>();
}

inline int64 operator + (const int64& other, const Var& da) {
  return other + da.Convert<int64>();
}

inline int64 operator - (const int64& other, const Var& da) {
  return other - da.Convert<int64>();
}

inline int64 operator * (const int64& other, const Var& da) {
  return other * da.Convert<int64>();
}

inline int64 operator / (const int64& other, const Var& da) {
  return other / da.Convert<int64>();
}

inline int64 operator += (int64& other, const Var& da) {
  return other += da.Convert<int64>();
}

inline int64 operator -= (int64& other, const Var& da) {
  return other -= da.Convert<int64>();
}

inline int64 operator *= (int64& other, const Var& da) {
  return other *= da.Convert<int64>();
}

inline int64 operator /= (int64& other, const Var& da) {
  return other /= da.Convert<int64>();
}

inline bool operator == (const int64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<int64>();
}

inline bool operator != (const int64& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<int64>();
}

inline bool operator < (const int64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<int64>();
}

inline bool operator <= (const int64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<int64>();
}

inline bool operator > (const int64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<int64>();
}

inline bool operator >= (const int64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<int64>();
}

inline uint64 operator + (const uint64& other, const Var& da) {
  return other + da.Convert<uint64>();
}

inline uint64 operator - (const uint64& other, const Var& da) {
  return other - da.Convert<uint64>();
}

inline uint64 operator * (const uint64& other, const Var& da) {
  return other * da.Convert<uint64>();
}

inline uint64 operator / (const uint64& other, const Var& da) {
  return other / da.Convert<uint64>();
}

inline uint64 operator += (uint64& other, const Var& da) {
  return other += da.Convert<uint64>();
}

inline uint64 operator -= (uint64& other, const Var& da) {
  return other -= da.Convert<uint64>();
}

inline uint64 operator *= (uint64& other, const Var& da) {
  return other *= da.Convert<uint64>();
}

inline uint64 operator /= (uint64& other, const Var& da) {
  return other /= da.Convert<uint64>();
}

inline bool operator == (const uint64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<uint64>();
}

inline bool operator != (const uint64& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<uint64>();
}

inline bool operator < (const uint64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<uint64>();
}

inline bool operator <= (const uint64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<uint64>();
}

inline bool operator > (const uint64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<uint64>();
}

inline bool operator >= (const uint64& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<uint64>();
}

inline float operator + (const float& other, const Var& da) {
  return other + da.Convert<float>();
}

inline float operator - (const float& other, const Var& da) {
  return other - da.Convert<float>();
}

inline float operator * (const float& other, const Var& da) {
  return other * da.Convert<float>();
}

inline float operator / (const float& other, const Var& da) {
  return other / da.Convert<float>();
}

inline float operator += (float& other, const Var& da) {
  return other += da.Convert<float>();
}

inline float operator -= (float& other, const Var& da) {
  return other -= da.Convert<float>();
}

inline float operator *= (float& other, const Var& da) {
  return other *= da.Convert<float>();
}

inline float operator /= (float& other, const Var& da) {
  return other /= da.Convert<float>();
}

inline bool operator == (const float& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<float>();
}

inline bool operator != (const float& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<float>();
}

inline bool operator < (const float& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<float>();
}

inline bool operator <= (const float& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<float>();
}

inline bool operator > (const float& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<float>();
}

inline bool operator >= (const float& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<float>();
}

inline double operator + (const double& other, const Var& da) {
  return other + da.Convert<double>();
}

inline double operator - (const double& other, const Var& da) {
  return other - da.Convert<double>();
}

inline double operator * (const double& other, const Var& da) {
  return other * da.Convert<double>();
}

inline double operator / (const double& other, const Var& da) {
  return other / da.Convert<double>();
}

inline double operator += (double& other, const Var& da) {
  return other += da.Convert<double>();
}

inline double operator -= (double& other, const Var& da) {
  return other -= da.Convert<double>();
}

inline double operator *= (double& other, const Var& da) {
  return other *= da.Convert<double>();
}

inline double operator /= (double& other, const Var& da) {
  return other /= da.Convert<double>();
}

inline bool operator == (const double& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<double>();
}

inline bool operator != (const double& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<double>();
}

inline bool operator < (const double& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<double>();
}

inline bool operator <= (const double& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<double>();
}

inline bool operator > (const double& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<double>();
}

inline bool operator >= (const double& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<double>();
}

inline bool operator == (const bool& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<bool>();
}

inline bool operator != (const bool& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<bool>();
}

inline bool operator == (const String& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<String>();
}

inline bool operator != (const String& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<String>();
}

inline bool operator == (const UString& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<UString>();
}

inline bool operator != (const UString& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<UString>();
}

inline bool operator == (const char* other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return da.Convert<String>() == other;
}

inline bool operator != (const char* other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return da.Convert<String>() != other;
}


#ifndef FUN_LONG_IS_64_BIT

inline long operator + (const long& other, const Var& da) {
  return other + da.Convert<long>();
}

inline long operator - (const long& other, const Var& da) {
  return other - da.Convert<long>();
}

inline long operator * (const long& other, const Var& da) {
  return other * da.Convert<long>();
}

inline long operator / (const long& other, const Var& da) {
  return other / da.Convert<long>();
}

inline long operator += (long& other, const Var& da) {
  return other += da.Convert<long>();
}

inline long operator -= (long& other, const Var& da) {
  return other -= da.Convert<long>();
}

inline long operator *= (long& other, const Var& da) {
  return other *= da.Convert<long>();
}

inline long operator /= (long& other, const Var& da) {
  return other /= da.Convert<long>();
}

inline bool operator == (const long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<long>();
}

inline bool operator != (const long& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<long>();
}

inline bool operator < (const long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<long>();
}

inline bool operator <= (const long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<long>();
}

inline bool operator > (const long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<long>();
}

inline bool operator >= (const long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<long>();
}

inline unsigned long operator + (const unsigned long& other, const Var& da) {
  return other + da.Convert<unsigned long>();
}

inline unsigned long operator - (const unsigned long& other, const Var& da) {
  return other - da.Convert<unsigned long>();
}

inline unsigned long operator * (const unsigned long& other, const Var& da) {
  return other * da.Convert<unsigned long>();
}

inline unsigned long operator / (const unsigned long& other, const Var& da) {
  return other / da.Convert<unsigned long>();
}

inline unsigned long operator += (unsigned long& other, const Var& da) {
  return other += da.Convert<unsigned long>();
}

inline unsigned long operator -= (unsigned long& other, const Var& da) {
  return other -= da.Convert<unsigned long>();
}

inline unsigned long operator *= (unsigned long& other, const Var& da) {
  return other *= da.Convert<unsigned long>();
}

inline unsigned long operator /= (unsigned long& other, const Var& da) {
  return other /= da.Convert<unsigned long>();
}

inline bool operator == (const unsigned long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other == da.Convert<unsigned long>();
}

inline bool operator != (const unsigned long& other, const Var& da) {
  if (da.IsEmpty()) {
    return true;
  }

  return other != da.Convert<unsigned long>();
}

inline bool operator < (const unsigned long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other < da.Convert<unsigned long>();
}

inline bool operator <= (const unsigned long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other <= da.Convert<unsigned long>();
}

inline bool operator > (const unsigned long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other > da.Convert<unsigned long>();
}

inline bool operator >= (const unsigned long& other, const Var& da) {
  if (da.IsEmpty()) {
    return false;
  }

  return other >= da.Convert<unsigned long>();
}

#endif // FUN_LONG_IS_64_BIT

} // namespace dynamic

//@ deprecated
typedef Dynamic::Var DynamicAny;

} // namespace fun
