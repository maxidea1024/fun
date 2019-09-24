#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

namespace fun {

/**
 * Nullable is a simple wrapper class for value types
 * that allows objects or native type variables
 * to have "null" value.
 *
 * The class is useful for passing parameters to functions
 * when parameters are optional and no default values
 * should be used or when a non-assigned state is needed,
 * such as in e.g. fetching null values from database.
 *
 * A Nullable can be default constructed. In this case,
 * the Nullable will have a null value and IsNull() will
 * return true. Calling Value() (without default value) on
 * a null object will throw a NullValueException.
 *
 * A Nullable can also be constructed from a value.
 * It is possible to assign a value to a Nullable, and
 * to reset a Nullable to contain a null value by calling
 * Clear().
 *
 * For use with Nullable, the value type should support
 * default construction.
 */
template <typename T>
class Nullable {
 public:
  /**
   * Creates an empty Nullable.
   */
  Nullable() : value_(), is_null_(true), null_() {}

  /**
   * Creates an empty Nullable.
   */
  Nullable(const decltype(nullptr)&) : value_(), is_null_(true), null_() {}

  /**
   * Creates a Nullable with the given value.
   */
  Nullable(const T& value) : value_(value), is_null_(false), null_() {}

  /**
   * Creates a Nullable by copying another one.
   */
  Nullable(const Nullable& other)
      : value_(other.value_), is_null_(other.is_null_), null_() {}

  /**
   * Destroys the Nullable.
   */
  ~Nullable() {}

  /**
   * Assigns a value to the Nullable.
   */
  Nullable& Assign(const T& value) {
    value_ = value;
    is_null_ = false;
    return *this;
  }

  /**
   * Assigns another Nullable.
   */
  Nullable& Assign(const Nullable& other) {
    Nullable tmp(other);
    Swap(tmp);
    return *this;
  }

  /**
   * Sets value to null.
   */
  Nullable& Assign(decltype(nullptr)) {
    is_null_ = true;
    return *this;
  }

  /**
   * Assigns a value to the Nullable.
   */
  Nullable& operator=(const T& value) { return Assign(value); }

  /**
   * Assigns another Nullable.
   */
  Nullable& operator=(const Nullable& other) { return Assign(other); }

  /**
   * Assigns another Nullable.
   */
  Nullable& operator=(decltype(nullptr)) {
    is_null_ = true;
    return *this;
  }

  /**
   * Swaps this Nullable with other.
   */
  void Swap(Nullable& other) {
    if (FUN_LIKELY(&other != this)) {
      fun::Swap(value_, other.value_);
      fun::Swap(is_null_, other.is_null_);
    }
  }

  /**
   * Compares two Nullables for equality
   */
  bool operator==(const Nullable<T>& other) const {
    return (is_null_ && other.is_null_) ||
           (is_null_ == other.is_null_ && value_ == other.value_);
  }

  /**
   * Compares Nullable with value for equality
   */
  bool operator==(const T& value) const {
    return (!is_null_ && value_ == value);
  }

  /**
   * Compares Nullable with NullData for equality
   */
  bool operator==(const decltype(nullptr)&) const { return is_null_; }

  /**
   * Compares Nullable with value for non equality
   */
  bool operator!=(const T& value) const { return !(*this == value); }

  /**
   * Compares two Nullables for non equality
   */
  bool operator!=(const Nullable<T>& other) const { return !(*this == other); }

  /**
   * Compares with NullData for non equality
   */
  bool operator!=(const decltype(nullptr)&) const { return !is_null_; }

  /**
   * Compares two Nullable objects. Return true if this object's
   * value is smaller than the other object's value.
   * null value is smaller than a non-null value.
   */
  bool operator<(const Nullable<T>& other) const {
    if (is_null_ && other.is_null_) {
      return false;
    }

    if (!is_null_ && !other.is_null_) {
      return (value_ < other.value_);
    }

    if (is_null_ && !other.is_null_) {
      return true;
    }

    return false;
  }

  /**
   * Compares two Nullable objects. Return true if this object's
   * value is greater than the other object's value.
   * A non-null value is greater than a null value.
   */
  bool operator>(const Nullable<T>& other) const {
    return !(*this == other) && !(*this < other);
  }

  /**
   * Returns the Nullable's value.
   *
   * Throws a NullValueException if the Nullable is empty.
   */
  T& Value() {
    if (!is_null_) {
      return value_;
    }

    throw NullValueException();
  }

  /**
   * Returns the Nullable's value.
   *
   * Throws a NullValueException if the Nullable is empty.
   */
  const T& Value() const {
    if (!is_null_) {
      return value_;
    }

    throw NullValueException();
  }

  /**
   * Returns the Nullable's value, or the
   * given default value if the Nullable is empty.
   */
  const T& Value(const T& default_value) const {
    return is_null_ ? default_value : value_;
  }

  /**
   * Get reference to the value
   */
  operator T&() { return Value(); }

  /**
   * Get const reference to the value
   */
  operator const T&() const { return Value(); }

  /**
   * Get reference to the value
   */
  operator decltype(nullptr)&() { return null_; }

  /**
   * Returns true if the Nullable is empty.
   */
  bool IsNull() const { return is_null_; }

  bool HasValue() const { return is_null_ == false; }

  /**
   * Clears the Nullable.
   */
  void Clear() { is_null_ = true; }

  friend void Swap(Nullable<T>& x, Nullable<T>& y) { x.Swap(y); }

  friend uint32 HashOf(const Nullable<T>& n) {
    return n.IsNull() ? 0 : HashOf(n.Value());
  }

  friend Archive& operator&(Archive& ar, Nullable<T>& n) {
    ar& n.is_null_;

    if (!is_null_) {
      ar& n.value_;
    }

    return ar;
  }

  friend String ToString(const Nullable<T>& n) {
    return n.is_null_ ? "<null>" : ToString(n.value_);
  }

 private:
  T value_;
  bool is_null_;
  decltype(nullptr) null_;
};

//
// inlines
//

template <typename T>
FUN_ALWAYS_INLINE bool operator==(const decltype(nullptr)&,
                                  const Nullable<T>& n) {
  return n.IsNull();
}

template <typename T>
FUN_ALWAYS_INLINE bool operator!=(const T& v, const Nullable<T>& n) {
  return !(n == v);
}

template <typename T>
FUN_ALWAYS_INLINE bool operator==(const T& v, const Nullable<T>& n) {
  return (n == v);
}

template <typename T>
FUN_ALWAYS_INLINE bool operator!=(const decltype(nullptr)&,
                                  const Nullable<T>& n) {
  return n.IsNull();
}

}  // namespace fun
