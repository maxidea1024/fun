#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * The Runnable interface with the Run() method
 * must be implemented by classes that provide
 * an entry point for a thread.
 */
class FUN_BASE_API Runnable {
 public:
  Runnable();
  virtual ~Runnable();

  /**
   * Do whatever the thread needs to do. Must be overridden by subclasses.
   */
  virtual void Run() = 0;
};

}  // namespace fun
