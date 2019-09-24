#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * This is the base class for thread error handlers.
 *
 * An unhandled exception that causes a thread to terminate is usually
 * silently ignored, since the class library cannot do anything meaningful
 * about it.
 *
 * The Thread class provides the possibility to register a
 * global ErrorHandler that is invoked whenever a thread has
 * been terminated by an unhandled exception.
 * The ErrorHandler must be derived from this class and can
 * provide implementations of all three OnException() overloads.
 *
 * The ErrorHandler is always invoked within the context of
 * the offending thread.
 */
class FUN_BASE_API ErrorHandler {
 public:
  ErrorHandler();
  virtual ~ErrorHandler();

  virtual void OnException(const Exception& e);
  virtual void OnException(const std::exception& e);
  virtual void OnException();

  static void Handle(const Exception& e);
  static void Handle(const std::exception& e);
  static void Handle();

  static ErrorHandler* Set(ErrorHandler* handler);
  static ErrorHandler* Get();

 protected:
  static ErrorHandler* DefaultHandler();

 private:
  static ErrorHandler* handler_;
  static FastMutex mutex_;
};

}  // namespace fun
