#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * When we have an optional value IsSpecified() returns true
 * and Value() is meaningful.
 * Otherwise Value() is not meaningful.
 */
template <typename T>
class Optional {
 public:
  Optional() : is_specified_(false) {}

  Optional(const T& initial_value) {
    new(&value_) T(initial_value);
    is_specified_ = true;
  }

  Optional(T&& initial_value) {
    new(&value_) T(MoveTemp(initial_value));
    is_specified_ = true;
  }

  Optional(const Optional& other) {
    new(&value_) T(*(const T*)&other.value_);
    is_specified_ = true;
  }

  Optional(Optional&& other) {
    new(&value_) T(MoveTemp(*(T*)&other.value_));
    is_specified_ = true;
  }

  ~Optional() {
    Reset();
  }

  Optional& operator = (const Optional& other) {
    if (FUN_LIKELY(&other != this)) {
      Reset();

      if (other.is_specified_) {
        new(&value_) T(*(const T*)&other.value_);
        is_specified_ = true;
      }
    }
    return *this;
  }

  Optional& operator = (Optional&& other) {
    if (FUN_LIKELY(&other != this)) {
      Reset();

      if (other.is_specified_) {
        new(&value_) T(MoveTemp(*(T*)&other.value_));
        is_specified_ = true;
      }
    }
    return *this;
  }

  Optional& operator = (const T& value) {
    if (FUN_LIKELY(&value != (T*)&value_)) {
      Reset();
      new(&value_) T(value);
      is_specified_ = true;
    }
    return *this;
  }

  Optional& operator = (T&& value) {
    if (FUN_LIKELY(&value != (T*)&value_)) {
      Reset();
      new(&value_) T(MoveTemp(value));
      is_specified_ = true;
    }
    return *this;
  }

  void Reset() {
    if (is_specified_) {
      is_specified_ = false;
      reinterpret_cast<T*>(&value_)->~T();
    }
  }

  template <typename... Args>
  void Emplace(Args&&... args) {
    Reset();
    new(&value_) T(Forward<Args>(args)...);
    is_specified_ = true;
  }

  /**
   * Returns true when the value is meaningful; false if calling Value() is undefined.
   */
  bool IsSpecified() const {
    return is_specified_;
  }

  explicit operator bool() const {
    return is_specified_;
  }

  /**
   * Returns The optional value; undefined when IsSpecified() returns false.
   */
  const T& Value() const {
    fun_check_msg(IsSpecified(), "It is an error to call Value() on an unset Optional. Please either check IsSpecified() or use ValueOr(default_value) instead.");
    return *(T*)&value_;
  }

  T& Value() {
    fun_check_msg(IsSpecified(), "It is an error to call Value() on an unset Optional. Please either check IsSpecified() or use ValueOr(default_value) instead.");
    return *(T*)&value_;
  }

  const T* operator -> () const {
    return &Value();
  }

  T* operator -> () {
     return &Value();
  }

  /**
   * Returns The optional value when set; default_value otherwise.
   */
  const T& ValueOr(const T& default_value) const {
    return IsSpecified() ? *(T*)&value_ : default_value;
  }

  friend bool operator == (const Optional& lhs, const Optional& rhs) {
    if (lhs.is_specified_ != rhs.is_specified_) {
      return false;
    }

    if (!lhs.is_specified_) { // both unset
      return true;
    }

    return (*(T*)&lhs.value_) == (*(T*)&rhs.value_);
  }

  friend bool operator != (const Optional& lhs, const Optional& rhs) {
    return !(lhs == rhs);
  }

 private:
  bool is_specified_;
  TypeCompatibleStorage<T> value_;
};

} // namespace fun
