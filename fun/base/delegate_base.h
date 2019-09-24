#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Base class for Delegate and Expire.
 */
template <typename ArgsType>
class DelegateBase {
 public:
  DelegateBase() {}

  DelegateBase(const DelegateBase&) {}

  virtual ~DelegateBase() {}

  /**
   * Invokes the delegate's callback function.
   * Returns true if successful, or false if the delegate
   * has been disabled or has expired.
   */
  virtual bool Notify(const void* sender, ArgsType& arguments) = 0;

  /**
   * Compares the DelegateBase with the other one for equality.
   */
  virtual bool Equals(const DelegateBase& other) const = 0;

  /**
   * Returns a deep copy of the DelegateBase.
   */
  virtual DelegateBase* Clone() const = 0;

  /**
   * Disables the delegate, which is done prior to removal.
   */
  virtual void Disable() = 0;

  /**
   * Returns the unwrapped delegate. Must be overridden by decorators
   * like Expire.
   */
  virtual const DelegateBase* Unwrap() const {
    return this;
  }
};

/**
 * Base class for Delegate and Expire.
 */
template <>
class DelegateBase<void> {
 public:
  DelegateBase() {}

  DelegateBase(const DelegateBase&) {}

  virtual ~DelegateBase() {}

  /**
   * Invokes the delegate's callback function.
   * Returns true if successful, or false if the delegate
   * has been disabled or has expired.
   */
  virtual bool Notify(const void* sender) = 0;

  /**
   * Compares the DelegateBase with the other one for equality.
   */
  virtual bool Equals(const DelegateBase& other) const = 0;

  /**
   * Returns a deep copy of the DelegateBase.
   */
  virtual DelegateBase* Clone() const = 0;

  /**
   * Disables the delegate, which is done prior to removal.
   */
  virtual void Disable() = 0;

  /**
   * Returns the unwrapped delegate. Must be overridden by decorators
   * like Expire.
   */
  virtual const DelegateBase* Unwrap() const {
    return this;
  }
};

} // namespace fun
