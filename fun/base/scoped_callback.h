#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Helper object for batching callback requests and firing on
 * destruction of the ScopedCallback object.
 *
 * CallbackType is a class implementing a static method
 * called FireCallback, which does the work.
 */
template <typename CallbackType>
class ScopedCallback {
 public:
  ScopedCallback() : counter_(0) {}

  /**
   * Fires a callback if outstanding requests exist.
   */
  ~ScopedCallback() {
    if (HasRequests()) {
      CallbackType::FireCallback();
    }
  }

  /**
   * Request a callback.
   */
  void Request() {
    ++counter_;
  }

  /**
   * Unrequest a callback.
   */
  void Unrequest() {
    --counter_;
  }

  /**
   * Checks whether this callback has outstanding requests.
   *
   * \return true if there are outstanding requests, false otherwise.
   */
  bool HasRequests() const {
    return counter_ > 0;
  }

 private:
  /** Counts callback requests. */
  int32 counter_;
};

} // namespace fun
