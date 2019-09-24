#pragma once

#include "fun/base/base.h"
#include "fun/base/optional.h"

namespace fun {

template <typename ArgType>
struct ValueOrError_ValueProxy {
  ValueOrError_ValueProxy(ArgType&& arg) : arg(arg) {}
  ArgType& arg;
};

template <typename ArgType>
struct ValueOrError_ErrorProxy {
  ValueOrError_ErrorProxy(ArgType&& arg) : arg(arg) {}
  ArgType& arg;
};

template <typename A>
FUN_ALWAYS_INLINE ValueOrError_ValueProxy<A> MakeValue(A&& arg) {
  return ValueOrError_ValueProxy<A>(Forward<A>(arg));
}

template <typename A>
FUN_ALWAYS_INLINE ValueOrError_ErrorProxy<A> MakeError(A&& arg) {
  return ValueOrError_ErrorProxy<A>(Forward<A>(arg));
}

/**
 * Type used to return either some data, or an error
 */
template <typename ValueType, typename ErrorType>
class ValueOrError {
 public:
  /**
   * Construct the result from a value, or an error
   * (See MakeValue and MakeError)
   */
  template <typename A>
  ValueOrError(ValueOrError_ValueProxy<A>&& proxy)
    : value_(MoveTemp(proxy.arg)) {}

  template <typename A>
  ValueOrError(ValueOrError_ErrorProxy<A>&& proxy)
    : error_(MoveTemp(proxy.arg)) {}

  /**
   * Move construction / assignment
   */
  ValueOrError(ValueOrError&& rhs)
    : error_(MoveTemp(rhs.error_)), value_(MoveTemp(rhs.value_)) {}

  /**
   * Move operator
   */
  ValueOrError& operator = (ValueOrError&& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      error_ = MoveTemp(rhs.error_);
      value_ = MoveTemp(rhs.value_);
    }

    return *this;
  }

  // Disable copy and assignment.
  ValueOrError(const ValueOrError&) = delete;
  ValueOrError& operator = (const ValueOrError&) = delete;

  /**
   * Check whether this value is valid
   */
  bool IsValid() const {
    return value_.IsSet() && !error_.IsSet();
  }

  /**
   * Get the error, if set
   */
  ErrorType& Error() {
    return error_.Value();
  }

  /**
   * Get the error, if set
   */
  const ErrorType& Error() const {
    return error_.Value();
  }

  /**
   * Steal this result's error, if set, causing it to become unset
   */
  ErrorType StealError() {
    ErrorType tmp = MoveTemp(error_.Value());
    error_.Reset();
    return tmp;
  }

  /**
   * Access the value contained in this result
   */
  ValueType& Value() {
    return value_.Value();
  }

  /**
   * Access the value contained in this result
   */
  const ValueType& Value() const {
    return value_.Value();
  }

  /**
   * Steal this result's value, causing it to become unset
   */
  ValueType StealValue() {
    ValueType tmp = MoveTemp(value_.Value());
    value_.Reset();
    return tmp;
  }

 private:
  /** The error reported by the procedure, if any */
  Optional<ErrorType> error_;
  /** Optional value to return as part of the result */
  Optional<ValueType> value_;
};

} // namespace fun
