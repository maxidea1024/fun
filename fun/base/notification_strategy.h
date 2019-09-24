#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * The interface that all notification strategies must implement.
 *
 * Note: Event is based on policy-driven design, so every strategy
 * implementation must provide all the methods from this interface (otherwise:
 * compile errors) but does not need to inherit from NotificationStrategy.
 */
template <typename ArgsType, typename DelegateType>
class NotificationStrategy {
 public:
  using DelegateHandle = DelegateType*;

  NotificationStrategy() {}

  virtual ~NotificationStrategy() {}

  /**
   * Sends a notification to all registered delegates.
   */
  virtual void Notify(const void* sender, ArgsType& arguments) = 0;

  /**
   * Adds a delegate to the strategy.
   */
  virtual DelegateHandle Add(const DelegateType& delegate) = 0;

  /**
   * Removes a delegate from the strategy, if found.
   * Does nothing if the delegate has not been added.
   */
  virtual void Remove(const DelegateType& delegate) = 0;

  /**
   * Removes a delegate from the strategy, if found.
   * Does nothing if the delegate has not been added.
   */
  virtual void Remove(DelegateHandle delegate_handle) = 0;

  /**
   * Removes all delegates from the strategy.
   */
  virtual void Clear() = 0;

  /**
   * Returns false if the strategy contains at least one delegate.
   */
  virtual bool IsEmpty() const = 0;
};

/**
 * The interface that all notification strategies must implement.
 *
 * Note: Event is based on policy-driven design, so every strategy
 * implementation must provide all the methods from this interface (otherwise:
 * compile errors) but does not need to inherit from NotificationStrategy.
 */
template <typename DelegateType>
class NotificationStrategy<void, DelegateType> {
 public:
  using DelegateHandle = DelegateType*;

  NotificationStrategy() {}

  virtual ~NotificationStrategy() {}

  /**
   * Sends a notification to all registered delegates.
   */
  virtual void Notify(const void* sender) = 0;

  /**
   * Adds a delegate to the strategy.
   */
  virtual DelegateHandle Add(const DelegateType& delegate) = 0;

  /**
   * Removes a delegate from the strategy, if found.
   * Does nothing if the delegate has not been added.
   */
  virtual void Remove(const DelegateType& delegate) = 0;

  /**
   * Removes a delegate from the strategy, if found.
   * Does nothing if the delegate has not been added.
   */
  virtual void Remove(DelegateHandle delegate_handle) = 0;

  /**
   * Removes all delegates from the strategy.
   */
  virtual void Clear() = 0;

  /**
   * Returns false if the strategy contains at least one delegate.
   */
  virtual bool IsEmpty() const = 0;
};

}  // namespace fun
